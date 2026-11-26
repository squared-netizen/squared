// SPDX-License-Identifier: MIT
//
// sq.drive — the Virtual Cartridge Drive: the big-file interface for
// squared apps. Mount a 3.0 cartridge, open its entries as files, and read
// and write them by range. sqcart (sq.cart, sq.cart.write) is the only
// thing that writes the format; the Drive owns staging, caching, commit
// policy and recovery ("Virtual Cartridge Drive: design").
//
// Reads go through a cache of decoded chunks. Writes change 1 MiB chunks in
// memory; sync() appends them to the journal in the Drive's own storage and
// flushes it, so they survive a crash, and they leave memory: a synced chunk
// is read back from the journal. A commit freezes what is staged, starts a
// new journal segment for the writes that go on meanwhile, and hands the
// frozen changes to sqcart as one commit, on a thread of its own when the
// policy commits by budget. At mount, segments a landed commit covered are
// dropped and the rest replayed; a cartridge moved on by another writer is
// a conflict (Code::conflict).

module;

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

export module sq.drive;

import std;
import sq.cart;
import sq.cart.write;

export namespace sq::drive {

using sq::cart::Code;
using sq::cart::Entry;
using sq::cart::Error;
using sq::cart::Result;

/// How a cartridge is mounted.
enum class Access : std::uint8_t {
    read_only,   ///< no lock, no journal; sees the cartridge as it was when mounted
    read_write,  ///< holds the writer lock (§10.2) until unmount; a journal; commits
};

/// When a read-write Drive commits on its own.
enum class Policy : std::uint8_t {
    budget,         ///< past commit_bytes of changes, or commit_after since the first, and at unmount
    explicit_only,  ///< only on commit(); unmount syncs, and the journal waits for the next mount
    on_unmount,     ///< on commit() and at unmount
};

struct MountOptions {
    Access access = Access::read_only;
    /// Decoded chunks kept in memory, in bytes. 0 keeps none.
    std::size_t cache_bytes = std::size_t{64} << 20;
    /// Where journals live, one folder per cartridge. Empty: $XDG_STATE_HOME/sqdrive,
    /// else ~/.local/state/sqdrive. An Android app passes its files directory.
    std::filesystem::path staging;
    Policy commit = Policy::budget;
    std::uint64_t commit_bytes = std::uint64_t{64} << 20;
    std::chrono::milliseconds commit_after{30'000};
    /// Changes held at most, in bytes: past it, a write commits first
    /// (budget) or is refused with Code::limit.
    std::uint64_t staging_bytes = std::uint64_t{1} << 30;
    /// Changes held in memory at most, in bytes: past it, a write syncs
    /// them to the journal first, and they are read back from there.
    std::uint64_t memory_bytes = std::uint64_t{32} << 20;
    /// After a commit with nothing else staged, compact the cartridge
    /// (§10.4) when at least compact_min_bytes and compact_at of it are
    /// unreachable. Writers wait meanwhile; readers do not. compact_at 0: never.
    double compact_at = 0.5;
    std::uint64_t compact_min_bytes = std::uint64_t{64} << 20;
    /// A journal the cartridge has moved past (another writer committed
    /// after a crash): set it aside as journal.conflict-<time>.<n> and start
    /// again, rather than refuse the mount with Code::conflict.
    bool discard_journal = false;
    sq::cart::ReadOptions read{};
    /// How commits are written (level, sync, lock); its limits are the reader's.
    sq::cart::WriteOptions write{};
};

/// How a file is opened: mode::read, or mode::write with create and truncate.
namespace mode {
inline constexpr unsigned read = 0, write = 1, create = 2, truncate = 4;
}  // namespace mode

/// What the cache has done since the mount.
struct CacheStats {
    std::uint64_t hits = 0, misses = 0, evictions = 0;
    std::size_t bytes = 0, units = 0;  ///< held now
};

class Drive;

/// An open file: a path on the Drive. Reads and writes go to whatever is
/// at that path now (after a rename, the old path is gone).
class File {
public:
    [[nodiscard]] const std::string& path() const noexcept { return path_; }
    /// Its size now, staged changes included; 0 if it is gone.
    [[nodiscard]] std::uint64_t size() const;
    /// Its record, as a commit would write it (size and all).
    [[nodiscard]] Result<Entry> stat() const;

    /// Read into `out` from `offset`: as many bytes as fit and the file
    /// holds there (0 at or past its end). Every committed byte is verified.
    Result<std::size_t> read(std::uint64_t offset, std::span<std::byte> out) const;
    /// Up to `n` bytes from `offset`, as a new vector.
    Result<std::vector<std::byte>> read(std::uint64_t offset, std::size_t n) const {
        const std::uint64_t s = size();
        std::vector<std::byte> out(static_cast<std::size_t>(std::min<std::uint64_t>(n, offset < s ? s - offset : 0)));
        auto got = read(offset, std::span(out));
        if (!got) return std::unexpected(got.error());
        out.resize(*got);
        return out;
    }
    /// Write `bytes` at `offset`, growing the file (gaps read as zeros).
    Result<void> write(std::uint64_t offset, std::span<const std::byte> bytes);
    /// Cut it, or grow it with zeros.
    Result<void> resize(std::uint64_t size);
    /// Make every change so far survive a crash (the whole Drive's: one journal).
    Result<void> sync();

private:
    friend class Drive;
    struct State;
    File(std::shared_ptr<State> state, std::string path, std::optional<Entry> entry)
        : state_(std::move(state)), path_(std::move(path)), entry_(std::move(entry)) {}
    std::shared_ptr<State> state_;
    std::string path_;
    std::optional<Entry> entry_;  // a read-only mount's record, kept
};

/// A mounted cartridge. Thread-safe: any number of threads may use one
/// Drive and its Files, and Files outlive the Drive they came from (a read
/// still works after unmount; a write does not).
class Drive {
public:
    [[nodiscard]] static Result<Drive> mount(const std::filesystem::path& cartridge, MountOptions options = {});
    Drive(Drive&&) noexcept = default;
    Drive& operator=(Drive&&) noexcept = default;
    /// Unmounts, as unmount() does, if that was not done.
    ~Drive();

    /// A file, by its path in the cartridge. mode::create makes an empty
    /// one where there is none; mode::truncate empties it.
    [[nodiscard]] Result<File> open(std::string_view path, unsigned how = mode::read) const;
    /// A file's record, as a commit would write it.
    [[nodiscard]] Result<Entry> stat(std::string_view path) const;
    /// Every file whose path starts with `prefix`, sorted by path.
    [[nodiscard]] Result<std::vector<Entry>> list(std::string_view prefix = {}) const;

    Result<void> remove(std::string_view path);
    /// Give a file a new path, replacing any file there. Its content is
    /// not copied: the commit names the same runs (§3.6).
    Result<void> rename(std::string_view from, std::string_view to);
    /// Make every change so far survive a crash.
    Result<void> sync();
    /// Commit every change as one sqcart commit (after any commit running
    /// in the background); the current commit when there is nothing to commit.
    Result<sq::cart::Commit> commit();
    /// Wait until no commit is running or asked for; the error of a
    /// background commit that failed since the last call, if one did.
    Result<void> wait();
    /// Commit or sync as the policy says, release the lock, end writing.
    Result<void> unmount();

    /// The commit the Drive reads, without what is staged.
    [[nodiscard]] sq::cart::Commit head() const;
    [[nodiscard]] const std::filesystem::path& path() const noexcept;
    /// Bytes of changes held, synced or not, not yet committed.
    [[nodiscard]] std::uint64_t staged_bytes() const;
    /// Bytes of changes held in memory (not yet synced).
    [[nodiscard]] std::uint64_t memory_bytes() const;
    /// The journal segment a read-write mount writes to now.
    [[nodiscard]] std::filesystem::path journal_path() const;
    [[nodiscard]] CacheStats cache_stats() const;
    /// Drop every cached chunk (a cold cache, for measuring).
    void drop_cache();

private:
    explicit Drive(std::shared_ptr<File::State> state) : state_(std::move(state)) {}
    std::shared_ptr<File::State> state_;
};

/// How `pack` makes a cartridge of a folder.
struct PackOptions {
    /// Top-level folders named with a dot to pack all the same (".git", for
    /// one's own backup); every other one is a tool's working state, left out.
    std::vector<std::string> keep;
    /// Paths in the folder left out with everything under them: what is
    /// made, not kept (a workspace's "build").
    std::vector<std::string> leave_out;
    /// How the cartridge is written (level, sync, lock).
    sq::cart::WriteOptions write{};
};

/// A folder holding SQ-INF/manifest.json (an exploded cartridge) packed into
/// a new cartridge, `cartridge`, as one commit: sqcart's exploded form (its
/// SQ-INF/entries.json metadata, names in NFC, links refused, dot folders
/// left out), each file read at the commit a MiB at a time, so no file is
/// held whole whatever its size. Refuses a cartridge that is there already.
Result<sq::cart::Commit> pack(const std::filesystem::path& folder, const std::filesystem::path& cartridge,
                              PackOptions options = {});

/// How `explode` writes a cartridge out as a folder.
struct ExplodeOptions {
    /// Write into a folder that has files already (never over one).
    bool merge = false;
    /// Make an entry flagged x executable: for one's own backup only; a
    /// cartridge from elsewhere never puts a program on the disk unasked.
    bool executable = false;
    sq::cart::ReadOptions read{};
};

/// A cartridge written out as a folder (sqcart's exploded form): every live
/// entry a file, each read and verified before it lands (a chunked one a
/// chunk at a time, through NAME.part), SQ-INF/entries.json for what a file
/// system cannot hold. The number of files written.
Result<std::size_t> explode(const std::filesystem::path& cartridge, const std::filesystem::path& folder,
                            ExplodeOptions options = {});

}  // namespace sq::drive

namespace sq::drive {

namespace fs = std::filesystem;
using sq::cart::chunk_size;

[[nodiscard]] std::unexpected<Error> fail(Code code, std::string message, std::optional<std::string> entry = std::nullopt) {
    return std::unexpected(Error{code, std::move(message), std::move(entry)});
}

std::string errno_text(std::string_view what, const fs::path& path) { return std::format("{} {}: {}", what, path.string(), std::strerror(errno)); }

/// Where a test can make the process stop dead (SQDRIVE_CRASH=<point>), as
/// a crash would: before_commit (the journal says a commit is coming) and
/// after_commit (it landed; the journal does not know yet). Tests only.
void crash_point(std::string_view where) {
    const char* const at = std::getenv("SQDRIVE_CRASH");  // each time: a test sets it in a child
    if (at == nullptr || where != at) return;
    // SQDRIVE_CRASH_DELAY_MS: stop only after that long, so that a test can
    // write on meanwhile, into the next segment.
    if (const char* delay = std::getenv("SQDRIVE_CRASH_DELAY_MS")) std::this_thread::sleep_for(std::chrono::milliseconds(std::atoi(delay)));
    std::_Exit(86);
}

std::int64_t now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

// ---------------------------------------------------------------- the cache

/// One decoded unit: a chunk of a chunked entry (§4.5), or the whole of
/// any other (at most 64 MiB). Keyed by the entry's digest and the chunk's
/// index, so an unchanged chunk keeps its place across commits and paths.
struct Key {
    sq::cart::Digest digest{};
    std::uint64_t index = 0;
    friend bool operator==(const Key&, const Key&) = default;
};

struct KeyHash {
    std::size_t operator()(const Key& k) const noexcept {
        std::size_t h = 0;
        for (std::size_t i = 0; i < sizeof(std::size_t); ++i) h = h << 8 | std::to_integer<std::size_t>(k.digest[i]);
        return h ^ (std::hash<std::uint64_t>{}(k.index) * 0x9e3779b97f4a7c15ull);
    }
};

/// A least-recently-used cache of decoded units, bounded in bytes. A unit
/// larger than the whole budget is read but not kept.
class Cache {
public:
    using Unit = std::shared_ptr<const std::vector<std::byte>>;

    explicit Cache(std::size_t budget) : budget_(budget) {}

    Unit find(const Key& k) {
        std::lock_guard lock(mutex_);
        auto it = map_.find(k);
        if (it == map_.end()) {
            ++stats_.misses;
            return nullptr;
        }
        ++stats_.hits;
        order_.splice(order_.begin(), order_, it->second);
        return it->second->second;
    }

    void put(const Key& k, Unit unit) {
        std::lock_guard lock(mutex_);
        if (unit->size() > budget_ || map_.contains(k)) return;
        order_.emplace_front(k, std::move(unit));
        map_.emplace(k, order_.begin());
        stats_.bytes += order_.front().second->size();
        while (stats_.bytes > budget_) {
            auto& [key, victim] = order_.back();
            stats_.bytes -= victim->size();
            map_.erase(key);
            order_.pop_back();
            ++stats_.evictions;
        }
    }

    void clear() {
        std::lock_guard lock(mutex_);
        order_.clear();
        map_.clear();
        stats_.bytes = 0;
    }

    CacheStats stats() const {
        std::lock_guard lock(mutex_);
        CacheStats s = stats_;
        s.units = map_.size();
        return s;
    }

private:
    std::size_t budget_;
    mutable std::mutex mutex_;
    std::list<std::pair<Key, Unit>> order_;  // most recent first
    std::unordered_map<Key, std::list<std::pair<Key, Unit>>::iterator, KeyHash> map_;
    CacheStats stats_;
};


// -------------------------------------------------------------- the journal

/// CRC-32C (Castagnoli), as iSCSI and ext4 use it.
constexpr auto crc_table = [] {
    std::array<std::uint32_t, 256> t{};
    for (std::uint32_t i = 0; i < 256; ++i) {
        std::uint32_t c = i;
        for (int k = 0; k < 8; ++k) c = (c & 1) ? 0x82f63b78u ^ (c >> 1) : c >> 1;
        t[i] = c;
    }
    return t;
}();

std::uint32_t crc32c(std::span<const std::byte> bytes, std::uint32_t crc = 0) noexcept {
    crc = ~crc;
    for (const std::byte b : bytes) crc = crc_table[(crc ^ std::to_integer<std::uint32_t>(b)) & 0xff] ^ (crc >> 8);
    return ~crc;
}

/// The journal's records. Each is its payload's length (u32), its type
/// (u8), the payload, and a CRC-32C of type and payload (u32). Only what
/// comes before a `sync` record whose CRC passes is replayed.
enum class Rec : std::uint8_t {
    entry = 1,       ///< a path's state: flags (removed, base, reset), its base's path, size, floor
    chunk = 2,       ///< a path's chunk: index, its bytes now
    rename = 3,      ///< from, to: what is staged at `from` moves to `to`
    sync = 4,        ///< everything before is durable
    committing = 5,  ///< the time (ms) of the commit about to be made of this segment's changes
    aborted = 6,     ///< that commit failed; it did not land
};

/// The journal is a series of segments, journal.<n>, each written by one
/// generation of changes. Its header: magic, the commit it applies on
/// (u64), and a CRC-32C of both.
constexpr std::string_view journal_magic = "SQDRIVE1";
constexpr std::size_t journal_header = 20;
constexpr std::size_t max_record = (std::size_t{1} << 20) + 4096;

void put_u(std::vector<std::byte>& out, std::uint64_t v, int n) {
    for (int i = 0; i < n; ++i) out.push_back(static_cast<std::byte>(v >> (8 * i)));
}
void put_s(std::vector<std::byte>& out, std::string_view s) {
    put_u(out, s.size(), 2);
    const auto b = std::as_bytes(std::span(s));
    out.insert(out.end(), b.begin(), b.end());
}

/// Reading a record's payload: every read checked against its end.
struct Cursor {
    std::span<const std::byte> b;
    std::size_t at = 0;
    bool ok = true;
    std::uint64_t u(int n) {
        if (b.size() - at < static_cast<std::size_t>(n)) {
            ok = false;
            return 0;
        }
        std::uint64_t v = 0;
        for (int i = n - 1; i >= 0; --i) v = v << 8 | std::to_integer<std::uint64_t>(b[at + static_cast<std::size_t>(i)]);
        at += static_cast<std::size_t>(n);
        return v;
    }
    std::string s() {
        const auto n = static_cast<std::size_t>(u(2));
        if (!ok || b.size() - at < n) {
            ok = false;
            return {};
        }
        std::string out(reinterpret_cast<const char*>(b.data() + at), n);
        at += n;
        return out;
    }
};

Result<void> pread_all(int fd, std::uint64_t at, std::span<std::byte> out, const fs::path& path) {
    std::size_t done = 0;
    while (done < out.size()) {
        const ssize_t n = ::pread(fd, out.data() + done, out.size() - done, static_cast<off_t>(at + done));
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return fail(Code::io, errno_text("cannot read", path));
        done += static_cast<std::size_t>(n);
    }
    return {};
}

Result<void> pwrite_all(int fd, std::uint64_t at, std::span<const std::byte> bytes, const fs::path& path) {
    std::size_t done = 0;
    while (done < bytes.size()) {
        const ssize_t n = ::pwrite(fd, bytes.data() + done, bytes.size() - done, static_cast<off_t>(at + done));
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return fail(Code::io, errno_text("cannot write", path));
        done += static_cast<std::size_t>(n);
    }
    return {};
}

Result<void> flush_folder(const fs::path& file) {
    const int fd = ::open(file.parent_path().c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0) return fail(Code::io, errno_text("cannot open the folder of", file));
    const int r = ::fsync(fd);
    ::close(fd);
    if (r != 0 && errno != EINVAL) return fail(Code::io, errno_text("cannot flush the folder of", file));
    return {};
}

/// One segment's file, open; shared by the chunks that live in it, so it
/// stays readable until the last of them is gone.
struct SegmentFile {
    int fd = -1;
    fs::path path;
    SegmentFile(int f, fs::path p) : fd(f), path(std::move(p)) {}
    SegmentFile(const SegmentFile&) = delete;
    SegmentFile& operator=(const SegmentFile&) = delete;
    ~SegmentFile() {
        if (fd >= 0) ::close(fd);
    }
};

std::vector<std::byte> header_bytes(std::uint64_t base) {
    std::vector<std::byte> h;
    const auto m = std::as_bytes(std::span(journal_magic));
    h.insert(h.end(), m.begin(), m.end());
    put_u(h, base, 8);
    put_u(h, crc32c(h), 4);
    return h;
}

/// A record read back: a chunk's bytes are not read, only where they are.
struct Record {
    Rec type{};
    std::vector<std::byte> payload;  ///< a chunk record's: its path and index only
    std::uint64_t data_at = 0;       ///< a chunk record's bytes, in the segment
    std::uint32_t data_length = 0;
};

/// What a segment holds, up to its last sync.
struct SegmentContents {
    std::uint64_t number = 0;
    fs::path path;
    std::uint64_t base = 0;
    std::vector<Record> records;
    std::optional<std::int64_t> committing;  ///< a commit it was about to make (not aborted)
    std::shared_ptr<SegmentFile> file;
};

/// The segment being written: records gather in memory and reach the file,
/// with a sync record, flushed to storage, on flush().
class Segment {
public:
    Segment() = default;

    /// A new, empty segment on `base`: written beside, flushed, renamed into place.
    static Result<Segment> create(const fs::path& path, std::uint64_t base, bool sync) {
        fs::path temporary = path;
        temporary += ".new";
        const auto header = header_bytes(base);
        const int fd = ::open(temporary.c_str(), O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
        if (fd < 0) return fail(Code::io, errno_text("cannot create", temporary));
        auto file = std::make_shared<SegmentFile>(fd, path);
        if (auto r = pwrite_all(fd, 0, header, temporary); !r) return std::unexpected(r.error());
        if (sync && ::fsync(fd) != 0) return fail(Code::io, errno_text("cannot flush", temporary));
        if (::rename(temporary.c_str(), path.c_str()) != 0) return fail(Code::io, errno_text("cannot rename into", path));
        if (sync) {
            if (auto r = flush_folder(path); !r) return std::unexpected(r.error());
        }
        Segment s;
        s.file_ = std::move(file);
        s.end_ = journal_header;
        return s;
    }

    /// Read a segment up to its last sync whose CRC passes; streamed, so a
    /// large segment is never held whole.
    static Result<SegmentContents> read(const fs::path& path, std::uint64_t number) {
        const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
        if (fd < 0) return fail(Code::io, errno_text("cannot open", path));
        SegmentContents c;
        c.number = number;
        c.path = path;
        c.file = std::make_shared<SegmentFile>(fd, path);
        struct stat st{};
        ::fstat(fd, &st);
        const auto size = static_cast<std::uint64_t>(st.st_size);
        std::vector<std::byte> header(journal_header);
        if (size < journal_header || !pread_all(fd, 0, header, path)) return fail(Code::malformed, path.string() + " is not a Drive journal");
        if (std::string_view(reinterpret_cast<const char*>(header.data()), 8) != journal_magic) return fail(Code::malformed, path.string() + " is not a Drive journal");
        Cursor h{std::span(header).subspan(8)};
        c.base = h.u(8);
        if (h.u(4) != crc32c(std::span(header).first(16))) return fail(Code::malformed, path.string() + ": its header does not match its CRC");
        std::vector<Record> tentative;
        std::optional<std::int64_t> committing = std::nullopt;
        std::vector<std::byte> buf;
        for (std::uint64_t at = journal_header; size - at >= 9;) {
            std::array<std::byte, 5> head{};
            if (!pread_all(fd, at, head, path)) break;
            Cursor r{head};
            const auto len = static_cast<std::size_t>(r.u(4));
            if (len > max_record || size - at - 9 < len) break;  // torn, or not a record
            buf.resize(len + 4);
            if (!pread_all(fd, at + 5, buf, path)) break;
            Cursor t{std::span(buf).subspan(len)};
            const std::byte type_byte = head[4];
            if (t.u(4) != crc32c(std::span(buf).first(len), crc32c(std::span(&type_byte, 1)))) break;  // torn or damaged: what follows is not trusted
            const auto type = static_cast<Rec>(std::to_integer<std::uint8_t>(type_byte));
            const std::span<const std::byte> payload(buf.data(), len);
            const std::uint64_t payload_at = at + 5;
            at += 9 + len;
            switch (type) {
                case Rec::sync:
                    std::ranges::move(tentative, std::back_inserter(c.records));
                    tentative.clear();
                    c.committing = committing;
                    break;
                case Rec::committing: {
                    Cursor p{payload};
                    committing = static_cast<std::int64_t>(p.u(8));
                    break;
                }
                case Rec::aborted: committing.reset(); break;
                case Rec::chunk: {
                    Cursor p{payload};
                    static_cast<void>(p.s());
                    static_cast<void>(p.u(8));
                    if (!p.ok) break;
                    Record rec{type, std::vector<std::byte>(payload.begin(), payload.begin() + static_cast<std::ptrdiff_t>(p.at)), payload_at + p.at,
                               static_cast<std::uint32_t>(len - p.at)};
                    tentative.push_back(std::move(rec));
                    break;
                }
                default: tentative.push_back({type, std::vector<std::byte>(payload.begin(), payload.end()), 0, 0});
            }
        }
        return c;
    }

    /// Add a record; where its payload will start in the file (once flushed).
    std::uint64_t add(Rec type, std::span<const std::byte> payload) {
        put_u(pending_, payload.size(), 4);
        const std::byte t{static_cast<std::uint8_t>(type)};
        pending_.push_back(t);
        const std::uint64_t at = end_ + pending_.size();
        pending_.insert(pending_.end(), payload.begin(), payload.end());
        put_u(pending_, crc32c(payload, crc32c(std::span(&t, 1))), 4);
        return at;
    }

    /// Append what was added and a sync record, and flush to storage.
    Result<void> flush(bool sync) {
        add(Rec::sync, {});
        auto r = pwrite_all(file_->fd, end_, pending_, file_->path);
        if (r && sync && ::fsync(file_->fd) != 0) r = fail(Code::io, errno_text("cannot flush", file_->path));
        if (!r) {
            pending_.clear();
            return r;
        }
        end_ += pending_.size();
        pending_.clear();
        return {};
    }

    /// Say the segment applies on another commit (its own landed, or the
    /// cartridge was compacted): its header rewritten in place.
    Result<void> rebase(std::uint64_t base, bool sync) {
        if (auto r = pwrite_all(file_->fd, 0, header_bytes(base), file_->path); !r) return r;
        if (sync && ::fsync(file_->fd) != 0) return fail(Code::io, errno_text("cannot flush", file_->path));
        return {};
    }

    [[nodiscard]] const std::shared_ptr<SegmentFile>& file() const noexcept { return file_; }
    [[nodiscard]] bool empty() const noexcept { return end_ == journal_header; }

private:
    std::shared_ptr<SegmentFile> file_;
    std::uint64_t end_ = journal_header;
    std::vector<std::byte> pending_;
};

// -------------------------------------------------------------- the overlay

/// A changed chunk's bytes: in memory (not synced yet, or loaded back to
/// change), or in a journal segment (synced).
struct Piece {
    std::vector<std::byte> bytes;
    std::shared_ptr<SegmentFile> file;
    std::uint64_t at = 0;
    std::uint32_t length = 0;
    [[nodiscard]] std::uint64_t size() const noexcept { return file ? length : bytes.size(); }
};

/// What is staged at one path: nothing there (removed), or a file built on
/// a committed entry (`base`, renamed or not) or on nothing (a new file),
/// `size` bytes long, its changed chunks in `chunks` (each holding all its
/// bytes now: 1 MiB, or less for the last). Bytes of no changed chunk are
/// the base's below `floor` (the lowest size it was cut to), else zeros.
struct Overlay {
    bool removed = false;
    std::optional<Entry> base;
    std::uint64_t size = 0, floor = 0;
    std::map<std::uint64_t, Piece> chunks;
    // Since the last sync:
    bool dirty = false, reset = false;
    std::set<std::uint64_t> dirty_chunks;
};

using Overlays = std::map<std::string, Overlay, std::less<>>;

/// An entry's metadata as PutOptions, to put it again as it was.
sq::cart::PutOptions options_of(const Entry& e) {
    namespace flag = sq::cart::flag;
    sq::cart::PutOptions o;
    o.title = e.title, o.author = e.author, o.media_type = e.media_type, o.attributes = e.attributes;
    o.created = e.created, o.updated = e.updated;
    o.flags = e.flags & (flag::r | flag::w | flag::x | flag::mappable | flag::dictionary);
    o.store = (e.flags & flag::mappable) ? sq::cart::Store::stored : sq::cart::Store::automatic;
    o.dictionary = e.codec == sq::cart::Codec::zstd_dict ? e.dictionary : 0;
    return o;
}

fs::path default_staging() {
    if (const char* x = std::getenv("XDG_STATE_HOME"); x != nullptr && *x != 0) return fs::path(x) / "sqdrive";
    if (const char* h = std::getenv("HOME"); h != nullptr && *h != 0) return fs::path(h) / ".local" / "state" / "sqdrive";
    return fs::temp_directory_path() / "sqdrive";
}

/// The folder for a cartridge's journal: a hash of its canonical path, and
/// its file name for people.
fs::path staging_folder(const fs::path& staging, const fs::path& cartridge) {
    std::error_code ec;
    auto canonical = fs::weakly_canonical(fs::absolute(cartridge), ec);
    if (ec) canonical = fs::absolute(cartridge);
    const auto text = canonical.string();
    const auto d = sq::cart::sha256(std::as_bytes(std::span(text)));
    std::string name;
    for (std::size_t i = 0; i < 8; ++i) name += std::format("{:02x}", std::to_integer<unsigned>(d[i]));
    std::string readable = cartridge.filename().string();
    for (char& c : readable) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '.' && c != '-' && c != '_') c = '_';
    }
    return staging / (name + "-" + readable);
}

fs::path segment_path(const fs::path& folder, std::uint64_t n) { return folder / std::format("journal.{:08}", n); }

struct File::State {
    State(fs::path p, MountOptions o, sq::cart::Archive a)
        : path(std::move(p)), options(std::move(o)), archive(std::move(a)), cache(options.cache_bytes) {}
    State(const State&) = delete;
    State& operator=(const State&) = delete;

    fs::path path;
    MountOptions options;
    mutable std::shared_mutex m;
    std::condition_variable_any changed;  // a commit or a compaction ended
    std::condition_variable_any wake;     // the commit thread has something to do
    sq::cart::Archive archive;
    Cache cache;
    // A read-write mount:
    std::optional<sq::cart::Writer> writer;
    fs::path folder;
    std::uint64_t next_segment = 0;
    Segment journal;                                    // the segment written now
    std::vector<std::shared_ptr<SegmentFile>> retired;  // older segments the overlays still use
    Overlays overlays;                                  // changes being made
    Overlays frozen;                                    // changes being committed
    std::vector<std::pair<std::string, std::string>> renames;  // since the last sync
    std::uint64_t staged = 0, frozen_bytes = 0, memory = 0;
    std::optional<std::chrono::steady_clock::time_point> first_change;
    bool writable = false, committing = false, compacting = false, commit_wanted = false;
    std::optional<Error> background_error;
    std::jthread worker;

    [[nodiscard]] const sq::cart::Limits& limits() const noexcept { return options.read.limits; }

    // ------------------------------------------------------------ reading

    /// Unit `index` of `e`: from the cache, or read, verified and kept.
    Result<Cache::Unit> unit(const Entry& e, std::uint64_t index) {
        const Key k{e.digest, index};
        if (auto u = cache.find(k)) return u;
        const bool chunked = e.codec == sq::cart::Codec::chunked;
        const std::uint64_t lo = chunked ? index * chunk_size : 0;
        const std::uint64_t n = chunked ? std::min<std::uint64_t>(chunk_size, e.size - lo) : e.size;
        auto bytes = archive.read_range(e, lo, n);
        if (!bytes) return std::unexpected(bytes.error());
        auto u = std::make_shared<const std::vector<std::byte>>(std::move(*bytes));
        cache.put(k, u);
        return u;
    }

    /// Bytes of a committed entry into `out` from `offset`.
    Result<std::size_t> read_entry(const Entry& e, std::uint64_t offset, std::span<std::byte> out) {
        if (offset >= e.size || out.empty()) return std::size_t{0};
        const std::uint64_t end = std::min<std::uint64_t>(e.size, offset + out.size());
        const bool chunked = e.codec == sq::cart::Codec::chunked;
        const std::uint64_t unit_size = chunked ? chunk_size : e.size;
        std::size_t done = 0;
        for (std::uint64_t at = offset; at < end;) {
            const std::uint64_t index = chunked ? at / unit_size : 0;
            auto u = unit(e, index);
            if (!u) return std::unexpected(u.error());
            const std::uint64_t from = at - index * unit_size;
            const std::uint64_t take = std::min<std::uint64_t>((*u)->size() - from, end - at);
            std::copy_n((*u)->begin() + static_cast<std::ptrdiff_t>(from), static_cast<std::ptrdiff_t>(take), out.begin() + static_cast<std::ptrdiff_t>(done));
            done += static_cast<std::size_t>(take);
            at += take;
        }
        return done;
    }

    static Result<void> read_piece(const Piece& p, std::uint64_t from, std::span<std::byte> out) {
        if (!p.file) {
            std::copy_n(p.bytes.begin() + static_cast<std::ptrdiff_t>(from), static_cast<std::ptrdiff_t>(out.size()), out.begin());
            return {};
        }
        return pread_all(p.file->fd, p.at + from, out, p.file->path);
    }

    /// `out` from `at`, as an overlay's base gives it: the base's bytes
    /// below its floor, zeros above (changed chunks aside).
    Result<void> from_base(const Overlay& ov, std::uint64_t at, std::span<std::byte> out) {
        std::ranges::fill(out, std::byte{0});
        if (!ov.base) return {};
        const std::uint64_t base_end = std::min(ov.floor, ov.base->size);
        if (at >= base_end) return {};
        const auto n = static_cast<std::size_t>(std::min<std::uint64_t>(out.size(), base_end - at));
        auto got = read_entry(*ov.base, at, out.first(n));
        if (!got) return std::unexpected(got.error());
        return {};
    }

    Result<std::size_t> read_overlay(const Overlay& ov, std::uint64_t offset, std::span<std::byte> out) {
        if (offset >= ov.size || out.empty()) return std::size_t{0};
        const std::uint64_t end = std::min<std::uint64_t>(ov.size, offset + out.size());
        std::size_t done = 0;
        for (std::uint64_t at = offset; at < end;) {
            const std::uint64_t i = at / chunk_size, lo = i * chunk_size;
            const std::uint64_t take = std::min(lo + chunk_size, end) - at;
            auto piece = out.subspan(done, static_cast<std::size_t>(take));
            if (auto c = ov.chunks.find(i); c != ov.chunks.end()) {
                if (auto r = read_piece(c->second, at - lo, piece); !r) return std::unexpected(r.error());
            } else if (auto r = from_base(ov, at, piece); !r) {
                return std::unexpected(r.error());
            }
            done += static_cast<std::size_t>(take);
            at += take;
        }
        return done;
    }

    /// What a path holds now: changes being made, then changes being
    /// committed, then the cartridge.
    struct Found {
        const Overlay* overlay = nullptr;
        std::optional<Entry> entry;
    };
    Result<Found> find(std::string_view path) const {
        for (const Overlays* gen : {&overlays, &frozen}) {
            if (auto it = gen->find(path); it != gen->end()) {
                if (it->second.removed) return fail(Code::not_found, "no such file", std::string(path));
                return Found{&it->second, std::nullopt};
            }
        }
        auto e = archive.find(path);
        if (!e) return std::unexpected(e.error());
        return Found{nullptr, std::move(*e)};
    }

    Result<Entry> stat(std::string_view path) const {
        auto f = find(path);
        if (!f) return std::unexpected(f.error());
        if (!f->overlay) return std::move(*f->entry);
        return as_entry(path, *f->overlay);
    }
    static Entry as_entry(std::string_view path, const Overlay& ov) {
        Entry e;
        if (ov.base) e = *ov.base;
        else e.flags = sq::cart::flag::r | sq::cart::flag::w;
        e.path = std::string(path);
        e.size = ov.size;
        return e;
    }

    // ------------------------------------------------------------ writing

    Result<void> writable_now() const {
        if (!writable) return fail(Code::unsupported, writer ? "the Drive is unmounted" : "the Drive is mounted read-only");
        return {};
    }

    /// Wait while `path` is being committed, or the cartridge compacted.
    void wait_free(std::unique_lock<std::shared_mutex>& lock, std::initializer_list<std::string_view> paths) {
        const auto busy = [&] {
            if (compacting) return true;
            if (!committing) return false;
            return std::ranges::any_of(paths, [&](std::string_view p) { return frozen.contains(p) && !overlays.contains(p); });
        };
        while (busy()) changed.wait(lock);
    }

    /// The overlay a change to `path` goes to, made from its committed
    /// entry if there is none. A committed entry without `w` (§4.2) is not
    /// changed.
    Result<Overlay*> editable(std::unique_lock<std::shared_mutex>& lock, std::string_view path) {
        wait_free(lock, {path});
        if (auto r = writable_now(); !r) return std::unexpected(r.error());
        if (auto it = overlays.find(path); it != overlays.end()) {
            if (it->second.removed) return fail(Code::not_found, "no such file", std::string(path));
            return &it->second;
        }
        auto e = archive.find(path);
        if (!e) return std::unexpected(e.error());
        if (!(e->flags & sq::cart::flag::w)) return fail(Code::unsupported, "not writable: its entry has no w flag (§4.2)", std::string(path));
        Overlay ov;
        ov.size = ov.floor = e->size;
        ov.base = std::move(*e);
        return &overlays.emplace(std::string(path), std::move(ov)).first->second;
    }

    void note_change() {
        if (!first_change) first_change = std::chrono::steady_clock::now();
    }

    void drop_piece(const Piece& p) {
        staged -= p.size();
        if (!p.file) memory -= p.bytes.size();
    }

    /// Chunk `i` of an overlay, in memory to be changed: loaded back from
    /// the journal, or made from the base if it has none yet.
    Result<std::vector<std::byte>*> chunk(Overlay& ov, std::uint64_t i) {
        if (auto c = ov.chunks.find(i); c != ov.chunks.end()) {
            Piece& p = c->second;
            if (p.file) {
                std::vector<std::byte> bytes(p.length);
                if (auto r = read_piece(p, 0, bytes); !r) return std::unexpected(r.error());
                p.bytes = std::move(bytes);
                p.file.reset();
                memory += p.bytes.size();
            }
            return &p.bytes;
        }
        const std::uint64_t lo = i * chunk_size;
        Piece p;
        p.bytes.resize(static_cast<std::size_t>(std::min<std::uint64_t>(chunk_size, ov.size - lo)));
        if (auto r = from_base(ov, lo, p.bytes); !r) return std::unexpected(r.error());
        staged += p.bytes.size();
        memory += p.bytes.size();
        return &ov.chunks.emplace(i, std::move(p)).first->second.bytes;
    }

    /// Change a chunk's length in memory, its counts with it.
    void set_length(std::vector<std::byte>& bytes, std::uint64_t n) {
        staged += n, memory += n;
        staged -= bytes.size(), memory -= bytes.size();
        bytes.resize(static_cast<std::size_t>(n));
    }

    Result<void> resize(Overlay& ov, std::uint64_t n) {
        if (n > limits().max_entry_size) return fail(Code::limit, "past the entry size limit");
        if (n > ov.size) {
            if (ov.size > 0) {
                const std::uint64_t last = (ov.size - 1) / chunk_size;
                if (ov.chunks.contains(last)) {
                    auto c = chunk(ov, last);
                    if (!c) return std::unexpected(c.error());
                    set_length(**c, std::min<std::uint64_t>(chunk_size, n - last * chunk_size));
                    ov.dirty_chunks.insert(last);
                }
            }
            ov.size = n;
        } else if (n < ov.size) {
            if (n % chunk_size != 0) {
                const std::uint64_t k = n / chunk_size;
                auto c = chunk(ov, k);
                if (!c) return std::unexpected(c.error());
                set_length(**c, n - k * chunk_size);
                ov.dirty_chunks.insert(k);
            }
            for (auto it = ov.chunks.lower_bound((n + chunk_size - 1) / chunk_size); it != ov.chunks.end();) {
                drop_piece(it->second);
                ov.dirty_chunks.erase(it->first);
                it = ov.chunks.erase(it);
            }
            ov.size = n;
            ov.floor = std::min(ov.floor, n);
        }
        ov.dirty = true;
        note_change();
        return {};
    }

    Result<void> write(Overlay& ov, std::uint64_t offset, std::span<const std::byte> bytes) {
        if (bytes.empty()) return {};
        if (offset > limits().max_entry_size || bytes.size() > limits().max_entry_size - offset) return fail(Code::limit, "past the entry size limit");
        const std::uint64_t end = offset + bytes.size();
        if (end > ov.size) {
            if (auto r = resize(ov, end); !r) return r;
        }
        for (std::uint64_t at = offset; at < end;) {
            const std::uint64_t i = at / chunk_size, lo = i * chunk_size;
            const std::uint64_t take = std::min(lo + chunk_size, end) - at;
            auto c = chunk(ov, i);
            if (!c) return std::unexpected(c.error());
            std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(at - offset), static_cast<std::ptrdiff_t>(take),
                        (*c)->begin() + static_cast<std::ptrdiff_t>(at - lo));
            ov.dirty_chunks.insert(i);
            at += take;
        }
        ov.dirty = true;
        note_change();
        return {};
    }

    [[nodiscard]] bool due() const {
        return first_change && !overlays.empty() && std::chrono::steady_clock::now() - *first_change >= options.commit_after;
    }

    Result<void> take_background_error() {
        if (!background_error) return {};
        Error e = std::move(*background_error);
        background_error.reset();
        return std::unexpected(std::move(e));
    }

    /// Before a change: room for it. Past staging_bytes, a budget Drive
    /// waits for a commit; any other refuses.
    Result<void> make_room(std::unique_lock<std::shared_mutex>& lock) {
        while (staged + frozen_bytes >= options.staging_bytes) {
            if (auto r = take_background_error(); !r) return r;
            if (options.commit != Policy::budget || !worker.joinable()) {
                return fail(Code::limit, std::format("{} bytes staged, the most allowed: commit first", staged + frozen_bytes));
            }
            if (!committing) {
                commit_wanted = true;
                wake.notify_all();
            }
            changed.wait(lock);
        }
        return {};
    }

    /// After a change: past memory_bytes, sync; past the budget, ask the
    /// commit thread to commit.
    Result<void> after_change() {
        if (memory >= options.memory_bytes) {
            if (auto r = sync(); !r) return r;
        }
        if (options.commit == Policy::budget && worker.joinable() && (staged >= options.commit_bytes || due())) {
            commit_wanted = true;
            wake.notify_all();
        }
        return {};
    }

    // ------------------------------------------------- the journal, commits

    Result<void> sync() {
        if (!writable) return {};
        const bool dirty = !renames.empty() || std::ranges::any_of(overlays, [](const auto& o) { return o.second.dirty; });
        if (!dirty) return {};
        std::vector<std::byte> p;
        for (const auto& [from, to] : renames) {
            p.clear();
            put_s(p, from);
            put_s(p, to);
            journal.add(Rec::rename, p);
        }
        std::vector<std::tuple<Piece*, std::uint64_t>> moved;  // pieces that will live in the journal
        for (auto& [path, ov] : overlays) {
            if (!ov.dirty) continue;
            p.clear();
            put_s(p, path);
            put_u(p, (ov.removed ? 1u : 0u) | (ov.base ? 2u : 0u) | (ov.reset ? 4u : 0u), 1);
            put_s(p, ov.base ? std::string_view(ov.base->path) : std::string_view{});
            put_u(p, ov.size, 8);
            put_u(p, ov.floor, 8);
            journal.add(Rec::entry, p);
            for (const std::uint64_t i : ov.dirty_chunks) {
                auto c = ov.chunks.find(i);
                if (c == ov.chunks.end() || c->second.file) continue;
                p.clear();
                put_s(p, path);
                put_u(p, i, 8);
                const std::size_t head = p.size();
                p.insert(p.end(), c->second.bytes.begin(), c->second.bytes.end());
                const std::uint64_t at = journal.add(Rec::chunk, p) + head;
                moved.emplace_back(&c->second, at);
            }
        }
        if (auto r = journal.flush(options.write.sync); !r) return r;
        // Synced chunks leave memory: they are read back from the journal.
        for (auto [piece, at] : moved) {
            memory -= piece->bytes.size();
            piece->length = static_cast<std::uint32_t>(piece->bytes.size());
            piece->at = at;
            piece->file = journal.file();
            std::vector<std::byte>{}.swap(piece->bytes);
        }
        renames.clear();
        for (auto& [path, ov] : overlays) {
            ov.dirty = ov.reset = false;
            ov.dirty_chunks.clear();
        }
        return {};
    }

    /// Apply a segment's records to the overlays, as replay after a crash.
    Result<void> replay(const SegmentContents& seg) {
        const auto broken = [&](std::string_view why) { return fail(Code::conflict, std::format("{}: {}", seg.path.string(), why)); };
        for (const Record& r : seg.records) {
            Cursor c{r.payload};
            if (r.type == Rec::rename) {
                const std::string from = c.s(), to = c.s();
                if (!c.ok) return broken("a rename record that does not parse");
                Overlay moved;
                if (auto it = overlays.find(from); it != overlays.end() && !it->second.removed) {
                    moved = std::move(it->second);
                } else if (auto e = archive.find(from); e && it == overlays.end()) {
                    moved.size = moved.floor = e->size;
                    moved.base = std::move(*e);
                } else {
                    return broken(std::format("a rename of {}, which is not there", from));
                }
                overlays.insert_or_assign(to, std::move(moved));
                Overlay gone;
                gone.removed = true;
                overlays.insert_or_assign(from, std::move(gone));
            } else if (r.type == Rec::entry) {
                const std::string path = c.s();
                const auto flags = c.u(1);
                const std::string base = c.s();
                const std::uint64_t size = c.u(8), floor = c.u(8);
                if (!c.ok) return broken("an entry record that does not parse");
                Overlay& ov = overlays[path];
                if ((flags & 4) || (flags & 1)) ov.chunks.clear();
                ov.removed = (flags & 1) != 0;
                ov.base.reset();
                if (flags & 2) {
                    auto e = archive.find(base);
                    if (!e) return broken(std::format("{} builds on {}, which the cartridge does not hold", path, base));
                    ov.base = std::move(*e);
                }
                ov.size = size;
                ov.floor = floor;
                for (auto it = ov.chunks.lower_bound((size + chunk_size - 1) / chunk_size); it != ov.chunks.end();) it = ov.chunks.erase(it);
            } else if (r.type == Rec::chunk) {
                const std::string path = c.s();
                const std::uint64_t i = c.u(8);
                auto it = overlays.find(path);
                if (!c.ok || it == overlays.end() || it->second.removed) return broken("a chunk record for no file");
                Piece p;
                p.file = seg.file;
                p.at = r.data_at;
                p.length = r.data_length;
                it->second.chunks.insert_or_assign(i, std::move(p));
            }
        }
        return {};
    }

    void recount() {
        staged = memory = 0;
        for (const auto& [path, ov] : overlays) {
            for (const auto& [i, p] : ov.chunks) {
                staged += p.size();
                if (!p.file) memory += p.bytes.size();
            }
        }
    }

    /// Hand a generation of changes to the writer as one commit's.
    Result<std::vector<fs::path>> stage(sq::cart::Writer& w, Overlays& gen) {
        std::vector<fs::path> temporaries;
        // Renames, through temporary names, so that any permutation (a swap) works.
        std::vector<std::pair<std::string, std::string>> moves;
        std::set<std::string, std::less<>> sources;
        for (const auto& [path, ov] : gen) {
            if (!ov.removed && ov.base && ov.base->path != path) {
                moves.emplace_back(ov.base->path, path);
                sources.insert(ov.base->path);
            }
        }
        std::vector<std::string> parked;
        for (std::size_t n = 0; const auto& [from, to] : moves) {
            std::string tmp;
            do tmp = std::format("SQ-INF/drive/rename-{}", n++);
            while (w.size(tmp).has_value());
            if (auto r = w.rename(from, tmp); !r) return std::unexpected(r.error());
            parked.push_back(std::move(tmp));
        }
        for (std::size_t k = 0; k < moves.size(); ++k) {
            if (auto r = w.rename(parked[k], moves[k].second); !r) return std::unexpected(r.error());
        }
        // Removals of what was committed and is not moved.
        for (const auto& [path, ov] : gen) {
            if (ov.removed && !sources.contains(path) && archive.find(path).has_value()) {
                if (auto r = w.remove(path); !r) return std::unexpected(r.error());
            }
        }
        // Contents.
        std::vector<std::byte> buf(static_cast<std::size_t>(chunk_size));
        for (auto& [path, ov] : gen) {
            if (ov.removed) continue;
            const bool cut = ov.base && ov.floor < ov.base->size;
            const bool changed = !ov.base || cut || ov.size != ov.base->size || !ov.chunks.empty();
            if (!changed) continue;
            if (ov.base && ov.base->codec == sq::cart::Codec::chunked) {
                // Only the chunks changed are written; the rest are named again.
                if (cut) {
                    if (auto r = w.resize(path, ov.floor); !r) return std::unexpected(r.error());
                }
                if (ov.size != (cut ? ov.floor : ov.base->size)) {
                    if (auto r = w.resize(path, ov.size); !r) return std::unexpected(r.error());
                }
                // A synced chunk is read from its journal segment at commit,
                // a chunk at a time (put_range_file): the commit holds none
                // of them in memory. One still in memory goes as it is.
                for (const auto& [i, piece] : ov.chunks) {
                    if (piece.file) {
                        if (auto r = w.put_range_file(path, i * chunk_size, piece.file->path, piece.at, piece.length); !r) {
                            return std::unexpected(r.error());
                        }
                    } else if (auto r = w.put_range(path, i * chunk_size, piece.bytes); !r) {
                        return std::unexpected(r.error());
                    }
                }
                continue;
            }
            // Anything else is put whole: from memory up to 64 MiB, else from
            // a file. A file the Drive made is stored chunked, so the next
            // commit writes only the chunks it changes.
            auto options = ov.base ? options_of(*ov.base) : sq::cart::PutOptions{.flags = sq::cart::flag::r | sq::cart::flag::w};
            if (!ov.base && ov.size > 0) options.store = sq::cart::Store::chunked;
            if (ov.size <= limits().max_unchunked_size) {
                std::vector<std::byte> content(static_cast<std::size_t>(ov.size));
                auto got = read_overlay(ov, 0, content);
                if (!got) return std::unexpected(got.error());
                if (auto r = w.put(path, content, options); !r) return std::unexpected(r.error());
                continue;
            }
            const fs::path tmp = folder / std::format("commit-{}.tmp", temporaries.size());
            temporaries.push_back(tmp);
            std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
            for (std::uint64_t at = 0; at < ov.size; at += chunk_size) {
                auto got = read_overlay(ov, at, buf);
                if (!got) return std::unexpected(got.error());
                out.write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(*got));
            }
            out.close();
            if (!out) return fail(Code::io, "cannot write " + tmp.string(), path);
            options.store = sq::cart::Store::chunked;
            if (auto r = w.put_file(path, tmp, options); !r) return std::unexpected(r.error());
        }
        return temporaries;
    }

    /// The commit itself, with the lock not held: only the thread with
    /// `committing` set uses the writer and the frozen changes.
    Result<sq::cart::Commit> write_commit(std::int64_t time) {
        crash_point("before_commit");
        auto temporaries = stage(*writer, frozen);
        const auto clean = [&] {
            std::error_code ec;
            if (temporaries) {
                for (const auto& t : *temporaries) fs::remove(t, ec);
            }
        };
        if (!temporaries) {
            writer->discard();
            return std::unexpected(temporaries.error());
        }
        writer->set_commit_time(time);
        auto c = writer->commit();
        clean();
        if (!c) {
            writer->discard();
            return std::unexpected(c.error());
        }
        crash_point("after_commit");
        return c;
    }

    /// Commit what is staged: freeze it, start a new segment for the writes
    /// that go on meanwhile, commit without the lock, then take the new
    /// state (or, on failure, give the changes back).
    Result<sq::cart::Commit> commit(std::unique_lock<std::shared_mutex>& lock) {
        while (committing || compacting) changed.wait(lock);
        if (auto r = writable_now(); !r) return std::unexpected(r.error());
        if (overlays.empty()) return archive.commit();
        if (auto r = sync(); !r) return std::unexpected(r.error());
        const std::uint64_t base = archive.commit().sequence;
        auto next = Segment::create(segment_path(folder, next_segment), base, options.write.sync);
        if (!next) return std::unexpected(next.error());
        ++next_segment;
        Segment old = std::exchange(journal, std::move(*next));
        auto covered = std::exchange(retired, {});
        frozen = std::exchange(overlays, {});
        frozen_bytes = std::exchange(staged, 0);
        memory = 0;
        first_change.reset();
        committing = true;
        // Write the commit's time down first: after a crash, a commit with
        // this time on top of the base is this one.
        const std::int64_t time = std::max(now_ms(), archive.commit().time + 1);
        std::vector<std::byte> p;
        put_u(p, static_cast<std::uint64_t>(time), 8);
        old.add(Rec::committing, p);
        auto flushed = old.flush(options.write.sync);
        lock.unlock();
        Result<sq::cart::Commit> c = flushed ? write_commit(time) : std::unexpected(flushed.error());
        const bool landed = c.has_value();
        std::optional<sq::cart::Archive> fresh;
        if (c) {
            auto a = sq::cart::Archive::open(path, options.read);
            if (a) fresh = std::move(*a);
            else c = std::unexpected(a.error());
        }
        lock.lock();
        if (!landed) {
            // Not committed: the changes come back (no write touched their
            // paths meanwhile), their segments with them.
            if (flushed) {
                old.add(Rec::aborted, {});
                static_cast<void>(old.flush(options.write.sync));
            }
            for (auto& [path, ov] : frozen) overlays.insert_or_assign(path, std::move(ov));
            frozen.clear();
            retired = std::move(covered);
            retired.push_back(old.file());
            recount();
            note_change();
        } else if (!fresh) {
            // Committed, but the cartridge does not read back: stop writing.
            writable = false;
        } else {
            archive = std::move(*fresh);
            frozen.clear();
            std::error_code ec;
            fs::remove(old.file()->path, ec);
            for (const auto& f : covered) fs::remove(f->path, ec);
            if (auto r = journal.rebase(c->sequence, options.write.sync); !r) c = std::unexpected(r.error());
        }
        frozen_bytes = 0;
        committing = false;
        changed.notify_all();
        if (c && overlays.empty()) maybe_compact(lock);
        return c;
    }

    /// After a commit, with nothing staged: compact if enough of the file
    /// is unreachable. Writers wait; readers go on with the old file.
    void maybe_compact(std::unique_lock<std::shared_mutex>& lock) {
        if (options.compact_at <= 0 || !writable) return;
        auto live = archive.entries();
        if (!live) return;
        std::uint64_t used = 8192;
        for (const Entry& e : *live) {
            used += e.stored_size;
            if (e.codec == sq::cart::Codec::chunked) {
                auto cs = archive.chunks(e);
                if (!cs) return;
                for (const auto& ch : *cs) used += ch.stored_size;
            }
        }
        const std::uint64_t size = archive.file_size();
        const std::uint64_t dead = size > used ? size - used : 0;
        if (dead < options.compact_min_bytes || static_cast<double>(dead) < options.compact_at * static_cast<double>(size)) return;
        compacting = true;
        lock.unlock();
        auto c = writer->compact();
        std::optional<sq::cart::Archive> fresh;
        if (c) {
            if (auto a = sq::cart::Archive::open(path, options.read)) fresh = std::move(*a);
        }
        lock.lock();
        if (!c) {
            background_error = c.error();
        } else if (!fresh) {
            writable = false;
        } else {
            archive = std::move(*fresh);
            if (auto r = journal.rebase(c->sequence, options.write.sync); !r) background_error = r.error();
        }
        compacting = false;
        changed.notify_all();
    }

    /// The commit thread of a budget Drive: commits when asked, or when the
    /// oldest change is commit_after old.
    void run(std::stop_token stop) {
        std::unique_lock lock(m);
        while (!stop.stop_requested()) {
            const auto ready = [&] { return commit_wanted || due(); };
            if (first_change) wake.wait_until(lock, stop, *first_change + options.commit_after, ready);
            else wake.wait(lock, stop, ready);
            if (stop.stop_requested()) break;
            if (!ready()) continue;
            commit_wanted = false;
            if (auto c = commit(lock); !c) {
                background_error = c.error();
                changed.notify_all();
                if (!writable) break;
            }
        }
    }

    Result<void> unmount(std::unique_lock<std::shared_mutex>& lock) {
        if (!writable && !worker.joinable()) return {};
        if (worker.joinable()) {
            worker.request_stop();
            wake.notify_all();
            lock.unlock();
            worker.join();
            lock.lock();
        }
        while (committing || compacting) changed.wait(lock);
        Result<void> r = take_background_error();
        if (writable) {
            if (options.commit == Policy::explicit_only) {
                if (auto s = sync(); !s && r) r = s;
            } else if (auto c = commit(lock); !c) {
                if (r) r = std::unexpected(c.error());
                static_cast<void>(sync());  // keep what can be kept for the next mount
            }
        }
        writable = false;
        writer.reset();
        journal = Segment{};
        retired.clear();
        return r;
    }
};

// ------------------------------------------------------------------ File

std::uint64_t File::size() const {
    if (entry_) return entry_->size;
    std::shared_lock lock(state_->m);
    auto e = state_->stat(path_);
    return e ? e->size : 0;
}

Result<Entry> File::stat() const {
    if (entry_) return *entry_;
    std::shared_lock lock(state_->m);
    return state_->stat(path_);
}

Result<std::size_t> File::read(std::uint64_t offset, std::span<std::byte> out) const {
    if (entry_) return state_->read_entry(*entry_, offset, out);
    std::shared_lock lock(state_->m);
    auto f = state_->find(path_);
    if (!f) return std::unexpected(f.error());
    if (f->overlay) return state_->read_overlay(*f->overlay, offset, out);
    return state_->read_entry(*f->entry, offset, out);
}

Result<void> File::write(std::uint64_t offset, std::span<const std::byte> bytes) {
    std::unique_lock lock(state_->m);
    if (auto r = state_->writable_now(); !r) return r;
    if (auto r = state_->make_room(lock); !r) return r;
    auto ov = state_->editable(lock, path_);
    if (!ov) return std::unexpected(ov.error());
    if (auto r = state_->write(**ov, offset, bytes); !r) return std::unexpected(Error{r.error().code, r.error().message, path_});
    return state_->after_change();
}

Result<void> File::resize(std::uint64_t size) {
    std::unique_lock lock(state_->m);
    if (auto r = state_->writable_now(); !r) return r;
    if (auto r = state_->make_room(lock); !r) return r;
    auto ov = state_->editable(lock, path_);
    if (!ov) return std::unexpected(ov.error());
    if (auto r = state_->resize(**ov, size); !r) return std::unexpected(Error{r.error().code, r.error().message, path_});
    return state_->after_change();
}

Result<void> File::sync() {
    std::unique_lock lock(state_->m);
    return state_->sync();
}

// ------------------------------------------------------------------ Drive

namespace {

/// Rewrite a segment file's header to apply on `base`.
Result<void> rebase_file(const fs::path& path, std::uint64_t base, bool sync) {
    const int fd = ::open(path.c_str(), O_WRONLY | O_CLOEXEC);
    if (fd < 0) return fail(Code::io, errno_text("cannot open", path));
    auto r = pwrite_all(fd, 0, header_bytes(base), path);
    if (r && sync && ::fsync(fd) != 0) r = fail(Code::io, errno_text("cannot flush", path));
    ::close(fd);
    return r;
}

}  // namespace

Result<Drive> Drive::mount(const fs::path& cartridge, MountOptions options) {
    options.write.limits = options.read.limits;
    if (options.access == Access::read_only) {
        auto archive = sq::cart::Archive::open(cartridge, options.read);
        if (!archive) return std::unexpected(archive.error());
        return Drive(std::make_shared<File::State>(cartridge, std::move(options), std::move(*archive)));
    }
    // The writer's lock first: no one else changes the cartridge from here.
    auto writer = sq::cart::Writer::open(cartridge, options.write);
    if (!writer) return std::unexpected(writer.error());
    auto archive = sq::cart::Archive::open(cartridge, options.read);
    if (!archive) return std::unexpected(archive.error());
    const fs::path folder = staging_folder(options.staging.empty() ? default_staging() : options.staging, cartridge);
    std::error_code ec;
    fs::create_directories(folder, ec);
    if (ec) return fail(Code::io, std::format("cannot create {}: {}", folder.string(), ec.message()));
    const bool sync = options.write.sync;
    const std::uint64_t head = archive->commit().sequence;
    const std::int64_t head_time = archive->commit().time;
    auto state = std::make_shared<File::State>(cartridge, std::move(options), std::move(*archive));
    state->writer = std::move(*writer);
    state->folder = folder;

    // The segments, oldest first (a journal from before segments is the first).
    if (fs::exists(folder / "journal", ec)) fs::rename(folder / "journal", segment_path(folder, 0), ec);
    std::vector<std::uint64_t> numbers;
    for (const auto& e : fs::directory_iterator(folder, ec)) {
        const std::string name = e.path().filename().string();
        if (name.size() != 16 || !name.starts_with("journal.")) continue;
        std::uint64_t n = 0;
        if (std::from_chars(name.data() + 8, name.data() + 16, n).ec == std::errc{}) numbers.push_back(n);
    }
    std::ranges::sort(numbers);
    std::vector<SegmentContents> segments;
    for (const std::uint64_t n : numbers) {
        auto s = Segment::read(segment_path(folder, n), n);
        if (!s) return std::unexpected(s.error());
        state->next_segment = n + 1;
        if (s->records.empty() && !s->committing) {
            fs::remove(s->path, ec);  // nothing in it
            continue;
        }
        segments.push_back(std::move(*s));
    }
    // A commit that landed covers its segment and every one before it.
    std::size_t first = 0;
    for (std::size_t i = 0; i < segments.size(); ++i) {
        const auto& s = segments[i];
        if (s.committing && head == s.base + 1 && head_time == *s.committing) first = i + 1;
    }
    for (std::size_t i = 0; i < first; ++i) fs::remove(segments[i].path, ec);
    segments.erase(segments.begin(), segments.begin() + static_cast<std::ptrdiff_t>(first));
    const bool fits = std::ranges::all_of(segments, [&](const SegmentContents& s) { return s.base == head || (first > 0 && s.base + 1 == head); });
    if (!fits) {
        if (!state->options.discard_journal) {
            return fail(Code::conflict, std::format("{} holds changes made on commit {}, and the cartridge is at commit {}: another writer "
                                                    "committed since (mount with discard_journal to set them aside)",
                                                    folder.string(), segments.front().base, head));
        }
        const auto when = now_ms();
        for (const auto& s : segments) {
            fs::path aside = folder / std::format("journal.conflict-{}.{}", when, s.number);
            fs::rename(s.path, aside, ec);
            if (ec) return fail(Code::io, std::format("cannot set {} aside: {}", s.path.string(), ec.message()));
        }
        segments.clear();
    }
    for (const auto& s : segments) {
        if (auto r = state->replay(s); !r) return std::unexpected(r.error());
        if (s.base != head) {
            if (auto r = rebase_file(s.path, head, sync); !r) return std::unexpected(r.error());
        }
        state->retired.push_back(s.file);
    }
    state->recount();
    if (!state->overlays.empty()) state->note_change();
    auto active = Segment::create(segment_path(folder, state->next_segment), head, sync);
    if (!active) return std::unexpected(active.error());
    ++state->next_segment;
    state->journal = std::move(*active);
    state->writable = true;
    if (state->options.commit == Policy::budget) {
        state->worker = std::jthread([raw = state.get()](std::stop_token stop) { raw->run(std::move(stop)); });
    }
    return Drive(std::move(state));
}

Drive::~Drive() {
    if (state_) {
        std::unique_lock lock(state_->m);
        static_cast<void>(state_->unmount(lock));
    }
}

Result<File> Drive::open(std::string_view path, unsigned how) const {
    if (!state_->writer && !state_->writable) {
        if (how != mode::read) return fail(Code::unsupported, "the Drive is mounted read-only", std::string(path));
        auto e = state_->archive.find(path);
        if (!e) return std::unexpected(e.error());
        return File(state_, std::string(path), std::move(*e));
    }
    std::unique_lock lock(state_->m);
    if (how == mode::read) {
        if (auto f = state_->find(path); !f) return std::unexpected(f.error());
        return File(state_, std::string(path), std::nullopt);
    }
    if (auto r = state_->writable_now(); !r) return std::unexpected(r.error());
    state_->wait_free(lock, {path});
    auto found = state_->find(path);
    if (!found) {
        if (found.error().code != Code::not_found || !(how & mode::create)) return std::unexpected(found.error());
        if (auto problem = sq::cart::path_problem(path, state_->limits().max_path_bytes)) return fail(Code::path, *problem, std::string(path));
        if (path.starts_with("SQ-INF/")) return fail(Code::path, "the Drive does not write in SQ-INF/", std::string(path));
        Overlay ov;
        ov.reset = ov.dirty = true;
        state_->overlays.insert_or_assign(std::string(path), std::move(ov));
        state_->note_change();
    } else if (how & mode::truncate) {
        auto ov = state_->editable(lock, path);
        if (!ov) return std::unexpected(ov.error());
        if (auto r = state_->resize(**ov, 0); !r) return std::unexpected(r.error());
    } else {
        if (auto ov = state_->editable(lock, path); !ov) return std::unexpected(ov.error());  // refuses one without w
    }
    return File(state_, std::string(path), std::nullopt);
}

Result<Entry> Drive::stat(std::string_view path) const {
    std::shared_lock lock(state_->m);
    return state_->stat(path);
}

Result<std::vector<Entry>> Drive::list(std::string_view prefix) const {
    std::shared_lock lock(state_->m);
    auto live = state_->archive.entries();
    if (!live) return std::unexpected(live.error());
    std::map<std::string, Entry, std::less<>> all;
    const auto first = std::ranges::lower_bound(*live, prefix, {}, [](const Entry& e) -> std::string_view { return e.path; });
    for (auto it = first; it != live->end() && it->path.starts_with(prefix); ++it) all.emplace(it->path, *it);
    for (const Overlays* gen : {&state_->frozen, &state_->overlays}) {
        for (const auto& [path, ov] : *gen) {
            if (!path.starts_with(prefix)) continue;
            if (ov.removed) all.erase(path);
            else all.insert_or_assign(path, File::State::as_entry(path, ov));
        }
    }
    std::vector<Entry> out;
    for (auto& [path, e] : all) out.push_back(std::move(e));
    return out;
}

Result<void> Drive::remove(std::string_view path) {
    std::unique_lock lock(state_->m);
    if (auto r = state_->writable_now(); !r) return r;
    auto ov = state_->editable(lock, path);
    if (!ov) return std::unexpected(ov.error());
    for (const auto& [i, p] : (*ov)->chunks) state_->drop_piece(p);
    Overlay gone;
    gone.removed = gone.reset = gone.dirty = true;
    **ov = std::move(gone);
    state_->note_change();
    return state_->after_change();
}

Result<void> Drive::rename(std::string_view from, std::string_view to) {
    std::unique_lock lock(state_->m);
    if (auto r = state_->writable_now(); !r) return r;
    if (from == to) return state_->find(from).has_value() ? Result<void>{} : fail(Code::not_found, "no such file", std::string(from));
    if (auto problem = sq::cart::path_problem(to, state_->limits().max_path_bytes)) return fail(Code::path, *problem, std::string(to));
    for (std::string_view p : {from, to}) {
        if (p.starts_with("SQ-INF/")) return fail(Code::path, "the Drive does not move files in SQ-INF/", std::string(p));
    }
    state_->wait_free(lock, {from, to});
    auto source = state_->editable(lock, from);
    if (!source) return std::unexpected(source.error());
    if (auto target = state_->find(to); target && !target->overlay && !(target->entry->flags & sq::cart::flag::w)) {
        return fail(Code::unsupported, "not writable: its entry has no w flag (§4.2)", std::string(to));
    }
    Overlay moved = std::move(**source);
    moved.dirty = true;
    if (auto old = state_->overlays.find(to); old != state_->overlays.end()) {
        for (const auto& [i, p] : old->second.chunks) state_->drop_piece(p);
    }
    Overlay gone;
    gone.removed = gone.reset = gone.dirty = true;
    state_->overlays.insert_or_assign(std::string(from), std::move(gone));
    state_->overlays.insert_or_assign(std::string(to), std::move(moved));
    state_->renames.emplace_back(from, to);
    state_->note_change();
    return state_->after_change();
}

Result<void> Drive::sync() {
    std::unique_lock lock(state_->m);
    return state_->sync();
}

Result<sq::cart::Commit> Drive::commit() {
    std::unique_lock lock(state_->m);
    if (auto r = state_->take_background_error(); !r) return std::unexpected(r.error());
    return state_->commit(lock);
}

Result<void> Drive::wait() {
    std::unique_lock lock(state_->m);
    while (state_->committing || state_->compacting || state_->commit_wanted) state_->changed.wait(lock);
    return state_->take_background_error();
}

Result<void> Drive::unmount() {
    std::unique_lock lock(state_->m);
    return state_->unmount(lock);
}

sq::cart::Commit Drive::head() const {
    std::shared_lock lock(state_->m);
    return state_->archive.commit();
}
const fs::path& Drive::path() const noexcept { return state_->path; }
std::uint64_t Drive::staged_bytes() const {
    std::shared_lock lock(state_->m);
    return state_->staged + state_->frozen_bytes;
}
std::uint64_t Drive::memory_bytes() const {
    std::shared_lock lock(state_->m);
    return state_->memory;
}
fs::path Drive::journal_path() const {
    std::shared_lock lock(state_->m);
    return state_->journal.file() ? state_->journal.file()->path : fs::path{};
}
CacheStats Drive::cache_stats() const { return state_->cache.stats(); }
void Drive::drop_cache() { state_->cache.clear(); }

Result<sq::cart::Commit> pack(const fs::path& folder, const fs::path& cartridge, PackOptions options) {
    std::error_code ec;
    if (fs::exists(fs::symlink_status(cartridge, ec))) {
        return fail(Code::io, cartridge.string() + " is there already: pack makes a new cartridge");
    }
    return sq::cart::pack(folder, cartridge, options.write,
                          {.keep = std::move(options.keep), .leave_out = std::move(options.leave_out)});
}

Result<std::size_t> explode(const fs::path& cartridge, const fs::path& folder, ExplodeOptions options) {
    auto archive = sq::cart::Archive::open(cartridge, options.read);
    if (!archive) return std::unexpected(archive.error());
    return sq::cart::unpack(*archive, folder, {.merge = options.merge, .executable = options.executable});
}

}  // namespace sq::drive
