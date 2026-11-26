// SPDX-License-Identifier: MIT
//
// sq.cart:exploded — a cartridge unpacked into a folder (§6): every live
// entry a file, its metadata in SQ-INF/entries.json, created so that the
// cartridge cannot write anywhere else. Packing a folder is the writer's
// (sq.cart.write's pack).
//
// Every file and folder is created by this code, relative to a folder it
// holds open, with O_NOFOLLOW and O_EXCL: a symbolic link planted in the
// target, or created by a name earlier in the cartridge, is never followed
// (the Zip Slip family, red-team R12).

module;

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <stdio.h>  // renameat
#include <sys/stat.h>
#include <unistd.h>

export module sq.cart:exploded;

import std;
import sq.data;
import :error;
import :json;
import :fold;
import :archive;

namespace sq::cart {

constexpr std::string_view entries_json = "SQ-INF/entries.json";

/// Windows cannot create these names (§4.1): refused by unpack everywhere,
/// so a cartridge unpacks on every system or on none.
[[nodiscard]] bool reserved_on_windows(std::string_view segment) {
    if (segment.ends_with('.') || segment.ends_with(' ')) return true;
    std::string base(segment.substr(0, segment.find('.')));
    std::ranges::transform(base, base.begin(), [](char c) { return static_cast<char>(c >= 'a' && c <= 'z' ? c - 32 : c); });
    static constexpr std::string_view names[] = {"CON", "PRN", "AUX", "NUL"};
    if (std::ranges::find(names, base) != std::end(names)) return true;
    return base.size() == 4 && (base.starts_with("COM") || base.starts_with("LPT")) && base[3] >= '1' && base[3] <= '9';
}

class Fd {
public:
    explicit Fd(int fd = -1) : fd_(fd) {}
    Fd(Fd&& o) noexcept : fd_(std::exchange(o.fd_, -1)) {}
    Fd& operator=(Fd&& o) noexcept {
        if (this != &o) {
            if (fd_ >= 0) ::close(fd_);
            fd_ = std::exchange(o.fd_, -1);
        }
        return *this;
    }
    ~Fd() {
        if (fd_ >= 0) ::close(fd_);
    }
    [[nodiscard]] int get() const noexcept { return fd_; }
    [[nodiscard]] bool ok() const noexcept { return fd_ >= 0; }

private:
    int fd_;
};

/// The folder `rel` under `root`, made as needed, each step opened without
/// following a link.
Result<Fd> open_folder(int root, std::string_view rel, const std::string& entry) {
    Fd at(::dup(root));
    for (std::size_t from = 0; from < rel.size();) {
        std::size_t slash = rel.find('/', from);
        if (slash == std::string_view::npos) slash = rel.size();
        const std::string name(rel.substr(from, slash - from));
        if (::mkdirat(at.get(), name.c_str(), 0755) != 0 && errno != EEXIST) {
            return fail(Code::io, std::format("cannot make the folder {}: {}", name, std::strerror(errno)), entry);
        }
        Fd next(::openat(at.get(), name.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
        if (!next.ok()) {
            return fail(Code::path, std::format("{} is not a folder this can write into (a link?): {}", name, std::strerror(errno)), entry);
        }
        at = std::move(next);
        from = slash + 1;
    }
    return at;
}

Result<void> write_all(int fd, std::span<const std::byte> bytes, const std::string& name, const std::string& entry) {
    std::size_t done = 0;
    while (done < bytes.size()) {
        const ssize_t n = ::write(fd, bytes.data() + done, bytes.size() - done);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return fail(Code::io, std::format("cannot write {}: {}", name, std::strerror(errno)), entry);
        done += static_cast<std::size_t>(n);
    }
    return {};
}

Result<void> write_new(int folder, const std::string& name, std::span<const std::byte> bytes, const std::string& entry,
                       ::mode_t mode = 0644) {
    Fd fd(::openat(folder, name.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, mode));
    if (!fd.ok()) return fail(Code::io, std::format("cannot create {}: {}", name, std::strerror(errno)), entry);
    return write_all(fd.get(), bytes, name, entry);
}

/// An entry into a new file, read and verified a chunk at a time when it is
/// chunked (codec 3: never held whole, so a large asset unpacks on a
/// phone), else read whole (64 MiB at most) and verified before it is written.
Result<void> write_entry(const Archive& cart, const Entry& e, int folder, const std::string& name, ::mode_t mode) {
    if (e.codec != Codec::chunked) {
        auto bytes = cart.read(e);
        if (!bytes) return std::unexpected(bytes.error());
        return write_new(folder, name, *bytes, e.path, mode);
    }
    // Into NAME.part, renamed to NAME once every chunk has been verified: a
    // damaged chunk leaves no file that looks whole.
    if (::faccessat(folder, name.c_str(), F_OK, AT_SYMLINK_NOFOLLOW) == 0) {
        return fail(Code::io, std::format("cannot create {}: {}", name, std::strerror(EEXIST)), e.path);
    }
    const std::string part = name + ".part";
    Fd fd(::openat(folder, part.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, mode));
    if (!fd.ok()) return fail(Code::io, std::format("cannot create {}: {}", part, std::strerror(errno)), e.path);
    const auto give_up = [&](Error error) {
        ::unlinkat(folder, part.c_str(), 0);
        return std::unexpected(std::move(error));
    };
    for (std::uint64_t at = 0; at < e.size; at += chunk_size) {
        auto bytes = cart.read_range(e, at, std::min<std::uint64_t>(chunk_size, e.size - at));
        if (!bytes) return give_up(bytes.error());
        if (auto r = write_all(fd.get(), *bytes, name, e.path); !r) return give_up(r.error());
    }
    if (::renameat(folder, part.c_str(), folder, name.c_str()) != 0) {
        return give_up(Error{Code::io, std::format("cannot rename {}: {}", part, std::strerror(errno)), e.path});
    }
    return {};
}

}  // namespace sq::cart

export namespace sq::cart {

/// The metadata of an entry as SQ-INF/entries.json holds it (§6): only
/// what differs from a plain file's (flags r, nothing else). Nothing when
/// there is nothing to say.
[[nodiscard]] std::optional<data::JsonValue> exploded_metadata(const Entry& e) {
    data::JsonValue::Object o;
    if (e.title) o.emplace("title", *e.title);
    if (e.author) o.emplace("author", *e.author);
    if (e.media_type) o.emplace("media_type", *e.media_type);
    if (e.created != 0) o.emplace("created", e.created);
    if (e.updated != 0) o.emplace("updated", e.updated);
    if (e.attributes) {
        auto a = data::parse_json(*e.attributes, {.ijson = true});
        if (a) o.emplace("attributes", std::move(a.value));
    }
    std::string letters;
    if (e.flags & flag::r) letters += 'r';
    if (e.flags & flag::w) letters += 'w';
    if (e.flags & flag::x) letters += 'x';
    if (e.flags & flag::mappable) letters += 'm';
    if (e.flags & flag::dictionary) letters += 'd';
    if (letters != "r") o.emplace("flags", letters);
    if (o.empty()) return std::nullopt;
    return data::JsonValue(std::move(o));
}

/// One record of SQ-INF/entries.json (§6) as an entry's metadata: path,
/// title, author, media_type, created, updated, attributes (canonical
/// JSON), flags. A member it does not know, or of another type, is an
/// error: a typo would otherwise drop a flag silently.
[[nodiscard]] Result<Entry> exploded_record(const data::JsonValue& v, const std::string& path) {
    Entry e;
    e.path = path;
    e.flags = flag::r;
    if (v.object_if() == nullptr) return fail(Code::json, "its SQ-INF/entries.json record is not an object", path);
    for (const auto& [name, value] : *v.object_if()) {
        const std::string* s = value.string_if();
        const auto number = [&]() -> std::optional<std::int64_t> {
            if (const auto* i = value.signed_integer_if()) return *i;
            if (const auto* u = value.unsigned_integer_if()) return static_cast<std::int64_t>(*u);
            return std::nullopt;
        };
        if (name == "title" && s) {
            e.title = *s;
        } else if (name == "author" && s) {
            e.author = *s;
        } else if (name == "media_type" && s) {
            e.media_type = *s;
        } else if (name == "created" && number()) {
            e.created = *number();
        } else if (name == "updated" && number()) {
            e.updated = *number();
        } else if (name == "attributes" && value.object_if()) {
            auto text = data::write_json(value, {.canonical = true});
            if (!text) return fail(Code::json, text.error.message, path);
            e.attributes = std::move(text.text);
        } else if (name == "flags" && s) {
            e.flags = 0;
            for (const char c : *s) {
                switch (c) {
                    case 'r': e.flags |= flag::r; break;
                    case 'w': e.flags |= flag::w; break;
                    case 'x': e.flags |= flag::x; break;
                    case 'm': e.flags |= flag::mappable; break;
                    case 'd': e.flags |= flag::dictionary; break;
                    default: return fail(Code::json, std::format("the flag '{}' in SQ-INF/entries.json", c), path);
                }
            }
        } else {
            return fail(Code::json, std::format("SQ-INF/entries.json: \"{}\" is not a member, or not of its type", name), path);
        }
    }
    return e;
}

/// An exploded cartridge (§6), read where it is: its manifest, its entries
/// (the folder's regular files, with SQ-INF/entries.json's metadata), and
/// their bytes. The same rules as pack: a symbolic link anywhere is refused,
/// a top-level folder named with a dot is not entries, every path is NFC
/// and follows §4.1. Nothing is hashed: the file system holds the bytes,
/// and a folder has no index to check them against. Orphan records are
/// ignored (§6). Entries are listed once, at open; read() opens each
/// folder of the path again without following a link, so a link made
/// after open is refused too.
class Folder {
public:
    [[nodiscard]] static Result<Folder> open(const std::filesystem::path& folder, const ReadOptions& options = {}) {
        namespace fs = std::filesystem;
        Folder f;
        f.root_ = folder;
        f.options_ = options;
        std::error_code ec;
        if (!fs::is_directory(fs::symlink_status(folder, ec))) return fail(Code::io, folder.string() + " is not a folder");
        Fd root(::open(folder.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
        if (!root.ok()) return fail(Code::io, std::format("cannot open {}: {}", folder.string(), std::strerror(errno)));
        auto text = read_at(root.get(), std::string(manifest_file), options.limits.max_unchunked_size);
        if (!text) {
            if (text.error().code == Code::not_found) return fail(Code::malformed, folder.string() + " is not a cartridge: no SQ-INF/manifest.json");
            return std::unexpected(text.error());
        }
        auto m = parse_manifest(std::string_view(reinterpret_cast<const char*>(text->data()), text->size()), options.limits.max_json_depth);
        if (!m) return std::unexpected(m.error());
        f.manifest_ = std::move(*m);

        std::map<std::string, Entry, std::less<>> records;
        auto ej = read_at(root.get(), std::string(entries_json), options.limits.max_unchunked_size);
        if (ej) {
            auto parsed = data::parse_json(std::string_view(reinterpret_cast<const char*>(ej->data()), ej->size()),
                                           {.maximum_depth = options.limits.max_json_depth, .ijson = true});
            if (!parsed) return fail(Code::json, "SQ-INF/entries.json: " + parsed.error.message);
            if (parsed.value.object_if() == nullptr) return fail(Code::json, "SQ-INF/entries.json is not an object");
            for (const auto& [path, v] : *parsed.value.object_if()) {
                if (!valid_utf8(path)) return fail(Code::path, "a name in SQ-INF/entries.json that is not UTF-8");
                auto e = exploded_record(v, nfc(path));
                if (!e) return std::unexpected(e.error());
                records.emplace(e->path, std::move(*e));
            }
        } else if (ej.error().code != Code::not_found) {
            return std::unexpected(ej.error());
        }

        fs::recursive_directory_iterator it(folder, fs::directory_options::none, ec), end;
        if (ec) return fail(Code::io, std::format("cannot read {}: {}", folder.string(), ec.message()));
        for (; it != end; it.increment(ec)) {
            if (ec) return fail(Code::io, std::format("cannot read {}: {}", folder.string(), ec.message()));
            std::string disk = it->path().lexically_relative(folder).generic_string();
            if (!valid_utf8(disk)) return fail(Code::path, "a name that is not UTF-8", disk);
            std::string name = nfc(disk);
            const auto status = it->symlink_status(ec);
            if (fs::is_symlink(status)) return fail(Code::path, "a symbolic link: refused, not followed (§6)", name);
            if (it.depth() == 0 && fs::is_directory(status) && name.starts_with(".")) {
                it.disable_recursion_pending();
                continue;
            }
            if (!fs::is_regular_file(status) || name == entries_json) continue;
            if (auto problem = path_problem(name, options.limits.max_path_bytes)) return fail(Code::path, *problem, name);
            if (f.entries_.size() >= options.limits.max_entries_per_index) return fail(Code::limit, "too many entries");
            Entry e;
            e.flags = flag::r;
            if (auto r = records.find(name); r != records.end()) e = r->second;
            e.path = name;
            e.size = static_cast<std::uint64_t>(fs::file_size(it->path(), ec));
            if (ec) return fail(Code::io, "cannot read its size", name);
            e.stored_size = e.size;
            f.entries_.push_back(std::move(e));
            f.disk_.push_back(std::move(disk));
        }
        // Sorted by path (as a packed cartridge's), with the names on disk alongside.
        std::vector<std::size_t> order(f.entries_.size());
        std::iota(order.begin(), order.end(), std::size_t{0});
        std::ranges::sort(order, {}, [&](std::size_t i) -> const std::string& { return f.entries_[i].path; });
        std::vector<Entry> entries;
        std::vector<std::string> disk;
        for (std::size_t i : order) {
            if (!entries.empty() && entries.back().path == f.entries_[i].path) {
                return fail(Code::path, "two names in the folder that are one in NFC", f.entries_[i].path);
            }
            entries.push_back(std::move(f.entries_[i]));
            disk.push_back(std::move(f.disk_[i]));
        }
        f.entries_ = std::move(entries);
        f.disk_ = std::move(disk);
        return f;
    }

    [[nodiscard]] const Manifest& manifest() const noexcept { return manifest_; }
    [[nodiscard]] const std::filesystem::path& root() const noexcept { return root_; }
    /// Every entry, sorted by path.
    [[nodiscard]] std::span<const Entry> entries() const noexcept { return entries_; }

    /// An entry by its path; Code::not_found when the folder has none.
    [[nodiscard]] Result<Entry> find(std::string_view path) const {
        auto it = std::ranges::lower_bound(entries_, path, {}, &Entry::path);
        if (it == entries_.end() || it->path != path) return fail(Code::not_found, "no such entry", std::string(path));
        return *it;
    }

    /// An entry's content, read without following a link.
    [[nodiscard]] Result<std::vector<std::byte>> read(const Entry& e) const {
        auto it = std::ranges::lower_bound(entries_, e.path, {}, &Entry::path);
        if (it == entries_.end() || it->path != e.path) return fail(Code::not_found, "no such entry", e.path);
        Fd root(::open(root_.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
        if (!root.ok()) return fail(Code::io, std::format("cannot open {}: {}", root_.string(), std::strerror(errno)), e.path);
        auto bytes = read_at(root.get(), disk_[static_cast<std::size_t>(it - entries_.begin())], options_.limits.max_entry_size);
        if (bytes && bytes->size() != it->size) return fail(Code::io, "it changed since the folder was opened", e.path);
        return bytes;
    }

private:
    static constexpr std::string_view manifest_file = "SQ-INF/manifest.json";

    /// A file under `root`, every folder on the way opened without
    /// following a link; Code::not_found when it is not there.
    static Result<std::vector<std::byte>> read_at(int root, const std::string& rel, std::uint64_t max_bytes) {
        Fd at(::dup(root));
        std::size_t from = 0;
        for (std::size_t slash; (slash = rel.find('/', from)) != std::string::npos; from = slash + 1) {
            const std::string name = rel.substr(from, slash - from);
            Fd next(::openat(at.get(), name.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
            if (!next.ok()) {
                if (errno == ENOENT) return fail(Code::not_found, "no such file", rel);
                return fail(Code::path, std::format("{} is not a folder this can read (a link?): {}", name, std::strerror(errno)), rel);
            }
            at = std::move(next);
        }
        Fd fd(::openat(at.get(), rel.substr(from).c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC));
        if (!fd.ok()) {
            if (errno == ENOENT) return fail(Code::not_found, "no such file", rel);
            return fail(Code::io, std::format("cannot open it (a link?): {}", std::strerror(errno)), rel);
        }
        struct ::stat st {};
        if (::fstat(fd.get(), &st) != 0 || !S_ISREG(st.st_mode)) return fail(Code::path, "not a regular file", rel);
        if (static_cast<std::uint64_t>(st.st_size) > max_bytes) return fail(Code::limit, "past the size limit", rel);
        std::vector<std::byte> out(static_cast<std::size_t>(st.st_size));
        std::size_t done = 0;
        while (done < out.size()) {
            const ssize_t n = ::read(fd.get(), out.data() + done, out.size() - done);
            if (n < 0 && errno == EINTR) continue;
            if (n < 0) return fail(Code::io, std::format("cannot read it: {}", std::strerror(errno)), rel);
            if (n == 0) break;
            done += static_cast<std::size_t>(n);
        }
        out.resize(done);
        return out;
    }

    std::filesystem::path root_;
    ReadOptions options_;
    Manifest manifest_;
    std::vector<Entry> entries_;
    std::vector<std::string> disk_;  ///< each entry's name on disk (NFD on a Mac)
};

struct UnpackOptions {
    /// Write into a folder that has files already (never over one).
    bool merge = false;
    /// Make an entry flagged x executable (mode 0755). Off by default: a
    /// cartridge from elsewhere never puts a program on the disk without
    /// being asked (§6). For one's own backup (a kept .git/ and its hooks).
    bool executable = false;
};

/// Unpack a cartridge into `target` (§6): every live entry a file, read
/// and verified first; SQ-INF/entries.json for metadata a file system
/// cannot hold, in canonical JSON. Refuses, before writing anything, a path
/// Windows cannot create. Never follows a link; never makes a file
/// executable unless asked (`executable`: an entry flagged x, 0755).
[[nodiscard]] Result<std::size_t> unpack(const Archive& cart, const std::filesystem::path& target, UnpackOptions options = {}) {
    auto live = cart.entries();
    if (!live) return std::unexpected(live.error());
    for (const Entry& e : *live) {
        for (std::size_t from = 0; from <= e.path.size();) {
            std::size_t slash = e.path.find('/', from);
            if (slash == std::string::npos) slash = e.path.size();
            if (reserved_on_windows(std::string_view(e.path).substr(from, slash - from))) {
                return fail(Code::path, "a name Windows cannot create (§4.1)", e.path);
            }
            from = slash + 1;
        }
        if (e.path == entries_json) return fail(Code::malformed, "SQ-INF/entries.json is the exploded form's, not an entry", e.path);
    }
    std::error_code ec;
    std::filesystem::create_directories(target, ec);
    Fd root(::open(target.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
    if (!root.ok()) return fail(Code::io, std::format("cannot open {}: {}", target.string(), std::strerror(errno)));
    if (!options.merge && std::filesystem::directory_iterator(target, ec) != std::filesystem::directory_iterator()) {
        return fail(Code::io, target.string() + " is not empty (merge to write into it)");
    }
    data::JsonValue::Object metadata;
    std::size_t written = 0;
    for (const Entry& e : *live) {
        const std::size_t slash = e.path.rfind('/');
        auto folder = open_folder(root.get(), slash == std::string::npos ? std::string_view() : std::string_view(e.path).substr(0, slash), e.path);
        if (!folder) return std::unexpected(folder.error());
        const ::mode_t mode = options.executable && (e.flags & flag::x) ? 0755 : 0644;
        if (auto r = write_entry(cart, e, folder->get(), e.path.substr(slash == std::string::npos ? 0 : slash + 1), mode); !r) {
            return std::unexpected(r.error());
        }
        if (auto m = exploded_metadata(e)) metadata.emplace(e.path, std::move(*m));
        ++written;
    }
    if (!metadata.empty()) {
        auto text = data::write_json(data::JsonValue(std::move(metadata)), {.canonical = true});
        if (!text) return fail(Code::json, text.error.message);
        auto folder = open_folder(root.get(), "SQ-INF", std::string(entries_json));
        if (!folder) return std::unexpected(folder.error());
        if (auto r = write_new(folder->get(), "entries.json", std::as_bytes(std::span(text.text)), std::string(entries_json)); !r) {
            return std::unexpected(r.error());
        }
    }
    return written;
}

}  // namespace sq::cart
