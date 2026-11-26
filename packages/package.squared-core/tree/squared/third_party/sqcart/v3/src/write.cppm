// SPDX-License-Identifier: MIT
//
// sq.cart.write — the 3.0 writer (docs/developer/specs/sq-cartridge-3.0.md
// §10): sqcart is the only thing that writes cartridge bytes, and this is
// it. A Writer holds the lock (<file>.lock, §10.2), stages puts, removals,
// renames, metadata changes, and ranges and resizes of entries, and commits
// them (§10.3): append, flush, write the spare slot, flush. compact()
// rewrites a cartridge as one commit (§10.4).
//
// Its own module, apart from sq.cart, so a program that only reads carries
// no compressor: this one links zstd's whole library (../third_party/zstd-
// 1.5.7/zstd.c) where a reader links its decoder alone.
//
// Staging is in memory (put_file aside, read at commit): what a commit
// writes is held until commit(). The Virtual Cartridge Drive (sqdrive), a
// tool of its own, is what stages large changes on disk and decides when
// to commit (§10.5).

module;

#define ZSTD_STATIC_LINKING_ONLY
#include <zstd.h>
#define ZDICT_STATIC_LINKING_ONLY
#include <zdict.h>

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#if defined(__linux__)
#include <sys/vfs.h>
#endif

export module sq.cart.write;

import std;
import sq.data;
import sq.cart;

namespace sq::cart {

[[nodiscard]] inline std::unexpected<Error> fail(Code code, std::string message,
                                                 std::optional<std::string> entry = std::nullopt) {
    return std::unexpected(Error{code, std::move(message), std::move(entry)});
}

constexpr std::uint64_t header_bytes = 8192;
constexpr std::uint64_t slot_a_at = 64;
constexpr std::uint64_t slot_b_at = 4096;
constexpr std::size_t entry_record = 128;
constexpr std::size_t commit_record = 224;
constexpr std::size_t chunk_record_bytes = 64;
constexpr std::uint64_t mappable_align = 16384;
constexpr std::uint64_t fat32_limit = (std::uint64_t{4} << 30) - 1;

void le(std::span<std::byte> out, std::size_t at, std::uint64_t v, int n) {
    for (int i = 0; i < n; ++i) out[at + static_cast<std::size_t>(i)] = static_cast<std::byte>(v >> (8 * i));
}

std::string errno_text(std::string_view what, const std::filesystem::path& path) {
    return std::format("{} {}: {}", what, path.string(), std::strerror(errno));
}

/// Media types whose content is compressed already (§4.3: stored).
bool compressed_already(const std::optional<std::string>& media_type) {
    if (!media_type) return false;
    static constexpr std::string_view kinds[] = {"image/png", "image/jpeg", "image/webp", "image/gif", "image/avif",
                                                 "audio/", "video/", "application/zstd", "application/zip",
                                                 "application/gzip", "font/woff2"};
    return std::ranges::any_of(kinds, [&](std::string_view k) { return media_type->starts_with(k); });
}

/// This process's identity for a lock (§10.2): its host, its process ID,
/// and when that process started (so a later process given the same ID is
/// not taken for it).
struct Holder {
    std::string host;
    long pid = 0;
    std::uint64_t started = 0;

    static std::uint64_t start_time(long pid) {
#if defined(__linux__)
        // /proc/<pid>/stat: field 22, after the command name in parentheses.
        std::ifstream in(std::format("/proc/{}/stat", pid));
        std::string line;
        if (!std::getline(in, line)) return 0;
        const auto close = line.rfind(')');
        if (close == std::string::npos) return 0;
        std::istringstream rest(line.substr(close + 2));
        std::string field;
        for (int i = 3; i <= 22 && rest >> field; ++i) {
            if (i == 22) return std::strtoull(field.c_str(), nullptr, 10);
        }
#endif
        static_cast<void>(pid);
        return 0;
    }

    static Holder self() {
        std::array<char, 256> host{};
        ::gethostname(host.data(), host.size() - 1);
        const long pid = static_cast<long>(::getpid());
        return {host.data(), pid, start_time(pid)};
    }

    [[nodiscard]] std::string text() const { return std::format("{} {} {}\n", host, pid, started); }

    static std::optional<Holder> parse(std::string_view text) {
        Holder h;
        std::istringstream in{std::string(text)};
        if (!(in >> h.host >> h.pid >> h.started)) return std::nullopt;
        return h;
    }

    /// Whether this holder is a process still running on this host.
    [[nodiscard]] bool alive_here() const {
        if (::kill(static_cast<pid_t>(pid), 0) != 0 && errno != EPERM) return false;
        const std::uint64_t now = start_time(pid);
        return now == 0 || started == 0 || now == started;
    }
};

}  // namespace sq::cart

export namespace sq::cart {

/// How a writer locks (§10.2).
enum class LockMode : std::uint8_t {
    automatic,   ///< flock on <file>.lock; where the file system has none, the owner file
    owner_file,  ///< <file>.lock.owner, created exclusively, naming its holder
};

}  // namespace sq::cart

namespace sq::cart {

/// The lock of §10.2, never on the cartridge (compaction renames a new file
/// over it): an exclusive flock on <file>.lock, which dies with the
/// process; or, on a file system without flock (some FUSE and network
/// ones), <file>.lock.owner, created exclusively (atomic where flock is
/// not), holding the writer's host, process and start time, and removed
/// when it is released. An owner file left by a process gone from this
/// host is taken over; one from another host never is.
class Lock {
public:
    static Result<Lock> take(const std::filesystem::path& cartridge, LockMode mode = LockMode::automatic) {
        auto path = cartridge;
        path += ".lock";
        if (mode == LockMode::automatic) {
            const int fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0644);
            if (fd < 0) return fail(Code::io, errno_text("cannot open the lock", path));
            if (::flock(fd, LOCK_EX | LOCK_NB) == 0) return Lock(fd, {});
            const int why = errno;
            ::close(fd);
            if (why == EWOULDBLOCK) {
                return fail(Code::busy, std::format("{} is being written by another process ({} is locked)", cartridge.string(), path.string()));
            }
            if (why != EINVAL && why != ENOSYS && why != EOPNOTSUPP && why != ENOLCK) {
                return fail(Code::io, std::format("cannot lock {}: {}", path.string(), std::strerror(why)));
            }
        }
        auto owner = path;
        owner += ".owner";
        const Holder me = Holder::self();
        for (int attempt = 0; attempt < 2; ++attempt) {
            const int fd = ::open(owner.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0644);
            if (fd >= 0) {
                const std::string t = me.text();
                const bool written = ::write(fd, t.data(), t.size()) == static_cast<ssize_t>(t.size()) && ::fsync(fd) == 0;
                ::close(fd);
                if (!written) {
                    ::unlink(owner.c_str());
                    return fail(Code::io, errno_text("cannot write the lock", owner));
                }
                return Lock(-1, owner);
            }
            if (errno != EEXIST) return fail(Code::io, errno_text("cannot create the lock", owner));
            std::ifstream in(owner);
            const std::string held{std::istreambuf_iterator<char>(in), {}};
            const auto holder = Holder::parse(held);
            if (holder && holder->host != me.host) {
                return fail(Code::busy, std::format("{} is held by a process on {} ({}): only that host can say it is gone", cartridge.string(),
                                                  holder->host, owner.string()));
            }
            if (holder && holder->alive_here()) {
                return fail(Code::busy, std::format("{} is being written by process {} ({})", cartridge.string(), holder->pid, owner.string()));
            }
            // A holder gone from this host (or a lock never finished): take it over, once.
            if (attempt == 0) ::unlink(owner.c_str());
        }
        return fail(Code::io, std::format("cannot take the lock {}", owner.string()));
    }
    Lock(Lock&& other) noexcept : fd_(std::exchange(other.fd_, -1)), owner_(std::exchange(other.owner_, {})) {}
    Lock& operator=(Lock&& other) noexcept {
        if (this != &other) {
            release();
            fd_ = std::exchange(other.fd_, -1);
            owner_ = std::exchange(other.owner_, {});
        }
        return *this;
    }
    ~Lock() { release(); }

private:
    Lock(int fd, std::filesystem::path owner) : fd_(fd), owner_(std::move(owner)) {}
    void release() noexcept {
        if (fd_ >= 0) ::close(fd_);  // closing drops the flock
        fd_ = -1;
        if (!owner_.empty()) ::unlink(owner_.c_str());
        owner_.clear();
    }
    int fd_ = -1;
    std::filesystem::path owner_;
};

/// A file being appended to: pwrite at the end, flushes that reach storage.
class Appender {
public:
    static Result<Appender> open(const std::filesystem::path& path, bool create) {
        const int flags = O_RDWR | O_CLOEXEC | (create ? O_CREAT | O_EXCL : 0);
        const int fd = ::open(path.c_str(), flags, 0644);
        if (fd < 0) return fail(Code::io, errno_text(create ? "cannot create" : "cannot open", path));
        struct stat st{};
        ::fstat(fd, &st);
        return Appender(fd, path, static_cast<std::uint64_t>(st.st_size));
    }
    Appender(Appender&& o) noexcept : fd_(std::exchange(o.fd_, -1)), path_(std::move(o.path_)), size_(o.size_) {}
    Appender& operator=(Appender&& o) noexcept {
        if (this != &o) {
            if (fd_ >= 0) ::close(fd_);
            fd_ = std::exchange(o.fd_, -1);
            path_ = std::move(o.path_);
            size_ = o.size_;
        }
        return *this;
    }
    ~Appender() {
        if (fd_ >= 0) ::close(fd_);
    }

    [[nodiscard]] std::uint64_t size() const noexcept { return size_; }
    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

    Result<void> write_at(std::uint64_t at, std::span<const std::byte> bytes) {
        std::size_t done = 0;
        while (done < bytes.size()) {
            const ssize_t n = ::pwrite(fd_, bytes.data() + done, bytes.size() - done, static_cast<off_t>(at + done));
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) return fail(Code::io, errno_text("cannot write", path_));
            done += static_cast<std::size_t>(n);
        }
        size_ = std::max(size_, at + bytes.size());
        return {};
    }
    Result<void> append(std::span<const std::byte> bytes) { return write_at(size_, bytes); }
    Result<void> pad_to(std::uint64_t alignment) {
        const std::uint64_t padded = (size_ + alignment - 1) / alignment * alignment;
        if (padded == size_) return {};
        std::vector<std::byte> zero(static_cast<std::size_t>(padded - size_));
        return append(zero);
    }
    Result<void> truncate(std::uint64_t length) {
        if (::ftruncate(fd_, static_cast<off_t>(length)) != 0) return fail(Code::io, errno_text("cannot truncate", path_));
        size_ = length;
        return {};
    }
    /// A flush that reaches storage (§10.3): F_FULLFSYNC where fsync stops
    /// at the drive's cache (macOS).
    Result<void> flush() {
#if defined(__APPLE__)
        if (::fcntl(fd_, F_FULLFSYNC) == 0) return {};
#endif
        if (::fsync(fd_) != 0) return fail(Code::io, errno_text("cannot flush", path_));
        return {};
    }
    Result<std::vector<std::byte>> read(std::uint64_t at, std::uint64_t n) const {
        std::vector<std::byte> out(static_cast<std::size_t>(n));
        std::size_t done = 0;
        while (done < out.size()) {
            const ssize_t got = ::pread(fd_, out.data() + done, out.size() - done, static_cast<off_t>(at + done));
            if (got < 0 && errno == EINTR) continue;
            if (got <= 0) return fail(Code::io, errno_text("cannot read", path_));
            done += static_cast<std::size_t>(got);
        }
        return out;
    }
    /// Whether the file system is FAT, which cannot hold 4 GiB (§10.6).
    [[nodiscard]] bool on_fat() const noexcept {
#if defined(__linux__)
        struct statfs fs{};
        return ::fstatfs(fd_, &fs) == 0 && fs.f_type == 0x4d44;  // MSDOS_SUPER_MAGIC
#else
        return false;
#endif
    }

private:
    Appender(int fd, std::filesystem::path path, std::uint64_t size) : fd_(fd), path_(std::move(path)), size_(size) {}
    int fd_ = -1;
    std::filesystem::path path_;
    std::uint64_t size_ = 0;
};

/// Flush the folder a file is in, so a name created or renamed there
/// survives a crash (§10.3, §10.4).
Result<void> flush_folder(const std::filesystem::path& file) {
    auto dir = file.parent_path();
    if (dir.empty()) dir = ".";
    const int fd = ::open(dir.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0) return fail(Code::io, errno_text("cannot open the folder", dir));
    const int r = ::fsync(fd);
    ::close(fd);
    if (r != 0 && errno != EINVAL) return fail(Code::io, errno_text("cannot flush the folder", dir));
    return {};
}

/// A zstd frame of `content` compressed with `dictionary` (its ID in the
/// frame header, §4.3).
Result<std::vector<std::byte>> zstd_dict_frame(std::span<const std::byte> content, std::span<const std::byte> dictionary, int level) {
    std::unique_ptr<ZSTD_CCtx, decltype(&ZSTD_freeCCtx)> ctx(ZSTD_createCCtx(), ZSTD_freeCCtx);
    if (!ctx) return fail(Code::io, "out of memory for a zstd compressor");
    ZSTD_CCtx_setParameter(ctx.get(), ZSTD_c_compressionLevel, level);
    ZSTD_CCtx_setParameter(ctx.get(), ZSTD_c_contentSizeFlag, 1);
    ZSTD_CCtx_setParameter(ctx.get(), ZSTD_c_checksumFlag, 0);
    ZSTD_CCtx_setParameter(ctx.get(), ZSTD_c_dictIDFlag, 1);
    ZSTD_CCtx_setParameter(ctx.get(), ZSTD_c_nbWorkers, 0);
    if (const std::size_t r = ZSTD_CCtx_loadDictionary(ctx.get(), dictionary.data(), dictionary.size()); ZSTD_isError(r)) {
        return fail(Code::malformed, std::format("an unusable zstd dictionary: {}", ZSTD_getErrorName(r)));
    }
    std::vector<std::byte> out(ZSTD_compressBound(content.size()));
    const std::size_t n = ZSTD_compress2(ctx.get(), out.data(), out.size(), content.data(), content.size());
    if (ZSTD_isError(n)) return fail(Code::io, std::format("zstd: {}", ZSTD_getErrorName(n)));
    out.resize(n);
    return out;
}

/// A zstd frame of `content`, with its content size and checksum-free, at
/// `level`, its window within 2^window_log, single-threaded (§3.10: the
/// same bytes every time).
Result<std::vector<std::byte>> zstd_frame(std::span<const std::byte> content, int level, std::uint32_t window_log) {
    std::unique_ptr<ZSTD_CCtx, decltype(&ZSTD_freeCCtx)> ctx(ZSTD_createCCtx(), ZSTD_freeCCtx);
    if (!ctx) return fail(Code::io, "out of memory for a zstd compressor");
    ZSTD_CCtx_setParameter(ctx.get(), ZSTD_c_compressionLevel, level);
    ZSTD_CCtx_setParameter(ctx.get(), ZSTD_c_contentSizeFlag, 1);
    ZSTD_CCtx_setParameter(ctx.get(), ZSTD_c_checksumFlag, 0);
    ZSTD_CCtx_setParameter(ctx.get(), ZSTD_c_nbWorkers, 0);
    // Only cap the window; zstd picks a smaller one for small content.
    if (const int wlog = static_cast<int>(window_log); content.size() > (std::size_t{1} << wlog)) {
        ZSTD_CCtx_setParameter(ctx.get(), ZSTD_c_windowLog, wlog);
    }
    std::vector<std::byte> out(ZSTD_compressBound(content.size()));
    const std::size_t n = ZSTD_compress2(ctx.get(), out.data(), out.size(), content.data(), content.size());
    if (ZSTD_isError(n)) return fail(Code::io, std::format("zstd: {}", ZSTD_getErrorName(n)));
    out.resize(n);
    return out;
}

}  // namespace sq::cart

export namespace sq::cart {

/// How a writer compresses (§4.3's profiles) and commits.
struct WriteOptions {
    Limits limits{};
    int level = 19;                ///< `small`; 3 is `fast`
    std::uint32_t window_log = 23; ///< up to 27 sets `zstd-window-27`
    /// Flush to storage on commit (§10.3). Off only for tests and scratch files.
    bool sync = true;
    /// The commit time; else SOURCE_DATE_EPOCH (in seconds) when set, else now (§3.10).
    std::optional<std::int64_t> commit_time;
    LockMode lock = LockMode::automatic;
};

/// How an entry is stored (§4.3).
enum class Store : std::uint8_t {
    automatic,  ///< stored when compressed already or it would not shrink 5%; chunked past 64 MiB; else zstd
    stored,
    zstd,
    chunked,
};

/// An entry's metadata (§4.2) and how to store it.
struct PutOptions {
    std::optional<std::string> title, author, media_type, attributes;
    std::int64_t created = 0, updated = 0;
    std::uint16_t flags = flag::r;
    Store store = Store::automatic;
    /// Compress with this dictionary (codec 2, §4.3): one trained with
    /// train_dictionary, or already in the cartridge. 0: none.
    std::uint32_t dictionary = 0;
};

/// A key to sign commits with (§9.3).
struct SigningKey {
    std::array<std::byte, 64> secret{};
    std::array<std::byte, 32> public_key{};
    std::string signer;

    /// From a 32-byte seed (Monocypher's Ed25519 key pair). The seed is wiped.
    static SigningKey from_seed(std::array<std::byte, 32> seed, std::string signer = {}) {
        SigningKey k;
        ed25519_key_pair(seed, k.secret, k.public_key);
        k.signer = std::move(signer);
        return k;
    }
};

/// One cartridge open for writing, under its lock (§10.1–10.3).
class Writer {
public:
    /// A new cartridge; refuses a file that is there already.
    [[nodiscard]] static Result<Writer> create(const std::filesystem::path& path, WriteOptions options = {}) {
        auto lock = Lock::take(path, options.lock);
        if (!lock) return std::unexpected(lock.error());
        auto file = Appender::open(path, true);
        if (!file) return std::unexpected(file.error());
        std::vector<std::byte> header(header_bytes);
        for (std::size_t i = 0; i < magic.size(); ++i) header[i] = static_cast<std::byte>(magic[i]);
        le(header, 8, major_version, 2);
        le(header, 10, 0, 2);
        le(header, 12, header_bytes, 4);
        if (auto r = file->append(header); !r) return std::unexpected(r.error());
        if (options.sync) {
            if (auto r = file->flush(); !r) return std::unexpected(r.error());
            if (auto r = flush_folder(path); !r) return std::unexpected(r.error());
        }
        return Writer(std::move(*lock), std::move(*file), std::move(options));
    }

    /// An existing cartridge: its lock taken, trailing bytes truncated (§10.3).
    [[nodiscard]] static Result<Writer> open(const std::filesystem::path& path, WriteOptions options = {}) {
        auto lock = Lock::take(path, options.lock);
        if (!lock) return std::unexpected(lock.error());
        auto archive = Archive::open(path, ReadOptions{options.limits});
        if (!archive) return std::unexpected(archive.error());
        auto file = Appender::open(path, false);
        if (!file) return std::unexpected(file.error());
        Writer w(std::move(*lock), std::move(*file), std::move(options));
        if (auto r = w.adopt(*archive); !r) return std::unexpected(r.error());
        if (const std::uint64_t end = archive->commit().offset + commit_record; w.file_.size() != end) {
            if (auto r = w.file_.truncate(end); !r) return std::unexpected(r.error());
        }
        return w;
    }

    /// Put an entry: its whole content, replacing any entry at that path.
    Result<void> put(std::string path, std::span<const std::byte> content, PutOptions options = {}) {
        if (auto problem = path_problem(path, options_.limits.max_path_bytes)) return fail(Code::path, *problem, path);
        if (auto r = check_metadata(path, options); !r) return r;
        Staged s;
        s.kind = Staged::Kind::put;
        s.content.assign(content.begin(), content.end());
        s.options = std::move(options);
        staged_.insert_or_assign(std::move(path), std::move(s));
        return {};
    }

    /// Put an entry from a file, read at commit in 1 MiB pieces: chunked
    /// past 64 MiB (or with Store::chunked) without ever holding it whole.
    /// The file must stay as it is until the commit.
    Result<void> put_file(std::string path, std::filesystem::path file, PutOptions options = {}) {
        if (auto problem = path_problem(path, options_.limits.max_path_bytes)) return fail(Code::path, *problem, path);
        if (auto r = check_metadata(path, options); !r) return r;
        std::error_code ec;
        if (!std::filesystem::is_regular_file(file, ec)) return fail(Code::io, file.string() + " is not a regular file", path);
        Staged s;
        s.kind = Staged::Kind::put;
        s.file = std::move(file);
        s.options = std::move(options);
        staged_.insert_or_assign(std::move(path), std::move(s));
        return {};
    }

    /// Remove an entry (a tombstone, §3.5).
    Result<void> remove(std::string_view path) {
        if (!exists(path)) return fail(Code::not_found, "no such entry", std::string(path));
        Staged s;
        s.kind = live_.contains(path) ? Staged::Kind::remove : Staged::Kind::forget;
        staged_.insert_or_assign(std::string(path), std::move(s));
        return {};
    }

    /// Give an entry a new path, replacing any entry there. Its content is
    /// not written again: the new record names the same runs (§3.6), and
    /// what is staged for it moves with it.
    Result<void> rename(std::string_view from_path, std::string to) {
        const std::string from(from_path);
        if (from == to) return exists(from) ? Result<void>{} : fail(Code::not_found, "no such entry", from);
        if (auto problem = path_problem(to, options_.limits.max_path_bytes)) return fail(Code::path, *problem, to);
        for (const std::string& p : {from, to}) {
            if (p == "SQ-INF/manifest.json" || p.starts_with("SQ-INF/dictionaries/")) {
                return fail(Code::path, "the manifest and dictionaries keep their paths (§4.1, §4.3)", p);
            }
        }
        Staged moved;
        if (auto it = staged_.find(from); it != staged_.end()) {
            if (it->second.kind == Staged::Kind::remove || it->second.kind == Staged::Kind::forget) {
                return fail(Code::not_found, "no such entry", from);
            }
            moved = std::move(it->second);
        } else if (live_.contains(from)) {
            moved.kind = Staged::Kind::edit;
            moved.source = from;
        } else {
            return fail(Code::not_found, "no such entry", from);
        }
        Staged gone;
        gone.kind = live_.contains(from) ? Staged::Kind::remove : Staged::Kind::forget;
        staged_.insert_or_assign(from, std::move(gone));
        staged_.insert_or_assign(std::move(to), std::move(moved));
        return {};
    }

    /// Change an entry's metadata only: its content, wherever it is, is not
    /// written again (§3.6). store and dictionary are ignored: an entry put
    /// in this commit keeps the ones it was put with.
    Result<void> set_metadata(std::string_view path, PutOptions options) {
        if (auto r = check_metadata(path, options); !r) return r;
        if (auto it = staged_.find(path); it != staged_.end()) {
            Staged& s = it->second;
            if (s.kind == Staged::Kind::put) {
                options.store = s.options.store;
                options.dictionary = s.options.dictionary;
                s.options = std::move(options);
                return {};
            }
            if (s.kind == Staged::Kind::edit) {
                s.options = std::move(options);
                s.metadata = true;
                return {};
            }
            return fail(Code::not_found, "no such entry", std::string(path));
        }
        if (!live_.contains(path)) return fail(Code::not_found, "no such entry (stage its content with put)", std::string(path));
        Staged s;
        s.kind = Staged::Kind::edit;
        s.source = std::string(path);
        s.options = std::move(options);
        s.metadata = true;
        staged_.emplace(std::string(path), std::move(s));
        return {};
    }

    /// Write bytes into an entry at `offset`, growing it if they run past
    /// its end, gaps reading as zeros. On a chunked entry (§4.5) only the
    /// chunks the ranges touch are written again; any number of ranges and
    /// resizes may be staged for one entry, and apply in the order given.
    /// An entry put in this commit (not with put_file) takes them at once.
    Result<void> put_range(std::string_view path, std::uint64_t offset, std::span<const std::byte> bytes) {
        auto s = editable(path, "put_range");
        if (!s) return std::unexpected(s.error());
        if (offset > options_.limits.max_entry_size || bytes.size() > options_.limits.max_entry_size - offset) {
            return fail(Code::limit, "the entry would grow past the size limit", std::string(path));
        }
        if ((*s)->kind == Staged::Kind::put) {
            auto& c = (*s)->content;
            if (offset + bytes.size() > c.size()) c.resize(static_cast<std::size_t>(offset + bytes.size()));
            std::ranges::copy(bytes, c.begin() + static_cast<std::ptrdiff_t>(offset));
            return {};
        }
        Staged::Op op;
        op.kind = Staged::Op::Kind::write;
        op.offset = offset;
        op.bytes.assign(bytes.begin(), bytes.end());
        (*s)->ops.push_back(std::move(op));
        return {};
    }

    /// put_range with its bytes in a file: `length` bytes of `file` from
    /// `file_offset`, read at commit a chunk at a time, never held whole (a
    /// Drive commits its journal so). The file must stay as it is until the
    /// commit.
    Result<void> put_range_file(std::string_view path, std::uint64_t offset, std::filesystem::path file, std::uint64_t file_offset,
                                std::uint64_t length) {
        std::error_code ec;
        const std::uint64_t have = std::filesystem::file_size(file, ec);
        if (ec) return fail(Code::io, std::format("cannot read {}: {}", file.string(), ec.message()), std::string(path));
        if (file_offset > have || length > have - file_offset) {
            return fail(Code::io, std::format("{} holds {} bytes, not {} from {}", file.string(), have, length, file_offset), std::string(path));
        }
        auto s = editable(path, "put_range_file");
        if (!s) return std::unexpected(s.error());
        if (offset > options_.limits.max_entry_size || length > options_.limits.max_entry_size - offset) {
            return fail(Code::limit, "the entry would grow past the size limit", std::string(path));
        }
        Staged::Op op;
        op.kind = Staged::Op::Kind::write;
        op.offset = offset;
        op.file = std::move(file);
        op.file_offset = file_offset;
        op.file_length = length;
        if ((*s)->kind == Staged::Kind::put) {
            // An entry put in this commit is in memory already: the bytes join it.
            auto& c = (*s)->content;
            if (offset + length > c.size()) c.resize(static_cast<std::size_t>(offset + length));
            return op.copy(0, std::span(c).subspan(static_cast<std::size_t>(offset), static_cast<std::size_t>(length)));
        }
        (*s)->ops.push_back(std::move(op));
        return {};
    }

    /// Make an entry `size` bytes long: cut, or grown with zeros. A chunked
    /// entry keeps every chunk below the cut as it is; any other entry is
    /// read and put again (it is at most 64 MiB).
    Result<void> resize(std::string_view path, std::uint64_t size) {
        if (size > options_.limits.max_entry_size) return fail(Code::limit, "past the entry size limit", std::string(path));
        auto s = editable(path, "resize");
        if (!s) return std::unexpected(s.error());
        if ((*s)->kind == Staged::Kind::put) {
            (*s)->content.resize(static_cast<std::size_t>(size));
            return {};
        }
        Staged::Op op;
        op.kind = Staged::Op::Kind::resize;
        op.offset = size;
        (*s)->ops.push_back(std::move(op));
        return {};
    }

    /// The size an entry will have once what is staged is committed.
    [[nodiscard]] Result<std::uint64_t> size(std::string_view path) const {
        if (auto it = staged_.find(path); it != staged_.end()) {
            const Staged& s = it->second;
            if (s.kind == Staged::Kind::remove || s.kind == Staged::Kind::forget) return fail(Code::not_found, "no such entry", std::string(path));
            if (s.kind == Staged::Kind::put) {
                if (!s.file) return std::uint64_t{s.content.size()};
                std::error_code ec;
                const auto n = std::filesystem::file_size(*s.file, ec);
                if (ec) return fail(Code::io, std::format("cannot read {}: {}", s.file->string(), ec.message()), std::string(path));
                return std::uint64_t{n};
            }
            return s.size_after(live_.at(s.source).size);
        }
        if (auto it = live_.find(path); it != live_.end()) return it->second.size;
        return fail(Code::not_found, "no such entry", std::string(path));
    }

    /// Drop everything staged, as if none of it had been asked for.
    void discard() noexcept { staged_.clear(); }

    /// Train a zstd dictionary on `samples` (many small, similar entries:
    /// JSON records, a manual's pages) and stage it as
    /// SQ-INF/dictionaries/<id>.zdict (§4.3). Its ID is derived from the
    /// samples, at least 32768 (below is zstd's registered range), so the
    /// same samples give the same ID. Pass the ID in PutOptions::dictionary.
    Result<std::uint32_t> train_dictionary(std::span<const std::span<const std::byte>> samples, std::size_t max_size = 16 * 1024) {
        std::vector<std::byte> joined;
        std::vector<std::size_t> sizes;
        Sha256 h;
        for (const auto& sample : samples) {
            joined.insert(joined.end(), sample.begin(), sample.end());
            sizes.push_back(sample.size());
            h.update(sample);
            std::array<std::byte, 8> n{};
            for (std::size_t i = 0; i < 8; ++i) n[i] = static_cast<std::byte>(std::uint64_t{sample.size()} >> (8 * i));
            h.update(n);
        }
        if (samples.size() < 8) return fail(Code::malformed, "too few samples to train a dictionary on (8 at least; hundreds is better)");
        const Digest d = h.finish();
        std::uint32_t id = 0;
        for (std::size_t i = 0; i < 4; ++i) id = id << 8 | std::to_integer<std::uint32_t>(d[i]);
        id = 32768 + id % (0x7fffffffu - 32768);
        while (live_.contains(std::format("SQ-INF/dictionaries/{}.zdict", id)) || dictionaries_.contains(id)) ++id;
        ZDICT_fastCover_params_t params{};
        params.k = 200;
        params.d = 8;
        params.f = 20;
        params.steps = 4;
        params.nbThreads = 0;
        params.accel = 1;
        params.zParams.compressionLevel = options_.level;
        params.zParams.notificationLevel = 0;
        params.zParams.dictID = id;
        std::vector<std::byte> dictionary(max_size);
        const std::size_t n = ZDICT_trainFromBuffer_fastCover(dictionary.data(), dictionary.size(), joined.data(), sizes.data(),
                                                              static_cast<unsigned>(sizes.size()), params);
        if (ZDICT_isError(n)) return fail(Code::malformed, std::format("zstd could not train a dictionary: {}", ZDICT_getErrorName(n)));
        dictionary.resize(n);
        if (ZDICT_getDictID(dictionary.data(), dictionary.size()) != id) return fail(Code::malformed, "zstd trained a dictionary with another ID");
        const std::string path = std::format("SQ-INF/dictionaries/{}.zdict", id);
        if (auto r = put(path, dictionary, {.flags = flag::dictionary, .store = Store::zstd}); !r) return std::unexpected(r.error());
        dictionaries_.emplace(id, std::move(dictionary));
        return id;
    }

    /// Sign the commits this writer makes (§9.3).
    void sign_with(SigningKey key) { keys_.push_back(std::move(key)); }

    /// The time the next commits record, in milliseconds since the epoch
    /// (§3.10); nullopt: SOURCE_DATE_EPOCH when set, else now. A caller
    /// that must know its commit after a crash (the Drive's journal) sets
    /// a time it wrote down first.
    void set_commit_time(std::optional<std::int64_t> ms) noexcept { options_.commit_time = ms; }

    [[nodiscard]] bool pending() const noexcept { return !staged_.empty(); }

    /// Commit what is staged (§10.3). Returns the new commit.
    Result<Commit> commit();

    /// Compact the cartridge (§10.4) under the lock this writer holds, and
    /// carry on writing to the compacted file. Nothing may be staged.
    Result<Commit> compact();

    Writer(Writer&&) noexcept = default;
    Writer& operator=(Writer&&) noexcept = default;

private:
    /// What one path will be after the commit.
    struct Staged {
        /// A write or a resize of an edited chunked entry, in the order asked.
        struct Op {
            enum class Kind : std::uint8_t { write, resize } kind = Kind::write;
            std::uint64_t offset = 0;  ///< where a write goes; a resize's new size
            std::vector<std::byte> bytes;
            /// A write from a file (put_range_file): `file_length` bytes at
            /// `file_offset`, read at commit.
            std::optional<std::filesystem::path> file;
            std::uint64_t file_offset = 0, file_length = 0;

            [[nodiscard]] std::uint64_t length() const noexcept { return file ? file_length : bytes.size(); }

            /// Bytes [at, at + out.size()) of what it writes.
            Result<void> copy(std::uint64_t at, std::span<std::byte> out) const {
                if (!file) {
                    std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(at), static_cast<std::ptrdiff_t>(out.size()), out.begin());
                    return {};
                }
                std::ifstream in(*file, std::ios::binary);
                in.seekg(static_cast<std::streamoff>(file_offset + at));
                in.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(out.size()));
                if (!in || static_cast<std::size_t>(in.gcount()) != out.size()) {
                    return fail(Code::io, std::format("cannot read {} bytes of {} at {} (did it change?)", out.size(), file->string(), file_offset + at));
                }
                return {};
            }
        };
        /// put: new content (`content`, or `file` read at commit) and `options`.
        /// remove: a tombstone. forget: nothing (it was never committed).
        /// edit: the live entry `source`, renamed to this path, its metadata
        /// replaced by `options` when `metadata`, and `ops` applied.
        enum class Kind : std::uint8_t { put, remove, forget, edit } kind = Kind::put;
        std::vector<std::byte> content;
        std::optional<std::filesystem::path> file;
        PutOptions options;
        bool metadata = false;
        std::string source;
        std::vector<Op> ops;

        /// The size after `ops`, from `base`.
        [[nodiscard]] std::uint64_t size_after(std::uint64_t base) const noexcept {
            for (const Op& op : ops) base = op.kind == Op::Kind::resize ? op.offset : std::max(base, op.offset + op.length());
            return base;
        }
    };

    Result<void> apply_ops(const Archive& before, const Entry& base, std::span<const Staged::Op> ops, std::span<const Chunk> old,
                           Entry& e, std::vector<Chunk>& out);
    Result<std::vector<Chunk>> put_streamed(const std::filesystem::path& source, Entry& e);

    /// Whether `path` is an entry once what is staged is committed.
    [[nodiscard]] bool exists(std::string_view path) const {
        if (auto it = staged_.find(path); it != staged_.end()) {
            return it->second.kind != Staged::Kind::remove && it->second.kind != Staged::Kind::forget;
        }
        return live_.contains(path);
    }

    /// An entry's metadata as PutOptions, to put it again as it was.
    static PutOptions options_of(const Entry& e) {
        PutOptions o;
        o.title = e.title, o.author = e.author, o.media_type = e.media_type, o.attributes = e.attributes;
        o.created = e.created, o.updated = e.updated;
        o.flags = e.flags & (flag::r | flag::w | flag::x | flag::mappable | flag::dictionary);
        o.store = (e.flags & flag::mappable) ? Store::stored : Store::automatic;
        o.dictionary = e.codec == Codec::zstd_dict ? e.dictionary : 0;
        return o;
    }

    /// The staged change that ranges and resizes of `path` go to: an edit
    /// of a chunked entry, or a put whose content is in memory. An entry
    /// that is not chunked is read and staged as a put of its content.
    Result<Staged*> editable(std::string_view path, std::string_view what) {
        auto it = staged_.find(path);
        if (it == staged_.end()) {
            if (!live_.contains(path)) return fail(Code::not_found, "no such entry", std::string(path));
            Staged s;
            s.kind = Staged::Kind::edit;
            s.source = std::string(path);
            it = staged_.emplace(std::string(path), std::move(s)).first;
        }
        Staged& s = it->second;
        if (s.kind == Staged::Kind::remove || s.kind == Staged::Kind::forget) return fail(Code::not_found, "no such entry", std::string(path));
        if (s.kind == Staged::Kind::put) {
            if (s.file) return fail(Code::malformed, std::format("{} of an entry put from a file: commit it first", what), std::string(path));
            return &s;
        }
        const Entry& base = live_.at(s.source);
        if (base.codec == Codec::chunked) return &s;
        // Not chunked: at most 64 MiB, so read it, and put it again.
        auto a = Archive::open(file_.path(), ReadOptions{options_.limits});
        if (!a) return std::unexpected(a.error());
        auto bytes = a->read(base);
        if (!bytes) return std::unexpected(bytes.error());
        Staged put;
        put.kind = Staged::Kind::put;
        put.content = std::move(*bytes);
        put.options = s.metadata ? s.options : options_of(base);
        if (s.metadata) put.options.dictionary = options_of(base).dictionary;
        for (const Staged::Op& op : s.ops) {
            if (op.kind == Staged::Op::Kind::resize) {
                put.content.resize(static_cast<std::size_t>(op.offset));
            } else {
                if (op.offset + op.length() > put.content.size()) put.content.resize(static_cast<std::size_t>(op.offset + op.length()));
                if (auto r = op.copy(0, std::span(put.content).subspan(static_cast<std::size_t>(op.offset), static_cast<std::size_t>(op.length()))); !r) {
                    return std::unexpected(r.error());
                }
            }
        }
        s = std::move(put);
        return &s;
    }

    Writer(Lock lock, Appender file, WriteOptions options)
        : lock_(std::move(lock)), file_(std::move(file)), options_(std::move(options)) {}

    /// The state a writer builds on: all of it checked (the reader's
    /// entries(), §3.5).
    Result<void> adopt(const Archive& a) {
        auto live = a.entries();
        if (!live) return std::unexpected(live.error());
        live_.clear();
        for (const Entry& e : *live) live_.emplace(e.path, e);
        current_ = a.commit();
        has_commit_ = true;
        return {};
    }

    /// A dictionary by ID: trained by this writer, or read from the cartridge.
    Result<const std::vector<std::byte>*> dictionary_bytes(std::uint32_t id) {
        if (auto it = dictionaries_.find(id); it != dictionaries_.end()) return &it->second;
        const std::string path = std::format("SQ-INF/dictionaries/{}.zdict", id);
        if (!live_.contains(path)) return fail(Code::not_found, std::format("no dictionary {}", id));
        auto a = Archive::open(file_.path(), ReadOptions{options_.limits});
        if (!a) return std::unexpected(a.error());
        auto e = a->find(path);
        if (!e) return std::unexpected(e.error());
        auto bytes = a->read(*e);
        if (!bytes) return std::unexpected(bytes.error());
        return &dictionaries_.emplace(id, std::move(*bytes)).first->second;
    }

    Result<void> check_metadata(std::string_view path, const PutOptions& o) const {
        if ((o.flags & ~(flag::r | flag::w | flag::x | flag::mappable | flag::dictionary)) != 0) {
            return fail(Code::malformed, "flags a writer may not set", std::string(path));
        }
        if (o.attributes) {
            auto canonical = canonical_json(*o.attributes, options_.limits.max_json_depth);
            if (!canonical) return std::unexpected(Error{canonical.error().code, canonical.error().message, std::string(path)});
            if (*canonical != *o.attributes) return fail(Code::json, "attributes must be stored in canonical (RFC 8785) form: " + *canonical, std::string(path));
        }
        constexpr std::int64_t exact = (std::int64_t{1} << 53) - 1;
        if (o.created < -exact || o.created > exact || o.updated < -exact || o.updated > exact) {
            return fail(Code::json, "a created or updated time beyond ±(2^53 − 1) ms", std::string(path));
        }
        return {};
    }

    Lock lock_;
    Appender file_;
    WriteOptions options_;
    std::vector<SigningKey> keys_;
    std::map<std::string, Entry, std::less<>> live_;
    std::map<std::string, Staged, std::less<>> staged_;
    std::map<std::uint32_t, std::vector<std::byte>> dictionaries_;  // by ID, trained or read
    Commit current_{};
    bool has_commit_ = false;
    bool sealed_ = false;  // this commit's record is written: no cutting back

    Result<Commit> commit_staged();

};

struct PackOptions {
    /// Leave out records in SQ-INF/entries.json for files that are not
    /// there (left by a rename outside the tools), rather than refuse (§6).
    bool drop_orphans = false;
    /// Top-level folders named with a dot to pack all the same, each by its
    /// name (".git", for a backup). Every other one is left out (§6). In a
    /// kept folder, a file the file system marks executable is flagged x.
    std::vector<std::string> keep;
    /// Paths in the folder, relative to it, left out with everything under
    /// them: what a consumer knows is made, not kept ("build", a workspace's
    /// output). Never in SQ-INF/, never with a "..".
    std::vector<std::string> leave_out;
};

/// Pack an exploded cartridge, `folder`, into a new cartridge, `out`, as one
/// commit (§6): its regular files, with the metadata SQ-INF/entries.json
/// gives them; never a symbolic link (refused, not followed), never the
/// top-level folder whose name begins with a dot (.git/, a tool's working
/// state) unless pack_options.keep names it, never what pack_options.leave_out
/// names, never entries.json itself.
Result<Commit> pack(const std::filesystem::path& folder, const std::filesystem::path& out, WriteOptions options = {},
                    PackOptions pack_options = {}, std::optional<SigningKey> key = std::nullopt);

/// Rewrite a cartridge as one commit with a full index, holding only its
/// live entries (§10.4): their stored bytes copied as they are, so the
/// content digest is unchanged and the current signatures still verify;
/// dictionaries no entry uses dropped; written beside it, checked, renamed
/// over it, the folder flushed.
Result<void> compact(const std::filesystem::path& path, WriteOptions options = {});

}  // namespace sq::cart

namespace sq::cart {

/// Everything one commit writes, laid out (§3.4, §3.10): runs in index
/// order, then the index, the strings in order of first use, the chunk
/// tables, the signature block, the record.
struct Layout {
    std::vector<Entry> records;                     // the index, sorted by path
    std::map<std::string, std::vector<std::byte>> chunk_tables;  // by path, offsets filled in
};

/// Where commit n+1 goes (§10.3): its sequence number is one more than the
/// higher of the two slots' numbers, valid or not, so none is used twice;
/// its slot is the one not naming the current commit.
struct NextSlot {
    std::uint64_t sequence = 1;
    std::uint64_t at = slot_a_at;
};

Result<NextSlot> next_slot(const Appender& file, const std::optional<Commit>& current) {
    if (!current) return NextSlot{};
    auto a = file.read(slot_a_at, 64);
    auto b = file.read(slot_b_at, 64);
    if (!a || !b) return std::unexpected(a ? b.error() : a.error());
    const auto seq = [](const std::vector<std::byte>& s) {
        std::uint64_t v = 0;
        for (int i = 7; i >= 0; --i) v = v << 8 | std::to_integer<std::uint64_t>(s[static_cast<std::size_t>(i)]);
        return v;
    };
    const auto names = [&](const std::vector<std::byte>& s) {
        std::uint64_t record = 0;
        for (int i = 15; i >= 8; --i) record = record << 8 | std::to_integer<std::uint64_t>(s[static_cast<std::size_t>(i)]);
        return seq(s) == current->sequence && record == current->offset;
    };
    NextSlot n;
    n.sequence = std::max(seq(*a), seq(*b)) + 1;
    n.at = names(*a) ? slot_b_at : slot_a_at;
    return n;
}

std::int64_t commit_time(const WriteOptions& o) {
    if (o.commit_time) return *o.commit_time;
    if (const char* epoch = std::getenv("SOURCE_DATE_EPOCH")) {
        std::int64_t s = 0;
        const std::string_view text(epoch);
        if (std::from_chars(text.data(), text.data() + text.size(), s).ec == std::errc{}) return s * 1000;
    }
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

/// Append the index, strings, chunk tables, signatures and record of a
/// commit whose runs are written, then point the spare slot at it.
Result<Commit> seal(Appender& file, const WriteOptions& options, std::vector<Entry> records,
                    std::map<std::string, std::vector<Chunk>, std::less<>> chunks, const std::map<std::string, Entry, std::less<>>& live,
                    std::uint64_t data_offset, const std::optional<Commit>& previous, bool full, std::span<const SigningKey> keys,
                    std::optional<std::vector<std::byte>> copied_signatures = std::nullopt) {
    std::ranges::sort(records, {}, &Entry::path);
    const std::uint64_t data_length = file.size() - data_offset;
    // Strings, in order of first use.
    std::vector<std::byte> strings;
    std::map<std::string, std::uint32_t, std::less<>> at;
    const auto str = [&](const std::optional<std::string>& s) -> std::uint32_t {
        if (!s) return 0xffffffffu;
        if (auto it = at.find(*s); it != at.end()) return it->second;
        const auto off = static_cast<std::uint32_t>(strings.size());
        std::array<std::byte, 4> len{};
        le(len, 0, s->size(), 4);
        strings.insert(strings.end(), len.begin(), len.end());
        const auto b = std::as_bytes(std::span(*s));
        strings.insert(strings.end(), b.begin(), b.end());
        at.emplace(*s, off);
        return off;
    };
    std::vector<std::array<std::uint32_t, 5>> refs;
    for (const Entry& e : records) refs.push_back({str(e.path), str(e.title), str(e.author), str(e.media_type), str(e.attributes)});
    if (auto r = file.pad_to(64); !r) return std::unexpected(r.error());
    const std::uint64_t index_offset = file.size();
    const std::uint64_t strings_offset = index_offset + records.size() * entry_record;
    const std::uint64_t chunks_offset = strings_offset + strings.size();
    // Chunk tables: one a chunked record whose chunks this commit lists.
    std::uint64_t chunk_at = chunks_offset;
    std::vector<std::byte> tables;
    for (Entry& e : records) {
        auto it = chunks.find(e.path);
        if (e.deleted() || e.codec != Codec::chunked || it == chunks.end()) continue;
        e.offset = chunk_at;
        e.stored_size = it->second.size() * chunk_record_bytes;
        for (const Chunk& c : it->second) {
            std::array<std::byte, chunk_record_bytes> rec{};
            le(rec, 0, c.offset, 8);
            le(rec, 8, c.stored_size, 4);
            le(rec, 12, c.size, 4);
            le(rec, 16, static_cast<std::uint16_t>(c.codec), 2);
            le(rec, 20, c.dictionary, 4);
            std::ranges::copy(c.sha256, rec.begin() + 32);
            tables.insert(tables.end(), rec.begin(), rec.end());
        }
        chunk_at += e.stored_size;
    }
    std::vector<std::byte> index;
    for (std::size_t i = 0; i < records.size(); ++i) {
        const Entry& e = records[i];
        std::array<std::byte, entry_record> r{};
        for (std::size_t k = 0; k < 5; ++k) le(r, 4 * k, refs[i][k], 4);
        le(r, 20, e.flags, 2);
        le(r, 22, static_cast<std::uint16_t>(e.codec), 2);
        le(r, 24, e.dictionary, 4);
        le(r, 32, e.offset, 8);
        le(r, 40, e.stored_size, 8);
        le(r, 48, e.size, 8);
        le(r, 56, static_cast<std::uint64_t>(e.created), 8);
        le(r, 64, static_cast<std::uint64_t>(e.updated), 8);
        std::ranges::copy(e.digest, r.begin() + 72);
        index.insert(index.end(), r.begin(), r.end());
    }
    for (const auto* part : {&index, &strings, &tables}) {
        if (auto r = file.append(*part); !r) return std::unexpected(r.error());
    }
    // The new state: its digest, features and manifest.
    std::vector<Entry> state;
    for (const auto& [path, e] : live) state.push_back(e);
    auto digest = content_digest(state);
    if (!digest) return std::unexpected(digest.error());
    std::uint64_t features = 0;
    for (const Entry& e : state) {
        if (e.codec == Codec::zstd_dict) features |= feature::zstd_dictionaries;
        if (e.codec == Codec::chunked) features |= feature::chunked;
    }
    if (options.window_log > 23) features |= feature::zstd_window_27;
    // Signatures (§9.3), over the new digest, with the manifest's id and version.
    std::vector<std::byte> block;
    if (copied_signatures) {
        block = std::move(*copied_signatures);
    } else if (!keys.empty()) {
        std::string id, version;
        if (auto m = live.find("SQ-INF/manifest.json"); m != live.end()) {
            auto bytes = file.read(m->second.offset, m->second.stored_size);
            // The manifest is stored (a writer stores it so, below).
            if (bytes) {
                auto parsed = data::parse_json(std::string_view(reinterpret_cast<const char*>(bytes->data()), bytes->size()));
                if (parsed) {
                    if (const auto* v = parsed.value.find("id"); v && v->string_if()) id = *v->string_if();
                    if (const auto* v = parsed.value.find("version"); v && v->string_if()) version = *v->string_if();
                }
            }
        }
        if (id.empty() || version.empty()) return fail(Code::malformed, "signing needs a manifest with an id and a version");
        const auto put = [&](std::uint64_t v, int n) {
            for (int i = 0; i < n; ++i) block.push_back(static_cast<std::byte>(v >> (8 * i)));
        };
        put(keys.size(), 4);
        for (const SigningKey& k : keys) {
            const auto sig = ed25519_sign(k.secret, signed_message(1, id, version, k.signer, *digest));
            put(1, 2);
            put(32, 2);
            block.insert(block.end(), k.public_key.begin(), k.public_key.end());
            const Digest kh = sha256(k.public_key);
            block.insert(block.end(), kh.begin(), kh.begin() + 8);
            put(k.signer.size(), 4);
            const auto s = std::as_bytes(std::span(k.signer));
            block.insert(block.end(), s.begin(), s.end());
            put(64, 2);
            block.insert(block.end(), sig.begin(), sig.end());
        }
    }
    const std::uint64_t signatures_offset = block.empty() ? 0 : file.size();
    if (!block.empty()) {
        if (auto r = file.append(block); !r) return std::unexpected(r.error());
    }
    // The record.
    auto slot_for = next_slot(file, previous);
    if (!slot_for) return std::unexpected(slot_for.error());
    Commit c;
    c.offset = file.size();
    c.sequence = slot_for->sequence;
    c.previous = previous ? previous->offset : 0;
    c.data_offset = data_offset, c.data_length = data_length;
    c.index_offset = index_offset, c.index_count = records.size();
    c.strings_offset = strings_offset, c.strings_length = strings.size();
    c.chunks_offset = tables.empty() ? 0 : chunks_offset, c.chunks_length = tables.size();
    c.index_kind = full ? IndexKind::full : IndexKind::delta;
    c.chain_length = full ? 0 : previous->chain_length + 1;
    c.live_count = state.size();
    c.time = commit_time(options);
    c.features = features;
    c.content_digest = *digest;
    c.signatures_offset = signatures_offset, c.signatures_length = block.size();
    std::array<std::byte, commit_record> rec{};
    std::ranges::copy(std::as_bytes(std::span(std::string_view("SQCOMMIT"))), rec.begin());
    le(rec, 8, c.sequence, 8);
    le(rec, 16, c.previous, 8);
    le(rec, 24, c.data_offset, 8);
    le(rec, 32, c.data_length, 8);
    le(rec, 40, c.index_offset, 8);
    le(rec, 48, c.index_count, 8);
    le(rec, 56, c.strings_offset, 8);
    le(rec, 64, c.strings_length, 8);
    le(rec, 72, c.chunks_offset, 8);
    le(rec, 80, c.chunks_length, 8);
    le(rec, 88, full ? 0 : 1, 4);
    le(rec, 92, c.chain_length, 4);
    le(rec, 96, c.live_count, 8);
    le(rec, 104, static_cast<std::uint64_t>(c.time), 8);
    le(rec, 112, c.features, 8);
    std::ranges::copy(c.content_digest, rec.begin() + 120);
    le(rec, 152, c.signatures_offset, 8);
    le(rec, 160, c.signatures_length, 8);
    const Digest seal_hash = sha256(std::span(rec).first(192));
    std::ranges::copy(seal_hash, rec.begin() + 192);
    if (auto r = file.append(rec); !r) return std::unexpected(r.error());
    if (file.on_fat() && file.size() > fat32_limit) {
        return fail(Code::limit, "the cartridge would pass 4 GiB on a FAT file system (§10.6): keep it on internal storage");
    }
    // Flush, point the spare slot at it, flush (§10.3).
    if (options.sync) {
        if (auto r = file.flush(); !r) return std::unexpected(r.error());
    }
    std::array<std::byte, 64> slot{};
    le(slot, 0, c.sequence, 8);
    le(slot, 8, c.offset, 8);
    le(slot, 16, commit_record, 8);
    le(slot, 24, c.offset + commit_record, 8);
    const Digest sh = sha256(std::span(slot).first(56));
    std::copy(sh.begin(), sh.begin() + 8, slot.begin() + 56);
    if (auto r = file.write_at(slot_for->at, slot); !r) return std::unexpected(r.error());
    if (options.sync) {
        if (auto r = file.flush(); !r) return std::unexpected(r.error());
    }
    return c;
}

/// One run: stored or zstd, and its record's fields.
Result<void> write_run(Appender& file, Entry& e, std::span<const std::byte> content, Store store, const WriteOptions& o,
                       std::uint32_t dictionary_id = 0, std::span<const std::byte> dictionary = {}) {
    e.size = content.size();
    e.digest = tree_hash(content);
    e.dictionary = 0;
    if (content.empty()) {
        e.codec = Codec::stored, e.offset = 0, e.stored_size = 0;
        return {};
    }
    if (dictionary_id != 0 && store != Store::stored && !(e.flags & flag::mappable)) {
        // Codec 2 when it shrinks the entry 5%, else what follows decides.
        auto f = zstd_dict_frame(content, dictionary, o.level);
        if (!f) return std::unexpected(f.error());
        if (store == Store::zstd || f->size() * 100 <= content.size() * 95) {
            if (auto r = file.pad_to(64); !r) return r;
            e.offset = file.size();
            e.codec = Codec::zstd_dict;
            e.dictionary = dictionary_id;
            e.stored_size = f->size();
            return file.append(*f);
        }
    }
    std::vector<std::byte> frame;
    bool use_zstd = store == Store::zstd;
    if (store == Store::automatic && !(e.flags & flag::mappable) && !compressed_already(e.media_type)) {
        auto f = zstd_frame(content, o.level, o.window_log);
        if (!f) return std::unexpected(f.error());
        // Stored unless it shrinks by at least 5% (§4.3).
        if (f->size() * 100 <= content.size() * 95) {
            frame = std::move(*f);
            use_zstd = true;
        }
    } else if (use_zstd) {
        auto f = zstd_frame(content, o.level, o.window_log);
        if (!f) return std::unexpected(f.error());
        frame = std::move(*f);
    }
    if (auto r = file.pad_to((e.flags & flag::mappable) ? mappable_align : 64); !r) return r;
    e.offset = file.size();
    e.codec = use_zstd ? Codec::zstd : Codec::stored;
    const std::span<const std::byte> run = use_zstd ? std::span<const std::byte>(frame) : content;
    e.stored_size = run.size();
    return file.append(run);
}

/// One chunk, at its own run: zstd when it shrinks 5%, else stored.
Result<Chunk> write_chunk(Appender& file, std::span<const std::byte> piece, const WriteOptions& o) {
    Chunk c;
    c.size = static_cast<std::uint32_t>(piece.size());
    c.sha256 = sha256(piece);
    auto f = zstd_frame(piece, o.level, o.window_log);
    if (!f) return std::unexpected(f.error());
    const bool use_zstd = f->size() * 100 <= piece.size() * 95;
    if (auto r = file.pad_to(64); !r) return std::unexpected(r.error());
    c.offset = file.size();
    c.codec = use_zstd ? Codec::zstd : Codec::stored;
    const std::span<const std::byte> run = use_zstd ? std::span<const std::byte>(*f) : piece;
    c.stored_size = static_cast<std::uint32_t>(run.size());
    if (auto r = file.append(run); !r) return std::unexpected(r.error());
    return c;
}

/// A chunked entry after its staged ops (§4.5): each chunk no write
/// touches, and no cut reaches, is named again; the rest are rebuilt from
/// the old bytes (verified) with the ops applied in order, and written.
/// A size of 0 leaves an empty stored entry.
Result<void> Writer::apply_ops(const Archive& before, const Entry& base, std::span<const Staged::Op> ops, std::span<const Chunk> old,
                               Entry& e, std::vector<Chunk>& out) {
    using Op = Staged::Op;
    std::uint64_t size = base.size, floor = base.size;  // floor: the lowest any cut reached
    for (const Op& op : ops) {
        if (op.kind == Op::Kind::resize) {
            size = op.offset;
            floor = std::min(floor, size);
        } else {
            size = std::max(size, op.offset + op.length());
        }
    }
    out.clear();
    e.size = size;
    e.dictionary = 0;
    if (size == 0) {
        e.codec = Codec::stored, e.offset = 0, e.stored_size = 0;
        e.digest = tree_hash(std::span<const std::byte>{});
        return {};
    }
    std::vector<Digest> leaves;
    for (std::uint64_t i = 0, n = (size + chunk_size - 1) / chunk_size; i < n; ++i) {
        const std::uint64_t lo = i * chunk_size, hi = std::min(size, lo + chunk_size);
        const bool had = i < old.size();
        bool touched = !had || old[i].size != hi - lo || floor < lo + old[i].size;
        for (const Op& op : ops) {
            if (op.kind == Op::Kind::write && op.offset < hi && op.offset + op.length() > lo) touched = true;
        }
        if (!touched) {
            out.push_back(old[i]);
            leaves.push_back(old[i].sha256);
            continue;
        }
        std::vector<std::byte> piece(static_cast<std::size_t>(hi - lo));
        if (const std::uint64_t keep = std::min({base.size, floor, hi}); lo < keep) {
            auto bytes = before.read_range(base, lo, keep - lo);
            if (!bytes) return std::unexpected(bytes.error());
            std::ranges::copy(*bytes, piece.begin());
        }
        for (const Op& op : ops) {
            if (op.kind == Op::Kind::resize) {
                if (op.offset < hi) std::fill(piece.begin() + static_cast<std::ptrdiff_t>(std::max(lo, op.offset) - lo), piece.end(), std::byte{0});
                continue;
            }
            const std::uint64_t from = std::max(lo, op.offset), to = std::min(hi, op.offset + op.length());
            if (from < to) {
                auto into = std::span(piece).subspan(static_cast<std::size_t>(from - lo), static_cast<std::size_t>(to - from));
                if (auto r = op.copy(from - op.offset, into); !r) return std::unexpected(Error{r.error().code, r.error().message, base.path});
            }
        }
        auto c = write_chunk(file_, piece, options_);
        if (!c) return std::unexpected(c.error());
        out.push_back(*c);
        leaves.push_back(c->sha256);
    }
    e.codec = Codec::chunked;
    e.digest = size <= chunk_size ? leaves.front() : tree_hash_from_leaves(leaves, size);
    return {};
}

/// A chunked entry read from a file 1 MiB at a time, never held whole.
Result<std::vector<Chunk>> Writer::put_streamed(const std::filesystem::path& source, Entry& e) {
    std::ifstream in(source, std::ios::binary);
    if (!in) return fail(Code::io, "cannot open " + source.string());
    std::vector<std::byte> piece(chunk_size);
    std::vector<Chunk> list;
    std::vector<Digest> leaves;
    std::uint64_t size = 0;
    for (;;) {
        in.read(reinterpret_cast<char*>(piece.data()), static_cast<std::streamsize>(piece.size()));
        const auto got = static_cast<std::size_t>(in.gcount());
        if (got == 0) break;
        auto c = write_chunk(file_, std::span(piece).first(got), options_);
        if (!c) return std::unexpected(c.error());
        list.push_back(*c);
        leaves.push_back(c->sha256);
        size += got;
        if (got < piece.size()) break;
    }
    if (in.bad()) return fail(Code::io, "cannot read " + source.string());
    if (size == 0) return fail(Code::malformed, "an empty entry cannot be chunked");
    if (size > options_.limits.max_entry_size) return fail(Code::limit, "past the entry size limit (did the file grow?)");
    e.codec = Codec::chunked;
    e.size = size;
    e.dictionary = 0;
    e.digest = size <= chunk_size ? leaves.front() : tree_hash_from_leaves(leaves, size);
    return list;
}

/// A commit that fails before its record is written leaves the file as it
/// was: what it appended is cut off again.
Result<Commit> Writer::commit() {
    const std::uint64_t start = file_.size();
    sealed_ = false;
    auto c = commit_staged();
    if (!c && !sealed_) static_cast<void>(file_.truncate(start));
    return c;
}

Result<Commit> Writer::commit_staged() {
    if (staged_.empty()) return fail(Code::malformed, "nothing staged to commit");
    // The live state after this commit, and the records this commit writes.
    auto live = live_;
    std::vector<Entry> records;
    std::map<std::string, std::vector<Chunk>, std::less<>> chunks;
    const std::uint64_t data_offset = (file_.size() + 63) / 64 * 64;
    if (auto r = file_.pad_to(64); !r) return std::unexpected(r.error());
    // The archive as it is, for reading chunks put_range keeps or changes.
    std::optional<Archive> before;
    for (auto& [path, s] : staged_) {
        if (s.kind == Staged::Kind::forget) continue;
        if (s.kind == Staged::Kind::remove) {
            Entry t;
            t.path = path;
            t.flags = flag::deleted;
            records.push_back(t);
            live.erase(path);
            continue;
        }
        if (s.kind == Staged::Kind::edit) {
            const Entry& base = live_.at(s.source);
            Entry e = base;
            e.path = path;
            if (s.metadata) {
                e.title = s.options.title, e.author = s.options.author, e.media_type = s.options.media_type;
                e.attributes = s.options.attributes, e.created = s.options.created, e.updated = s.options.updated;
                e.flags = s.options.flags;
            }
            if (base.codec == Codec::chunked) {
                if (!before) {
                    auto a = Archive::open(file_.path(), ReadOptions{options_.limits});
                    if (!a) return std::unexpected(a.error());
                    before = std::move(*a);
                }
                auto old = before->chunks(base);
                if (!old) return std::unexpected(old.error());
                if (s.ops.empty()) {
                    chunks.emplace(path, std::move(*old));  // named again, not written again (§3.6)
                } else if (auto r = apply_ops(*before, base, s.ops, *old, e, chunks[path]); !r) {
                    return std::unexpected(r.error());
                }
                if (e.codec != Codec::chunked) chunks.erase(path);
            }
            records.push_back(e);
            live.insert_or_assign(path, e);
            continue;
        }
        // A put.
        Entry e;
        e.path = path;
        const PutOptions& o = s.options;
        e.title = o.title, e.author = o.author, e.media_type = o.media_type, e.attributes = o.attributes;
        e.created = o.created, e.updated = o.updated, e.flags = o.flags;
        if (s.file) {
            std::error_code ec;
            const std::uint64_t n = std::filesystem::file_size(*s.file, ec);
            if (ec) return fail(Code::io, std::format("cannot read {}: {}", s.file->string(), ec.message()), path);
            if (n > options_.limits.max_entry_size) return fail(Code::limit, "past the entry size limit", path);
            const bool stream = o.store == Store::chunked ||
                                (o.store == Store::automatic && n > options_.limits.max_unchunked_size && !(o.flags & flag::mappable));
            if (stream) {
                auto list = put_streamed(*s.file, e);
                if (!list) return std::unexpected(Error{list.error().code, list.error().message, path});
                chunks.emplace(path, std::move(*list));
                records.push_back(e);
                live.insert_or_assign(path, e);
                continue;
            }
            // Small enough to hold: read it, and put it as any other.
            std::ifstream in(*s.file, std::ios::binary);
            s.content.resize(static_cast<std::size_t>(n));
            if (!in.read(reinterpret_cast<char*>(s.content.data()), static_cast<std::streamsize>(n))) {
                return fail(Code::io, "cannot read " + s.file->string(), path);
            }
            s.file.reset();
        }
        const bool chunk = o.store == Store::chunked ||
                           (o.store == Store::automatic && s.content.size() > options_.limits.max_unchunked_size && !(o.flags & flag::mappable));
        if (path == "SQ-INF/manifest.json") {
            // Stored, so a signer (and anyone) can read it without decoding.
            auto json = check_ijson(std::string_view(reinterpret_cast<const char*>(s.content.data()), s.content.size()));
            if (!json) return std::unexpected(Error{json.error().code, json.error().message, path});
            if (auto r = write_run(file_, e, s.content, Store::stored, options_); !r) return std::unexpected(r.error());
        } else if (chunk) {
            if (s.content.empty()) return fail(Code::malformed, "an empty entry cannot be chunked", path);
            e.codec = Codec::chunked;
            e.size = s.content.size();
            e.digest = tree_hash(s.content);
            std::vector<Chunk> list;
            for (std::size_t at = 0; at < s.content.size(); at += chunk_size) {
                auto c = write_chunk(file_, std::span(s.content).subspan(at, std::min<std::size_t>(chunk_size, s.content.size() - at)), options_);
                if (!c) return std::unexpected(c.error());
                list.push_back(*c);
            }
            chunks.emplace(path, std::move(list));
        } else {
            if (s.content.size() > options_.limits.max_unchunked_size) return fail(Code::limit, "past 64 MiB an entry must be chunked", path);
            std::span<const std::byte> dictionary;
            if (o.dictionary != 0) {
                auto d = dictionary_bytes(o.dictionary);
                if (!d) return std::unexpected(Error{d.error().code, d.error().message, path});
                dictionary = **d;
            }
            if (auto r = write_run(file_, e, s.content, o.store, options_, o.dictionary, dictionary); !r) return std::unexpected(r.error());
        }
        records.push_back(e);
        live.insert_or_assign(path, e);
    }
    if (!live.contains("SQ-INF/manifest.json")) return fail(Code::malformed, "a cartridge needs SQ-INF/manifest.json (§4.1)");
    // Case collisions (§4.1), before anything is sealed; none to look for
    // when the manifest committed says case_sensitive.
    bool case_sensitive = false;
    if (auto m = staged_.find("SQ-INF/manifest.json"); m != staged_.end() && m->second.kind == Staged::Kind::put) {
        auto parsed = data::parse_json(std::string_view(reinterpret_cast<const char*>(m->second.content.data()), m->second.content.size()));
        if (const data::JsonValue* cs = parsed ? parsed.value.find("case_sensitive") : nullptr; cs && cs->boolean_if()) case_sensitive = *cs->boolean_if();
    } else if (has_commit_) {
        if (!before) {
            auto a = Archive::open(file_.path(), ReadOptions{options_.limits});
            if (!a) return std::unexpected(a.error());
            before = std::move(*a);
        }
        case_sensitive = before->manifest().case_sensitive;
    }
    std::map<std::u32string, std::string_view> keys;
    for (const auto& [path, e] : live) {
        if (case_sensitive) break;
        auto [it, fresh] = keys.try_emplace(collision_key(path), path);
        if (!fresh) return fail(Code::path, std::format("collides with \"{}\" on a case-insensitive file system", it->second), path);
    }
    // A full index first, and before the chain would pass its limit; else a delta.
    const bool full = !has_commit_ || current_.chain_length + 1 > options_.limits.max_delta_chain;
    if (full) {
        records.clear();
        for (const auto& [path, e] : live) records.push_back(e);
        for (const auto& [path, e] : live) {
            if (e.codec == Codec::chunked && !chunks.contains(path)) {
                if (!before) {
                    auto a = Archive::open(file_.path(), ReadOptions{options_.limits});
                    if (!a) return std::unexpected(a.error());
                    before = std::move(*a);
                }
                auto old = before->chunks(e);
                if (!old) return std::unexpected(old.error());
                chunks.emplace(path, std::move(*old));
            }
        }
    }
    sealed_ = true;  // from here a slot may name it: a failure keeps what was written
    auto c = seal(file_, options_, std::move(records), std::move(chunks), live, data_offset,
                  has_commit_ ? std::optional<Commit>(current_) : std::nullopt, full, keys_);
    if (!c) return std::unexpected(c.error());
    // Read back what was written, as any reader would: the state must open.
    auto check = Archive::open(file_.path(), ReadOptions{options_.limits});
    if (!check) return std::unexpected(Error{check.error().code, "the commit does not read back: " + check.error().message, check.error().entry});
    if (auto r = adopt(*check); !r) return std::unexpected(r.error());
    staged_.clear();
    return *c;
}


namespace {

std::optional<std::vector<std::byte>> read_file(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) return std::nullopt;
    std::vector<char> text{std::istreambuf_iterator<char>(in), {}};
    const auto b = std::as_bytes(std::span(text));
    return std::vector<std::byte>(b.begin(), b.end());
}

/// One record of SQ-INF/entries.json as PutOptions.
Result<PutOptions> exploded_options(const data::JsonValue& v, const std::string& path) {
    auto e = exploded_record(v, path);
    if (!e) return std::unexpected(e.error());
    PutOptions o;
    o.title = std::move(e->title);
    o.author = std::move(e->author);
    o.media_type = std::move(e->media_type);
    o.attributes = std::move(e->attributes);
    o.created = e->created;
    o.updated = e->updated;
    o.flags = e->flags;
    if (o.flags & flag::mappable) o.store = Store::stored;
    return o;
}

}  // namespace

Result<Commit> pack(const std::filesystem::path& folder, const std::filesystem::path& out, WriteOptions options,
                    PackOptions pack_options, std::optional<SigningKey> key) {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::is_regular_file(fs::symlink_status(folder / "SQ-INF" / "manifest.json", ec))) {
        return fail(Code::malformed, folder.string() + " is not a cartridge: no SQ-INF/manifest.json");
    }
    // The metadata the file system cannot hold.
    std::map<std::string, PutOptions, std::less<>> metadata;
    if (const auto ej = folder / "SQ-INF" / "entries.json"; fs::exists(fs::symlink_status(ej, ec))) {
        if (!fs::is_regular_file(fs::symlink_status(ej, ec))) return fail(Code::path, "SQ-INF/entries.json is not a regular file");
        auto bytes = read_file(ej);
        if (!bytes) return fail(Code::io, "cannot read SQ-INF/entries.json");
        auto parsed = data::parse_json(std::string_view(reinterpret_cast<const char*>(bytes->data()), bytes->size()),
                                       {.maximum_depth = options.limits.max_json_depth, .ijson = true});
        if (!parsed) return fail(Code::json, "SQ-INF/entries.json: " + parsed.error.message);
        if (parsed.value.object_if() == nullptr) return fail(Code::json, "SQ-INF/entries.json is not an object");
        for (const auto& [path, v] : *parsed.value.object_if()) {
            auto o = exploded_options(v, path);
            if (!o) return std::unexpected(o.error());
            if (!valid_utf8(path)) return fail(Code::path, "a name in SQ-INF/entries.json that is not UTF-8");
            metadata.emplace(nfc(path), std::move(*o));
        }
    }
    // The kept dot folders: each a name at the top, and there.
    for (const std::string& k : pack_options.keep) {
        if (!k.starts_with(".") || k == "." || k == ".." || k.find('/') != std::string::npos)
            return fail(Code::path, "keep: a top-level folder named with a dot (.git), not " + k);
        if (!fs::is_directory(fs::symlink_status(folder / k, ec))) return fail(Code::path, "keep: no folder " + k + " at the top");
    }
    // What is left out: relative paths in the folder, and not SQ-INF.
    for (const std::string& l : pack_options.leave_out) {
        const fs::path lp(l);
        if (l.empty() || lp.is_absolute() || std::ranges::any_of(lp, [](const fs::path& part) { return part == ".."; }) ||
            l == "SQ-INF" || l.starts_with("SQ-INF/"))
            return fail(Code::path, "leave_out: a path in the folder, outside SQ-INF, not " + l);
    }
    auto left_out = [&](std::string_view name) {
        return std::ranges::any_of(pack_options.leave_out, [&](const std::string& l) {
            const std::string_view p = std::string_view(l).ends_with('/') ? std::string_view(l).substr(0, l.size() - 1) : std::string_view(l);
            return name == p || (name.starts_with(p) && name.size() > p.size() && name[p.size()] == '/');
        });
    };
    auto kept = [&](std::string_view name) {
        return std::ranges::any_of(pack_options.keep, [&](const std::string& k) { return name == k || name.starts_with(k + "/"); });
    };
    // The files, and which a kept folder marks executable.
    std::vector<std::pair<std::string, fs::path>> files;
    std::set<std::string, std::less<>> executable;
    fs::recursive_directory_iterator it(folder, fs::directory_options::none, ec), end;
    if (ec) return fail(Code::io, std::format("cannot read {}: {}", folder.string(), ec.message()));
    for (; it != end; it.increment(ec)) {
        if (ec) return fail(Code::io, std::format("cannot read {}: {}", folder.string(), ec.message()));
        const fs::path rel = it->path().lexically_relative(folder);
        std::string name = rel.generic_string();
        // Names as the file system gives them; macOS decomposes them (NFD),
        // and every path is NFC (§4.1, §6).
        if (!valid_utf8(name)) return fail(Code::path, "a name that is not UTF-8", name);
        name = nfc(name);
        const auto status = it->symlink_status(ec);
        if (left_out(name)) {  // before the link check: a left-out link is not looked at
            if (fs::is_directory(status)) it.disable_recursion_pending();
            continue;
        }
        if (fs::is_symlink(status)) return fail(Code::path, "a symbolic link: refused, not followed (§6)", name);
        // A top-level folder named with a dot: a tool's working state
        // (.git/, and any consumer's), left out unless kept by name (§6).
        if (it.depth() == 0 && fs::is_directory(status) && name.starts_with(".") && !kept(name)) {
            it.disable_recursion_pending();
            continue;
        }
        if (!fs::is_regular_file(status) || name == "SQ-INF/entries.json") continue;
        if (!files.empty() && std::ranges::any_of(files, [&](const auto& f) { return f.first == name; })) {
            return fail(Code::path, "two names in the folder that are one in NFC", name);
        }
        files.emplace_back(name, it->path());
        if (kept(name) && (status.permissions() & fs::perms::owner_exec) != fs::perms::none) executable.insert(name);
    }
    std::ranges::sort(files);
    std::set<std::string, std::less<>> present;
    for (const auto& [name, p] : files) present.insert(name);
    for (const auto& [path, o] : metadata) {
        if (!present.contains(path) && !pack_options.drop_orphans) {
            return fail(Code::malformed, "a record in SQ-INF/entries.json for a file that is not there (an orphan, §6)", path);
        }
    }
    auto w = Writer::create(out, options);
    if (!w) return std::unexpected(w.error());
    if (key) w->sign_with(std::move(*key));
    for (const auto& [name, p] : files) {
        PutOptions o;
        if (auto m = metadata.find(name); m != metadata.end()) o = m->second;
        else if (executable.contains(name)) o.flags = flag::r | flag::x;
        // Read at the commit, 1 MiB at a time: a file is never held whole
        // (chunked past 64 MiB), so a large asset packs on a phone too.
        if (auto r = w->put_file(name, p, o); !r) return std::unexpected(r.error());
    }
    return w->commit();
}

namespace {

/// compact(), its lock already held.
Result<Commit> compact_locked(const std::filesystem::path& path, const WriteOptions& options) {
    auto old = Archive::open(path, ReadOptions{options.limits});
    if (!old) return std::unexpected(old.error());
    auto source = Appender::open(path, false);
    if (!source) return std::unexpected(source.error());
    auto temporary = path;
    temporary += ".compact";
    std::error_code ignore;
    std::filesystem::remove(temporary, ignore);
    auto out = Appender::open(temporary, true);
    if (!out) return std::unexpected(out.error());
    std::vector<std::byte> header(header_bytes);
    for (std::size_t i = 0; i < magic.size(); ++i) header[i] = static_cast<std::byte>(magic[i]);
    le(header, 8, major_version, 2);
    le(header, 12, header_bytes, 4);
    if (auto r = out->append(header); !r) return std::unexpected(r.error());
    // Dictionaries no live entry uses are dropped.
    std::set<std::uint32_t> used;
    auto old_live = old->entries();
    if (!old_live) return std::unexpected(old_live.error());
    for (const Entry& e : *old_live) {
        if (e.codec == Codec::zstd_dict) used.insert(e.dictionary);
        if (e.codec == Codec::chunked) {
            auto cs = old->chunks(e);
            if (!cs) return std::unexpected(cs.error());
            for (const Chunk& c : *cs) {
                if (c.codec == Codec::zstd_dict) used.insert(c.dictionary);
            }
        }
    }
    const std::uint64_t data_offset = out->size();
    std::map<std::string, Entry, std::less<>> live;
    std::map<std::string, std::vector<Chunk>, std::less<>> chunks;
    const auto copy_run = [&](std::uint64_t at, std::uint64_t n, std::uint64_t align) -> Result<std::uint64_t> {
        if (n == 0) return std::uint64_t{0};
        if (auto r = out->pad_to(align); !r) return std::unexpected(r.error());
        auto bytes = source->read(at, n);
        if (!bytes) return std::unexpected(bytes.error());
        const std::uint64_t where = out->size();
        if (auto r = out->append(*bytes); !r) return std::unexpected(r.error());
        return where;
    };
    for (Entry e : *old_live) {
        if (e.flags & flag::dictionary) {
            const std::string_view name = std::string_view(e.path).substr(20);  // SQ-INF/dictionaries/
            std::uint32_t id = 0;
            std::from_chars(name.data(), name.data() + name.size(), id);
            if (!used.contains(id)) continue;
        }
        if (e.codec == Codec::chunked) {
            auto cs = old->chunks(e);
            if (!cs) return std::unexpected(cs.error());
            for (Chunk& c : *cs) {
                auto where = copy_run(c.offset, c.stored_size, 64);
                if (!where) return std::unexpected(where.error());
                c.offset = *where;
            }
            chunks.emplace(e.path, std::move(*cs));
        } else {
            auto where = copy_run(e.offset, e.stored_size, (e.flags & flag::mappable) ? mappable_align : 64);
            if (!where) return std::unexpected(where.error());
            e.offset = *where;
        }
        live.emplace(e.path, e);
    }
    std::vector<Entry> records;
    for (const auto& [p, e] : live) records.push_back(e);
    // The signature block, copied: the digest is unchanged, unless a
    // dictionary was dropped (then the state changed, and it is not).
    std::optional<std::vector<std::byte>> signatures;
    const bool same_state = live.size() == old_live->size();
    if (same_state && old->commit().signatures_length != 0) {
        auto block = source->read(old->commit().signatures_offset, old->commit().signatures_length);
        if (!block) return std::unexpected(block.error());
        signatures = std::move(*block);
    }
    auto c = seal(*out, options, std::move(records), std::move(chunks), live, data_offset, std::nullopt, true, {}, std::move(signatures));
    if (!c) return std::unexpected(c.error());
    // Check it (§10.4), then rename it over the old file and flush the folder.
    auto check = Archive::open(temporary, ReadOptions{options.limits});
    if (!check) return fail(Code::malformed, "the compacted file does not open: " + check.error().message);
    if (same_state && c->content_digest != old->commit().content_digest) {
        return fail(Code::integrity, "compaction changed the content digest");
    }
    std::filesystem::rename(temporary, path, ignore);
    if (ignore) return fail(Code::io, std::format("cannot rename {} over {}: {}", temporary.string(), path.string(), ignore.message()));
    if (options.sync) {
        if (auto r = flush_folder(path); !r) return std::unexpected(r.error());
    }
    return *c;
}

}  // namespace

Result<void> compact(const std::filesystem::path& path, WriteOptions options) {
    auto lock = Lock::take(path, options.lock);
    if (!lock) return std::unexpected(lock.error());
    auto c = compact_locked(path, options);
    if (!c) return std::unexpected(c.error());
    return {};
}

Result<Commit> Writer::compact() {
    if (!staged_.empty()) return fail(Code::malformed, "commit or discard what is staged before compacting");
    const std::filesystem::path path = file_.path();
    auto c = compact_locked(path, options_);
    if (!c) return std::unexpected(c.error());
    // The compacted file is a new one at the same path: write to it from now on.
    auto f = Appender::open(path, false);
    if (!f) return std::unexpected(f.error());
    file_ = std::move(*f);
    auto a = Archive::open(path, ReadOptions{options_.limits});
    if (!a) return std::unexpected(a.error());
    if (auto r = adopt(*a); !r) return std::unexpected(r.error());
    return *c;
}

}  // namespace sq::cart
