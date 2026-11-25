// SPDX-License-Identifier: MIT
//
// sq.cart:archive — a 3.0 cartridge opened, looked up, read and verified
// (docs/developer/specs/sq-cartridge-3.0.md). Section numbers are the
// spec's; Appendix A gives every layout this file reads.

export module sq.cart:archive;

import std;
import sq.data;
import :error;
import :bytes;
import :sha256;
import :json;
import :fold;
import :zstd;
import :ed25519;

export namespace sq::cart {

/// The format's magic, at offset 0 (§3.2).
inline constexpr std::array<std::uint8_t, 8> magic{0x89, 'S', 'Q', 'C', '\r', '\n', 0x1a, '\n'};
inline constexpr std::uint16_t major_version = 3;
inline constexpr std::uint64_t header_size = 8192;

/// Whether `head` starts with the 3.0 magic: the format is decided by this
/// alone (§3.3, §13).
[[nodiscard]] bool is_v3(std::span<const std::byte> head) noexcept {
    if (head.size() < magic.size()) return false;
    for (std::size_t i = 0; i < magic.size(); ++i) {
        if (std::to_integer<std::uint8_t>(head[i]) != magic[i]) return false;
    }
    return true;
}

/// §3.8, with the specified defaults. A host may lower them.
struct Limits {
    std::uint64_t max_entries_per_index = 0xffffffffu;
    std::uint32_t max_path_bytes = 1024;
    std::uint32_t max_string_bytes = 1u << 20;
    std::uint64_t max_entry_size = std::uint64_t{64} << 30;
    std::uint64_t max_unchunked_size = std::uint64_t{64} << 20;
    std::uint32_t max_delta_chain = 32;
    std::uint32_t max_json_depth = 64;
    std::uint32_t max_signatures = 16;
    /// The largest zstd window decoded, as a power of two: 23 (8 MiB) every
    /// reader supports; 27 also reads `zstd-window-27` cartridges.
    std::uint32_t max_window_log = 23;
};

struct ReadOptions {
    Limits limits{};
};

enum class Codec : std::uint16_t { stored = 0, zstd = 1, zstd_dict = 2, chunked = 3 };

/// Entry flags (§4.2).
namespace flag {
inline constexpr std::uint16_t r = 1u << 0;
inline constexpr std::uint16_t w = 1u << 1;
inline constexpr std::uint16_t x = 1u << 2;
inline constexpr std::uint16_t deleted = 1u << 3;
inline constexpr std::uint16_t mappable = 1u << 4;
inline constexpr std::uint16_t dictionary = 1u << 5;
inline constexpr std::uint16_t known = 0x3f;
}  // namespace flag

/// Feature bits of a commit (§3.9): the low 32 required, the high 32 optional.
namespace feature {
inline constexpr std::uint64_t zstd_dictionaries = 1u << 0;
inline constexpr std::uint64_t chunked = 1u << 1;
inline constexpr std::uint64_t zstd_window_27 = 1u << 2;
inline constexpr std::uint64_t known = 0x7;
}  // namespace feature

/// An entry record (§4.2, A.3), its strings resolved.
struct Entry {
    std::string path;
    std::optional<std::string> title;
    std::optional<std::string> author;
    std::optional<std::string> media_type;
    std::optional<std::string> attributes;  ///< the stored JSON text
    std::int64_t created = 0;
    std::int64_t updated = 0;
    std::uint16_t flags = 0;
    Codec codec = Codec::stored;
    std::uint32_t dictionary = 0;  ///< zstd dictionary ID (codec 2)
    std::uint64_t offset = 0;      ///< the run, or the chunk table (codec 3)
    std::uint64_t stored_size = 0;
    std::uint64_t size = 0;
    Digest digest{};               ///< the tree hash of §9.1
    std::uint64_t commit = 0;      ///< the sequence number of the commit whose index holds it

    [[nodiscard]] bool deleted() const noexcept { return (flags & flag::deleted) != 0; }
};

enum class IndexKind : std::uint32_t { full = 0, delta = 1 };

/// A commit record (§3.4, A.4).
struct Commit {
    std::uint64_t offset = 0;  ///< where the record is
    std::uint64_t sequence = 0;
    std::uint64_t previous = 0;  ///< the previous record's offset; 0 for the first
    std::uint64_t data_offset = 0, data_length = 0;
    std::uint64_t index_offset = 0, index_count = 0;
    std::uint64_t strings_offset = 0, strings_length = 0;
    std::uint64_t chunks_offset = 0, chunks_length = 0;
    IndexKind index_kind = IndexKind::full;
    std::uint32_t chain_length = 0;
    std::uint64_t live_count = 0;
    std::int64_t time = 0;
    std::uint64_t features = 0;
    Digest content_digest{};
    std::uint64_t signatures_offset = 0, signatures_length = 0;
};

/// What the manifest says that a reader needs (§7): the rest is consumers'.
struct Manifest {
    std::string id;
    std::string version;
    std::string kind;
    std::string json;  ///< the entry's text, as stored
    /// `case_sensitive: true` (§4.1): its paths may collide under case
    /// folding; it is for case-sensitive file systems.
    bool case_sensitive = false;
};

/// One signature (§9.3, A.5).
struct Signature {
    std::uint16_t algorithm = 0;  ///< 1: Ed25519
    std::vector<std::byte> public_key;
    std::array<std::byte, 8> key_id{};
    std::string signer;
    std::vector<std::byte> signature;
};

enum class SignatureState : std::uint8_t {
    valid,              ///< verifies over this commit's content digest
    invalid,            ///< does not verify, or its key or encoding is refused
    unknown_algorithm,  ///< one this reader lacks: reported, never trusted
};

struct SignatureCheck {
    Signature signature;
    SignatureState state = SignatureState::invalid;
    std::string why;
};

/// What `check` reports (§12): an error makes a cartridge non-conforming; a
/// warning is something a reader may ignore.
struct Finding {
    enum class Severity : std::uint8_t { error, warning };
    Severity severity = Severity::error;
    Code code = Code::malformed;
    std::string message;
    std::optional<std::string> entry;
};

struct CheckReport {
    std::vector<Finding> findings;
    std::optional<Commit> commit;
    std::size_t entries_checked = 0;
    std::size_t signatures_valid = 0;

    [[nodiscard]] bool ok() const noexcept {
        return std::ranges::none_of(findings, [](const Finding& f) { return f.severity == Finding::Severity::error; });
    }
};

class Archive;

/// One chunk of a chunked entry (§4.5, A.6).
struct Chunk {
    std::uint64_t offset = 0;
    std::uint32_t stored_size = 0;
    std::uint32_t size = 0;
    Codec codec = Codec::stored;
    std::uint32_t dictionary = 0;
    Digest sha256{};
    bool reserved_clear = true;
};

/// The message a signature signs (§9.3): "squared-cartridge/3 signature",
/// 0, the algorithm (u16), id, 0, version, 0, signer, 0, the content digest.
[[nodiscard]] std::vector<std::byte> signed_message(std::uint16_t algorithm, std::string_view id, std::string_view version,
                                                    std::string_view signer, const Digest& content_digest) {
    std::vector<std::byte> m;
    const auto text = [&](std::string_view s) {
        for (const char c : s) m.push_back(static_cast<std::byte>(c));
    };
    text("squared-cartridge/3 signature");
    m.push_back(std::byte{0});
    m.push_back(static_cast<std::byte>(algorithm & 0xff));
    m.push_back(static_cast<std::byte>(algorithm >> 8));
    text(id);
    m.push_back(std::byte{0});
    text(version);
    m.push_back(std::byte{0});
    text(signer);
    m.push_back(std::byte{0});
    m.insert(m.end(), content_digest.begin(), content_digest.end());
    return m;
}

/// The metadata object of §9.2 for an entry: title, author, media type,
/// created, updated, flags (letters, rwx) and attributes, absent ones left
/// out, in RFC 8785's canonical form (which orders members by name).
[[nodiscard]] Result<std::string> metadata_json(const Entry& e) {
    data::JsonValue::Object meta;
    if (e.attributes) {
        auto attributes = parse_ijson(*e.attributes, 64);
        if (!attributes) return std::unexpected(attributes.error());
        meta.emplace("attributes", std::move(*attributes));
    }
    if (e.author) meta.emplace("author", *e.author);
    meta.emplace("created", e.created);
    std::string letters;
    if (e.flags & flag::r) letters += 'r';
    if (e.flags & flag::w) letters += 'w';
    if (e.flags & flag::x) letters += 'x';
    meta.emplace("flags", std::move(letters));
    if (e.media_type) meta.emplace("media_type", *e.media_type);
    if (e.title) meta.emplace("title", *e.title);
    meta.emplace("updated", e.updated);
    return write_canonical(data::JsonValue(std::move(meta)));
}

/// The content digest of a state (§9.2): its live entries, sorted by path.
[[nodiscard]] Result<Digest> content_digest(std::span<const Entry> live) {
    Sha256 h;
    const auto field = [&](std::span<const std::byte> bytes) {
        std::array<std::byte, 4> le{};
        for (std::size_t i = 0; i < 4; ++i) le[i] = static_cast<std::byte>(bytes.size() >> (8 * i));
        h.update(le);
        h.update(bytes);
    };
    h.update(std::string_view("squared-cartridge/3 state\0", 26));
    std::array<std::byte, 8> count{};
    for (std::size_t i = 0; i < 8; ++i) count[i] = static_cast<std::byte>(std::uint64_t{live.size()} >> (8 * i));
    h.update(count);
    for (const Entry& e : live) {
        auto meta = metadata_json(e);
        if (!meta) {
            auto err = meta.error();
            err.entry = e.path;
            return std::unexpected(err);
        }
        field(std::as_bytes(std::span(e.path)));
        field(e.digest);
        field(std::as_bytes(std::span(*meta)));
    }
    return h.finish();
}

/// A manifest's text read as §7 asks: I-JSON, format squared-cartridge,
/// format_version 3, an id, a version and a kind. What a reader needs of
/// it; the rest is its consumers'. For a packed cartridge and a folder.
[[nodiscard]] Result<Manifest> parse_manifest(std::string_view text, std::uint32_t max_json_depth = 64) {
    constexpr std::string_view at = "SQ-INF/manifest.json";
    auto json = parse_ijson(text, max_json_depth);
    if (!json) {
        auto err = json.error();
        err.entry = std::string(at);
        return std::unexpected(err);
    }
    if (json->object_if() == nullptr) return fail(Code::malformed, "the manifest is not a JSON object", std::string(at));
    const data::JsonValue* format_version = json->find("format_version");
    const bool three = format_version != nullptr &&
                       ((format_version->unsigned_integer_if() && *format_version->unsigned_integer_if() == 3) ||
                        (format_version->signed_integer_if() && *format_version->signed_integer_if() == 3));
    if (json_string_member(*json, "format") != "squared-cartridge") {
        return fail(Code::malformed, "the manifest's format is not squared-cartridge", std::string(at));
    }
    if (!three) return fail(Code::unsupported, "the manifest's format_version is not 3", std::string(at));
    auto id = json_string_member(*json, "id");
    auto version = json_string_member(*json, "version");
    auto kind = json_string_member(*json, "kind");
    if (!id || !version || !kind) return fail(Code::malformed, "the manifest has no id, version or kind", std::string(at));
    Manifest m{std::move(*id), std::move(*version), std::move(*kind), std::string(text)};
    if (const data::JsonValue* cs = json->find("case_sensitive")) {
        if (cs->boolean_if() == nullptr) return fail(Code::malformed, "the manifest's case_sensitive is not true or false", std::string(at));
        m.case_sensitive = *cs->boolean_if();
    }
    return m;
}

/// Whether a path follows §4.1 (as 2.0 §3.3): relative, '/'-separated, no
/// '.', '..' or empty segment, no NUL, backslash, ':' or control character,
/// within `max_bytes`, valid UTF-8. The reason when it does not.
[[nodiscard]] std::optional<std::string> path_problem(std::string_view path, std::uint32_t max_bytes) {
    if (path.empty()) return "an empty path";
    if (path.size() > max_bytes) return std::format("a path of {} bytes, past {}", path.size(), max_bytes);
    if (!valid_utf8(path)) return "a path that is not valid UTF-8";
    if (!is_nfc(path)) return "a path not in Normalization Form C (a Mac's decomposed names: pack converts them)";
    if (path.front() == '/') return "an absolute path";
    for (std::size_t i = 0; i < path.size(); ++i) {
        const auto c = static_cast<unsigned char>(path[i]);
        if (c < 0x20 || c == 0x7f) return "a control character in a path";
        if (c == '\\' || c == ':') return std::format("'{}' in a path", static_cast<char>(c));
        // C1 controls: U+0080..U+009F, UTF-8 C2 80..C2 9F.
        if (c == 0xc2 && i + 1 < path.size() && static_cast<unsigned char>(path[i + 1]) <= 0x9f) {
            return "a control character in a path";
        }
    }
    for (std::size_t at = 0; at <= path.size();) {
        std::size_t slash = path.find('/', at);
        if (slash == std::string_view::npos) slash = path.size();
        const std::string_view segment = path.substr(at, slash - at);
        if (segment.empty() || segment == "." || segment == "..") return "an empty, '.' or '..' segment in a path";
        at = slash + 1;
    }
    return std::nullopt;
}

}  // namespace sq::cart

namespace sq::cart {

constexpr std::uint64_t slot_a = 64;
constexpr std::uint64_t slot_b = 4096;
constexpr std::size_t record_size = 128;   // A.3
constexpr std::size_t commit_size = 224;   // A.4
constexpr std::size_t chunk_record = 64;   // A.6
constexpr std::uint32_t no_string = 0xffffffffu;
constexpr std::uint64_t mappable_alignment = 16384;
constexpr std::string_view manifest_path = "SQ-INF/manifest.json";
constexpr std::string_view dictionary_dir = "SQ-INF/dictionaries/";

struct Slot {
    std::uint64_t sequence = 0, record = 0, length = 0, file_length = 0;
    bool valid = false;
    std::string why;
};

struct Region {
    std::uint64_t offset, length;
    std::string_view name;
};

/// One commit as the reader walks it: its record, and its index and string
/// table, in place in a mapped file where the source allows (else copied).
struct Loaded {
    Commit commit;
    std::span<const std::byte> index, strings;
    std::vector<std::byte> owned_index, owned_strings;
};

/// A region of one commit (§3.4), for the bounds runs must keep (§3.6).
struct CommitRegion {
    std::uint64_t offset, length, sequence;
};

[[nodiscard]] Slot read_slot(std::span<const std::byte> header, std::uint64_t at, std::uint64_t file_size) {
    const std::byte* p = header.data() + at;
    Slot s;
    s.sequence = u64(p);
    s.record = u64(p + 8);
    s.length = u64(p + 16);
    s.file_length = u64(p + 24);
    const Digest sum = sha256(std::span(p, 56));
    if (all_zero(std::span(p, 64))) {
        s.why = "it is empty";  // a new cartridge's spare slot
    } else if (!std::equal(sum.begin(), sum.begin() + 8, p + 56)) {
        s.why = "its checksum fails";
    } else if (s.sequence == 0) {
        s.why = "it is empty";
    } else if (s.length != commit_size) {
        s.why = std::format("it names a commit record of {} bytes", s.length);
    } else if (const auto end = add(s.record, s.length); !end || *end > file_size) {
        s.why = "its commit record lies outside the file";
    } else if (*end != s.file_length) {
        s.why = "its file length is not its commit record's end";
    } else if (s.record < header_size) {
        s.why = "its commit record is inside the header";
    } else {
        s.valid = true;
    }
    return s;
}

/// A commit record at `offset`, its own checks only (§3.4); what it must
/// satisfy against its predecessor is checked as the chain is walked.
[[nodiscard]] Result<Commit> read_commit(const Source& source, std::uint64_t offset, const Limits& limits) {
    auto bytes = source.read(offset, commit_size);
    if (!bytes) return std::unexpected(bytes.error());
    const std::byte* p = bytes->data();
    const std::string at = std::format("the commit record at {}", offset);
    if (!std::equal(p, p + 8, reinterpret_cast<const std::byte*>("SQCOMMIT"))) {
        return fail(Code::malformed, at + " has no SQCOMMIT magic");
    }
    const Digest sum = sha256(std::span(p, 192));
    if (!std::equal(sum.begin(), sum.end(), p + 192)) return fail(Code::malformed, at + " fails its SHA-256");
    Commit c;
    c.offset = offset;
    c.sequence = u64(p + 8);
    c.previous = u64(p + 16);
    c.data_offset = u64(p + 24), c.data_length = u64(p + 32);
    c.index_offset = u64(p + 40), c.index_count = u64(p + 48);
    c.strings_offset = u64(p + 56), c.strings_length = u64(p + 64);
    c.chunks_offset = u64(p + 72), c.chunks_length = u64(p + 80);
    const std::uint32_t kind = u32(p + 88);
    c.chain_length = u32(p + 92);
    c.live_count = u64(p + 96);
    c.time = i64(p + 104);
    c.features = u64(p + 112);
    std::copy(p + 120, p + 152, c.content_digest.begin());
    c.signatures_offset = u64(p + 152), c.signatures_length = u64(p + 160);

    if (kind > 1) return fail(Code::malformed, std::format("{} has index kind {}", at, kind));
    c.index_kind = static_cast<IndexKind>(kind);
    if (c.index_kind == IndexKind::full && c.chain_length != 0) return fail(Code::malformed, at + ": a full index with a delta chain length");
    if (c.index_kind == IndexKind::delta && c.chain_length == 0) return fail(Code::malformed, at + ": a delta index with no chain length");
    if (c.chain_length > limits.max_delta_chain) {
        return fail(Code::limit, std::format("{}: a delta chain of {}, past {}", at, c.chain_length, limits.max_delta_chain));
    }
    if (c.index_kind == IndexKind::delta && c.previous == 0) return fail(Code::malformed, at + ": a delta index with no previous commit");
    if (c.index_count > limits.max_entries_per_index) return fail(Code::limit, std::format("{}: {} entries in one index", at, c.index_count));
    if (const std::uint64_t unknown = c.features & 0xffffffffu & ~feature::known; unknown != 0) {
        return fail(Code::unsupported, std::format("{} requires features this reader lacks (bits {:#x})", at, unknown));
    }
    if ((c.features & feature::zstd_window_27) && limits.max_window_log < 27) {
        return fail(Code::unsupported, at + " needs zstd windows up to 2^27 (zstd-window-27); this reader allows 2^" +
                                           std::to_string(limits.max_window_log));
    }

    // Regions: after the previous record (or the header), before this one,
    // and apart from each other.
    const std::uint64_t floor = c.previous == 0 ? header_size : c.previous + commit_size;
    if (c.previous != 0 && (c.previous < header_size || c.previous + commit_size > offset)) {
        return fail(Code::malformed, at + "'s previous commit is not before it");
    }
    const auto index_bytes = mul(c.index_count, record_size);
    if (!index_bytes) return fail(Code::malformed, at + ": an index too large to address");
    std::vector<Region> regions{{c.data_offset, c.data_length, "data"},
                                {c.index_offset, *index_bytes, "index"},
                                {c.strings_offset, c.strings_length, "string table"},
                                {c.chunks_offset, c.chunks_length, "chunk tables"},
                                {c.signatures_offset, c.signatures_length, "signature block"}};
    for (const Region& r : regions) {
        if (r.length == 0) continue;
        const auto end = add(r.offset, r.length);
        if (r.offset < floor || !end || *end > offset) {
            return fail(Code::malformed, std::format("{}: its {} lies outside the commit", at, r.name));
        }
    }
    for (std::size_t i = 0; i < regions.size(); ++i) {
        for (std::size_t j = i + 1; j < regions.size(); ++j) {
            const Region& a = regions[i];
            const Region& b = regions[j];
            if (a.length == 0 || b.length == 0) continue;
            if (a.offset < b.offset + b.length && b.offset < a.offset + a.length) {
                return fail(Code::malformed, std::format("{}: its {} and {} overlap", at, a.name, b.name));
            }
        }
    }
    return c;
}

[[nodiscard]] Result<std::optional<std::string>> string_at(const Loaded& l, std::uint32_t offset, std::uint32_t max_bytes,
                                                           std::string_view what) {
    if (offset == no_string) return std::optional<std::string>{};
    const auto& t = l.strings;
    if (std::uint64_t{offset} + 4 > t.size()) return fail(Code::malformed, std::format("a {} outside its string table", what));
    const std::uint32_t n = u32(t.data() + offset);
    if (n > max_bytes) return fail(Code::limit, std::format("a {} of {} bytes", what, n));
    if (std::uint64_t{offset} + 4 + n > t.size()) return fail(Code::malformed, std::format("a {} running past its string table", what));
    std::string s(reinterpret_cast<const char*>(t.data() + offset + 4), n);
    if (!valid_utf8(s)) return fail(Code::malformed, std::format("a {} that is not valid UTF-8", what));
    return std::optional<std::string>(std::move(s));
}

/// Entry `i` of a commit's index, its strings resolved and its fields
/// checked against §4.2 and §4.3.
[[nodiscard]] Result<Entry> entry_at(const Loaded& l, std::uint64_t i, const Limits& limits) {
    const std::byte* p = l.index.data() + i * record_size;
    Entry e;
    auto path = string_at(l, u32(p), limits.max_path_bytes, "path");
    if (!path) return std::unexpected(path.error());
    if (!*path) return fail(Code::malformed, std::format("entry {} of commit {} has no path", i, l.commit.sequence));
    e.path = std::move(**path);
    const auto field = [&](std::uint32_t at, std::optional<std::string>& into, std::string_view what) -> Result<void> {
        auto s = string_at(l, at, limits.max_string_bytes, what);
        if (!s) {
            auto err = s.error();
            err.entry = e.path;
            return std::unexpected(err);
        }
        into = std::move(*s);
        return {};
    };
    if (auto r = field(u32(p + 4), e.title, "title"); !r) return std::unexpected(r.error());
    if (auto r = field(u32(p + 8), e.author, "author"); !r) return std::unexpected(r.error());
    if (auto r = field(u32(p + 12), e.media_type, "media type"); !r) return std::unexpected(r.error());
    if (auto r = field(u32(p + 16), e.attributes, "attributes object"); !r) return std::unexpected(r.error());
    e.flags = u16(p + 20);
    const std::uint16_t codec = u16(p + 22);
    e.dictionary = u32(p + 24);
    e.offset = u64(p + 32);
    e.stored_size = u64(p + 40);
    e.size = u64(p + 48);
    e.created = i64(p + 56);
    e.updated = i64(p + 64);
    std::copy(p + 72, p + 104, e.digest.begin());
    e.commit = l.commit.sequence;

    const auto bad = [&](std::string why) { return fail(Code::malformed, std::move(why), e.path); };
    if (auto problem = path_problem(e.path, limits.max_path_bytes)) return fail(Code::path, *problem, e.path);
    if ((e.flags & ~flag::known) != 0) return bad(std::format("reserved flag bits set ({:#x})", e.flags));
    if (codec > 3) return fail(Code::unsupported, std::format("codec {}", codec), e.path);
    e.codec = static_cast<Codec>(codec);
    if (e.deleted()) {
        if (e.title || e.author || e.media_type || e.attributes || codec != 0 || e.offset != 0 || e.size != 0 ||
            e.stored_size != 0 || e.dictionary != 0 || !all_zero(e.digest)) {
            return bad("a tombstone with content or metadata");
        }
        return e;
    }
    if (e.size > limits.max_entry_size) return fail(Code::limit, std::format("an entry of {} bytes", e.size), e.path);
    if (e.size == 0 && (e.codec != Codec::stored || e.offset != 0 || e.stored_size != 0)) {
        return bad("an empty entry with a run");
    }
    if (e.codec != Codec::zstd_dict && e.dictionary != 0) return bad("a dictionary ID on an entry that uses none");
    if (e.codec == Codec::zstd_dict && e.dictionary == 0) return bad("codec 2 with no dictionary ID");
    if (e.codec == Codec::stored && e.stored_size != e.size) return bad("a stored entry whose stored size is not its size");
    if (e.codec != Codec::chunked && e.size > limits.max_unchunked_size) {
        return bad(std::format("an unchunked entry of {} bytes (past {}, it must be chunked)", e.size, limits.max_unchunked_size));
    }
    if (e.codec == Codec::chunked) {
        const std::uint64_t chunks = (e.size + chunk_size - 1) / chunk_size;
        if (e.size == 0 || e.stored_size != chunks * chunk_record) return bad("a chunk table of the wrong size");
    }
    if ((e.flags & flag::mappable) && (e.codec != Codec::stored || e.offset % mappable_alignment != 0)) {
        return bad("a mappable entry that is not stored at a 16384-byte boundary");
    }
    if (e.flags & flag::dictionary) {
        if ((e.flags & flag::r) || (e.codec != Codec::stored && e.codec != Codec::zstd)) {
            return bad("a dictionary entry that is readable, or not stored or zstd");
        }
        if (!e.path.starts_with(dictionary_dir) || !e.path.ends_with(".zdict")) {
            return bad("a dictionary outside SQ-INF/dictionaries/<id>.zdict");
        }
    }
    if (e.codec != Codec::stored && e.codec != Codec::chunked && e.size > 0) {
        // A frame is never much larger than what it holds: zstd's bound.
        if (e.stored_size > e.size + e.size / 128 + 1024) return bad("a zstd run larger than its content could need");
    }
    return e;
}

[[nodiscard]] std::string_view path_view(const Loaded& l, std::uint64_t i) {
    const std::byte* p = l.index.data() + i * record_size;
    const std::uint32_t at = u32(p);
    if (std::uint64_t{at} + 4 > l.strings.size()) return {};
    const std::uint32_t n = u32(l.strings.data() + at);
    if (std::uint64_t{at} + 4 + n > l.strings.size()) return {};
    return {reinterpret_cast<const char*>(l.strings.data() + at + 4), n};
}

}  // namespace sq::cart

export namespace sq::cart {

/// An opened 3.0 cartridge, at the commit its superblocks name (§3.3).
/// Immutable once open: concurrent const use is safe. Move-only.
///
/// Open reads the header, the superblocks, the commit records down to the
/// full index, and the manifest; its indexes and string tables are mapped,
/// not read (§12). Every record find() returns is checked when it is
/// returned, and every byte read is checked against its digest. The whole
/// state (every index's order, every record, the live count, collisions,
/// the features) is checked the first time something needs all of it:
/// entries(), the content digest, check().
class Archive {
public:
    [[nodiscard]] static Result<Archive> open(const std::filesystem::path& path, const ReadOptions& options = {}) {
        auto source = FileSource::open(path);
        if (!source) return std::unexpected(source.error());
        return open_source(std::move(*source), options);
    }
    /// The span must outlive the Archive.
    [[nodiscard]] static Result<Archive> open_memory(std::span<const std::byte> bytes, const ReadOptions& options = {}) {
        return open_source(std::make_unique<MemorySource>(bytes), options);
    }

    [[nodiscard]] const Commit& commit() const noexcept { return chain_.front().commit; }
    /// The commits a lookup walks, newest first, down to the full index.
    [[nodiscard]] std::vector<Commit> chain() const {
        std::vector<Commit> out;
        for (const Loaded& l : chain_) out.push_back(l.commit);
        return out;
    }
    [[nodiscard]] std::uint64_t file_size() const noexcept { return source_->size(); }
    /// Bytes after the current commit (§3.3): ignored, and reported by check.
    [[nodiscard]] std::uint64_t trailing_bytes() const noexcept {
        return source_->size() - (commit().offset + commit_size);
    }
    [[nodiscard]] const Manifest& manifest() const noexcept { return manifest_; }

    /// A live entry (§3.5), its record checked; Code::not_found when there
    /// is none (or it was deleted), another error when its record is broken.
    [[nodiscard]] Result<Entry> find(std::string_view path) const {
        for (const Loaded& l : chain_) {
            std::uint64_t lo = 0, hi = l.commit.index_count;
            while (lo < hi) {
                const std::uint64_t mid = lo + (hi - lo) / 2;
                const std::string_view p = path_view(l, mid);
                if (p == path) {
                    auto e = entry_at(l, mid, options_.limits);
                    if (!e) return e;
                    if (e->deleted()) {
                        if (l.commit.index_kind == IndexKind::full) return fail(Code::malformed, "a tombstone in a full index", e->path);
                        return fail(Code::not_found, "deleted", std::string(path));
                    }
                    if (auto r = check_run(*e); !r) return std::unexpected(r.error());
                    return e;
                }
                if (p < path) {
                    lo = mid + 1;
                } else {
                    hi = mid;
                }
            }
        }
        return fail(Code::not_found, "no such entry", std::string(path));
    }

    /// Every live entry, sorted by path: the whole state, checked the first
    /// time it is asked for (§3.5, §4.1, §3.9), then kept.
    [[nodiscard]] Result<std::span<const Entry>> entries() const {
        std::call_once(lazy_->live_once, [this] { lazy_->live = load_live(); });
        if (!lazy_->live) return std::unexpected(lazy_->live.error());
        return std::span<const Entry>(*lazy_->live);
    }

    /// An entry's content, all of it, verified against its digest before it
    /// is returned (§9.1).
    [[nodiscard]] Result<std::vector<std::byte>> read(const Entry& e) const { return read_range(e, 0, e.size); }

    /// Bytes [offset, offset + length) of an entry, verified (§9.1): a
    /// chunked entry's chunk table is checked against the digest and each
    /// chunk read against its hash; anything else is read and checked whole.
    [[nodiscard]] Result<std::vector<std::byte>> read_range(const Entry& e, std::uint64_t offset, std::uint64_t length) const {
        if (e.deleted()) return fail(Code::not_found, "a deleted entry", e.path);
        const auto end = add(offset, length);
        if (!end || *end > e.size) return fail(Code::not_found, "a range past the end of the entry", e.path);
        if (e.codec != Codec::chunked) {
            auto all = decode_run(e.offset, e.stored_size, e.size, e.codec, e.dictionary, e.path);
            if (!all) return all;
            if (tree_hash(*all) != e.digest) return fail(Code::integrity, "the content does not match its digest", e.path);
            if (offset == 0 && length == e.size) return all;
            return std::vector<std::byte>(all->begin() + static_cast<std::ptrdiff_t>(offset),
                                          all->begin() + static_cast<std::ptrdiff_t>(offset + length));
        }
        auto chunks = chunk_table(e);
        if (!chunks) return std::unexpected(chunks.error());
        std::vector<std::byte> out;
        out.reserve(static_cast<std::size_t>(length));
        for (std::uint64_t at = offset; at < offset + length;) {
            const std::uint64_t i = at / chunk_size;
            const Chunk& c = (*chunks)[static_cast<std::size_t>(i)];
            const std::string what = std::format("{} (chunk {})", e.path, i);
            auto bytes = decode_run(c.offset, c.stored_size, c.size, c.codec, c.dictionary, what);
            if (!bytes) return bytes;
            if (sha256(*bytes) != c.sha256) return fail(Code::integrity, std::format("chunk {} does not match its hash", i), e.path);
            const std::uint64_t from = at - i * chunk_size;
            const std::uint64_t take = std::min<std::uint64_t>(c.size - from, offset + length - at);
            out.insert(out.end(), bytes->begin() + static_cast<std::ptrdiff_t>(from),
                       bytes->begin() + static_cast<std::ptrdiff_t>(from + take));
            at += take;
        }
        return out;
    }

    /// The content digest of the current state, from the live entries
    /// (§9.2): compare with commit().content_digest.
    [[nodiscard]] Result<Digest> compute_content_digest() const {
        auto live = entries();
        if (!live) return std::unexpected(live.error());
        return content_digest(*live);
    }

    /// A chunked entry's chunks (§4.5), its table checked against its digest.
    [[nodiscard]] Result<std::vector<Chunk>> chunks(const Entry& e) const {
        if (e.codec != Codec::chunked) return fail(Code::malformed, "not a chunked entry", e.path);
        return chunk_table(e);
    }

    /// The current commit's signatures, each verified (§9.3); empty when it
    /// carries none.
    [[nodiscard]] Result<std::vector<SignatureCheck>> signatures() const {
        std::vector<SignatureCheck> out;
        const Commit& c = commit();
        if (c.signatures_length == 0) return out;
        auto block = source_->read(c.signatures_offset, c.signatures_length);
        if (!block) return std::unexpected(block.error());
        std::span<const std::byte> b(*block);
        const auto take = [&](std::size_t n) -> std::optional<std::span<const std::byte>> {
            if (b.size() < n) return std::nullopt;
            auto head = b.first(n);
            b = b.subspan(n);
            return head;
        };
        const auto truncated = [] { return fail(Code::malformed, "a truncated signature block"); };
        auto count = take(4);
        if (!count) return truncated();
        const std::uint32_t n = u32(count->data());
        if (n > options_.limits.max_signatures) return fail(Code::limit, std::format("{} signatures in one block", n));
        for (std::uint32_t i = 0; i < n; ++i) {
            SignatureCheck s;
            auto head = take(4);
            if (!head) return truncated();
            s.signature.algorithm = u16(head->data());
            const std::uint16_t key_length = u16(head->data() + 2);
            auto key = take(key_length);
            auto id = key ? take(8) : std::nullopt;
            auto name_length = id ? take(4) : std::nullopt;
            if (!name_length) return truncated();
            const std::uint32_t nl = u32(name_length->data());
            if (nl > options_.limits.max_string_bytes) return fail(Code::limit, "a signer name too long");
            auto name = take(nl);
            auto sig_length = name ? take(2) : std::nullopt;
            if (!sig_length) return truncated();
            auto sig = take(u16(sig_length->data()));
            if (!sig) return truncated();
            s.signature.public_key.assign(key->begin(), key->end());
            std::ranges::copy(*id, s.signature.key_id.begin());
            s.signature.signer.assign(reinterpret_cast<const char*>(name->data()), name->size());
            s.signature.signature.assign(sig->begin(), sig->end());
            verify(s);
            out.push_back(std::move(s));
        }
        if (!b.empty()) return fail(Code::malformed, "bytes after the last signature in its block");
        return out;
    }

    Archive(Archive&&) noexcept = default;
    Archive& operator=(Archive&&) noexcept = default;

private:
    Archive() = default;

    static Result<Archive> open_source(std::unique_ptr<Source> source, const ReadOptions& options) {
        Archive a;
        a.source_ = std::move(source);
        a.options_ = options;
        if (auto r = a.load(); !r) return std::unexpected(r.error());
        return a;
    }

    Result<void> load() {
        const Limits& limits = options_.limits;
        // 1. The header (§3.2).
        if (source_->size() < header_size) return fail(Code::malformed, "shorter than a 3.0 header");
        auto header = source_->read(0, header_size);
        if (!header) return std::unexpected(header.error());
        header_ = std::move(*header);
        if (!is_v3(header_)) return fail(Code::malformed, "not a 3.0 cartridge (no magic)");
        const std::uint16_t major = u16(header_.data() + 8);
        if (major != major_version) return fail(Code::unsupported, std::format("format major version {}", major));
        if (u32(header_.data() + 12) != header_size) return fail(Code::malformed, "a header size other than 8192");

        // 2. The superblocks; the newer valid one, else the other (§3.3).
        slots_ = {read_slot(header_, slot_a, source_->size()), read_slot(header_, slot_b, source_->size())};
        std::vector<const Slot*> order;
        for (const Slot& s : slots_) {
            if (s.valid) order.push_back(&s);
        }
        std::ranges::sort(order, std::greater{}, &Slot::sequence);
        std::string why = std::format("slot A: {}; slot B: {}", slots_[0].valid ? "valid" : slots_[0].why,
                                      slots_[1].valid ? "valid" : slots_[1].why);
        std::optional<Commit> current;
        for (const Slot* s : order) {
            auto c = read_commit(*source_, s->record, limits);
            if (c && c->sequence == s->sequence) {
                current = *c;
                break;
            }
            // Fall back only past damage. A commit this reader cannot or
            // may not read (a feature it lacks, a limit) is refused as it
            // is: reading the older one instead would show a state the
            // cartridge has replaced.
            if (!c && (c.error().code == Code::unsupported || c.error().code == Code::limit)) return std::unexpected(c.error());
            why += std::format("; the commit slot {} names: {}", s == &slots_[0] ? 'A' : 'B',
                               c ? "its sequence number differs" : c.error().message);
        }
        if (!current) return fail(Code::malformed, "no superblock names a valid commit (" + why + ")");

        // 3. The chain, newest first, down to a full index (§3.5): records,
        // and their indexes and string tables in place.
        std::optional<Commit> c = current;
        while (true) {
            Loaded l;
            l.commit = *c;
            if (auto r = map_index(l); !r) return r;
            chain_.push_back(std::move(l));
            if (c->index_kind == IndexKind::full) break;
            auto prev = read_commit(*source_, c->previous, limits);
            if (!prev) return std::unexpected(prev.error());
            if (prev->sequence >= c->sequence) return fail(Code::malformed, "a commit whose predecessor does not come before it");
            const std::uint32_t expected = prev->index_kind == IndexKind::full ? 1 : prev->chain_length + 1;
            if (c->chain_length != expected) return fail(Code::malformed, "a delta chain length that does not follow its predecessor's");
            c = *prev;
        }

        // 4. The manifest (§7).
        return load_manifest();
    }

    /// The whole state (§3.5): every index sorted and unique, every record
    /// checked, the deltas applied oldest first, the live count, the case
    /// collisions (§4.1), the features (§3.9).
    Result<std::vector<Entry>> load_live() const {
        const Limits& limits = options_.limits;
        std::map<std::string, Entry, std::less<>> live;
        for (auto it = chain_.rbegin(); it != chain_.rend(); ++it) {
            if (auto r = check_order(*it); !r) return std::unexpected(r.error());
            for (std::uint64_t i = 0; i < it->commit.index_count; ++i) {
                auto e = entry_at(*it, i, limits);
                if (!e) return std::unexpected(e.error());
                if (it->commit.index_kind == IndexKind::full && e->deleted()) return fail(Code::malformed, "a tombstone in a full index", e->path);
                if (e->deleted()) {
                    live.erase(e->path);
                } else {
                    if (auto r = check_run(*e); !r) return std::unexpected(r.error());
                    live.insert_or_assign(e->path, std::move(*e));
                }
            }
        }
        const Commit& current = commit();
        if (live.size() != current.live_count) {
            return fail(Code::malformed, std::format("{} live entries, where the commit says {}", live.size(), current.live_count));
        }
        std::vector<Entry> out;
        out.reserve(live.size());
        for (auto& [path, e] : live) out.push_back(std::move(e));
        std::map<std::u32string, std::string_view> keys;
        for (const Entry& e : out) {
            if (manifest_.case_sensitive) break;
            auto [it, fresh] = keys.try_emplace(collision_key(e.path), e.path);
            if (!fresh) return fail(Code::path, std::format("collides with \"{}\" on a case-insensitive file system", it->second), e.path);
        }
        std::uint64_t uses = 0;
        for (const Entry& e : out) {
            if (e.codec == Codec::zstd_dict) uses |= feature::zstd_dictionaries;
            if (e.codec == Codec::chunked) uses |= feature::chunked;
        }
        if ((uses & ~current.features & 0x3) != 0) return fail(Code::malformed, "the state uses a feature its commit does not declare");
        return out;
    }

    /// A commit's index and string table, in place where the source allows.
    Result<void> map_index(Loaded& l) const {
        const Commit& c = l.commit;
        const std::uint64_t index_bytes = c.index_count * record_size;
        if (auto v = source_->view(c.index_offset, index_bytes)) {
            l.index = *v;
        } else {
            auto index = source_->read(c.index_offset, index_bytes);
            if (!index) return std::unexpected(index.error());
            l.owned_index = std::move(*index);
            l.index = l.owned_index;
        }
        if (auto v = source_->view(c.strings_offset, c.strings_length)) {
            l.strings = *v;
        } else {
            auto strings = source_->read(c.strings_offset, c.strings_length);
            if (!strings) return std::unexpected(strings.error());
            l.owned_strings = std::move(*strings);
            l.strings = l.owned_strings;
        }
        return {};
    }

    /// Sorted by path, byte-wise, each path once (§3.5).
    Result<void> check_order(const Loaded& l) const {
        std::string_view last;
        for (std::uint64_t i = 0; i < l.commit.index_count; ++i) {
            const std::string_view p = path_view(l, i);
            if (p.empty()) return fail(Code::malformed, std::format("entry {} of commit {} has no readable path", i, l.commit.sequence));
            if (i > 0 && !(last < p)) {
                return fail(Code::malformed, std::format("the index of commit {} is not sorted, or names a path twice", l.commit.sequence),
                            std::string(p));
            }
            last = p;
        }
        return {};
    }

    /// Every commit's data and chunk-table regions, from the current one to
    /// the first, their records read and checked (§3.4). Walked once, when a
    /// run is first checked.
    Result<void> walk_regions() const {
        std::call_once(lazy_->regions_once, [this] {
            std::vector<CommitRegion> data, chunks;
            std::optional<Commit> c = commit();
            std::uint64_t steps = 0;
            while (c) {
                if (c->data_length) data.push_back({c->data_offset, c->data_length, c->sequence});
                if (c->chunks_length) chunks.push_back({c->chunks_offset, c->chunks_length, c->sequence});
                if (c->previous == 0) break;
                if (++steps > (std::uint64_t{1} << 32)) break;
                auto prev = read_commit(*source_, c->previous, options_.limits);
                if (!prev) {
                    lazy_->regions_error = prev.error();
                    return;
                }
                if (prev->sequence >= c->sequence) {
                    lazy_->regions_error = Error{Code::malformed, "a commit whose predecessor does not come before it", std::nullopt};
                    return;
                }
                c = *prev;
            }
            lazy_->data_regions = std::move(data);
            lazy_->chunk_regions = std::move(chunks);
        });
        if (lazy_->regions_error) return std::unexpected(*lazy_->regions_error);
        return {};
    }

    /// Whether [offset, offset + length) lies wholly inside one region of a
    /// commit no later than `sequence` (§3.6: a run in data its own commit or
    /// an earlier one wrote; a chunk table in a chunk-table region).
    static bool inside(const std::vector<CommitRegion>& regions, std::uint64_t offset, std::uint64_t length, std::uint64_t sequence) {
        if (length == 0) return true;
        const auto end = add(offset, length);
        if (!end) return false;
        return std::ranges::any_of(regions, [&](const CommitRegion& r) {
            return r.sequence <= sequence && offset >= r.offset && *end <= r.offset + r.length;
        });
    }

    Result<void> check_run(const Entry& e) const {
        if (auto r = walk_regions(); !r) return std::unexpected(Error{r.error().code, r.error().message, e.path});
        const bool ok = e.codec == Codec::chunked ? inside(lazy_->chunk_regions, e.offset, e.stored_size, e.commit)
                                                  : inside(lazy_->data_regions, e.offset, e.stored_size, e.commit);
        if (!ok) return fail(Code::malformed, e.codec == Codec::chunked ? "its chunk table lies outside the chunk-table regions"
                                                                        : "its run lies outside the data regions", e.path);
        return {};
    }

    Result<void> load_manifest() {
        const auto e = find(manifest_path);
        if (!e) {
            if (e.error().code == Code::not_found) return fail(Code::malformed, "no live SQ-INF/manifest.json");
            return std::unexpected(e.error());
        }
        auto bytes = read(*e);
        if (!bytes) return std::unexpected(bytes.error());
        auto m = parse_manifest(std::string_view(reinterpret_cast<const char*>(bytes->data()), bytes->size()), options_.limits.max_json_depth);
        if (!m) return std::unexpected(m.error());
        manifest_ = std::move(*m);
        return {};
    }

    /// A chunked entry's chunk table, read and checked against its digest.
    Result<std::vector<Chunk>> chunk_table(const Entry& e) const {
        if (auto r = check_run(e); !r) return std::unexpected(r.error());
        auto table = source_->read(e.offset, e.stored_size);
        if (!table) return std::unexpected(table.error());
        const std::uint64_t n = e.stored_size / chunk_record;
        std::vector<Chunk> chunks;
        std::vector<Digest> leaves;
        for (std::uint64_t i = 0; i < n; ++i) {
            const std::byte* p = table->data() + i * chunk_record;
            Chunk c;
            c.offset = u64(p);
            c.stored_size = u32(p + 8);
            c.size = u32(p + 12);
            const std::uint16_t codec = u16(p + 16);
            c.dictionary = u32(p + 20);
            std::copy(p + 32, p + 64, c.sha256.begin());
            c.reserved_clear = all_zero(std::span(p + 18, 2)) && all_zero(std::span(p + 24, 8));
            const std::uint64_t want = i + 1 < n ? chunk_size : e.size - (n - 1) * chunk_size;
            if (c.size != want) return fail(Code::malformed, std::format("chunk {} holds {} bytes, not {}", i, c.size, want), e.path);
            if (codec > 2) return fail(Code::malformed, std::format("chunk {} has codec {}", i, codec), e.path);
            c.codec = static_cast<Codec>(codec);
            if ((c.codec == Codec::zstd_dict) != (c.dictionary != 0)) return fail(Code::malformed, std::format("chunk {}'s dictionary ID does not fit its codec", i), e.path);
            if (c.codec == Codec::stored && c.stored_size != c.size) return fail(Code::malformed, std::format("stored chunk {} is not its size", i), e.path);
            if (!inside(lazy_->data_regions, c.offset, c.stored_size, e.commit)) {
                return fail(Code::malformed, std::format("chunk {} lies outside the data regions", i), e.path);
            }
            leaves.push_back(c.sha256);
            chunks.push_back(c);
        }
        const Digest whole = e.size <= chunk_size ? leaves.front() : tree_hash_from_leaves(leaves, e.size);
        if (whole != e.digest) return fail(Code::integrity, "the chunk table does not match the entry's digest", e.path);
        return chunks;
    }

    /// One run, decoded: stored, zstd, or zstd with its dictionary.
    Result<std::vector<std::byte>> decode_run(std::uint64_t offset, std::uint64_t stored, std::uint64_t size, Codec codec,
                                              std::uint32_t dictionary, const std::string& what) const {
        auto run = source_->read(offset, stored);
        if (!run) return std::unexpected(Error{run.error().code, run.error().message, what});
        if (codec == Codec::stored) return run;
        const std::uint32_t max_window = options_.limits.max_window_log;
        if (auto info = check_frame(*run, size, dictionary, max_window, what); !info) return std::unexpected(info.error());
        std::shared_ptr<const std::vector<std::byte>> dict;
        if (codec == Codec::zstd_dict) {
            auto d = load_dictionary(dictionary);
            if (!d) return std::unexpected(Error{d.error().code, d.error().message, what});
            dict = *d;
        }
        return zstd_decode(*run, size, dict ? std::span<const std::byte>(*dict) : std::span<const std::byte>(), max_window, what);
    }

    /// A dictionary by ID: SQ-INF/dictionaries/<id>.zdict, flagged as one,
    /// its own header naming the same ID (§4.3). Kept while open.
    Result<std::shared_ptr<const std::vector<std::byte>>> load_dictionary(std::uint32_t id) const {
        {
            std::lock_guard lock(lazy_->mutex);
            if (auto it = lazy_->dictionaries.find(id); it != lazy_->dictionaries.end()) return it->second;
        }
        const std::string path = std::format("{}{}.zdict", dictionary_dir, id);
        const auto e = find(path);
        if (!e) {
            if (e.error().code == Code::not_found) return fail(Code::malformed, std::format("no dictionary {} ({})", id, path));
            return std::unexpected(e.error());
        }
        if (!(e->flags & flag::dictionary)) return fail(Code::malformed, path + " is not flagged as a dictionary");
        if (e->codec == Codec::zstd_dict || e->codec == Codec::chunked) return fail(Code::malformed, path + " is not stored or zstd");
        auto bytes = read(*e);
        if (!bytes) return std::unexpected(bytes.error());
        if (dictionary_id_of(*bytes) != id) return fail(Code::malformed, path + "'s own header names another dictionary ID");
        auto shared = std::make_shared<const std::vector<std::byte>>(std::move(*bytes));
        std::lock_guard lock(lazy_->mutex);
        lazy_->dictionaries.emplace(id, shared);
        return shared;
    }

    void verify(SignatureCheck& s) const {
        const Signature& sig = s.signature;
        if (sig.algorithm != 1) {
            s.state = SignatureState::unknown_algorithm;
            s.why = std::format("algorithm {} is not one this reader knows", sig.algorithm);
            return;
        }
        if (sig.public_key.size() != 32 || sig.signature.size() != 64) {
            s.why = "an Ed25519 key or signature of the wrong length";
            return;
        }
        const Digest key_hash = sha256(sig.public_key);
        if (!std::equal(sig.key_id.begin(), sig.key_id.end(), key_hash.begin())) {
            s.why = "its key identifier is not its key's";
            return;
        }
        const auto message = signed_message(1, manifest_.id, manifest_.version, sig.signer, commit().content_digest);
        if (!ed25519_verify(std::span<const std::byte, 64>(sig.signature.data(), 64),
                            std::span<const std::byte, 32>(sig.public_key.data(), 32), message)) {
            s.why = "it does not verify (or its encoding is not canonical)";
            return;
        }
        s.state = SignatureState::valid;
    }

    friend CheckReport check_archive(const Archive&, CheckReport);

    /// What is worked out once, on first need, by whichever thread needs it.
    struct Lazy {
        std::once_flag live_once;
        Result<std::vector<Entry>> live = std::vector<Entry>{};
        std::once_flag regions_once;
        std::vector<CommitRegion> data_regions, chunk_regions;
        std::optional<Error> regions_error;
        std::mutex mutex;
        std::map<std::uint32_t, std::shared_ptr<const std::vector<std::byte>>> dictionaries;
    };

    std::unique_ptr<Source> source_;
    ReadOptions options_;
    std::vector<std::byte> header_;
    std::array<Slot, 2> slots_;
    std::vector<Loaded> chain_;
    Manifest manifest_;
    std::unique_ptr<Lazy> lazy_ = std::make_unique<Lazy>();
};

}  // namespace sq::cart

namespace sq::cart {

[[nodiscard]] bool deceptive_text(std::string_view s) {
    std::u32string cps;
    if (!decode_utf8(s, &cps)) return true;
    return std::ranges::any_of(cps, [](char32_t c) {
        return c < 0x20 || c == 0x7f || (c >= 0x80 && c <= 0x9f) || (c >= 0x202a && c <= 0x202e) ||
               (c >= 0x2066 && c <= 0x2069);
    });
}

CheckReport check_archive(const Archive& a, CheckReport r) {
    const auto add_finding = [&](Finding::Severity s, Code code, std::string message, std::optional<std::string> entry = {}) {
        r.findings.push_back({s, code, std::move(message), std::move(entry)});
    };
    const auto error = [&](Code c, std::string m, std::optional<std::string> e = {}) { add_finding(Finding::Severity::error, c, std::move(m), std::move(e)); };
    const auto warning = [&](std::string m, std::optional<std::string> e = {}) {
        add_finding(Finding::Severity::warning, Code::malformed, std::move(m), std::move(e));
    };
    r.commit = a.commit();

    // What a reader ignores (§9.4, §12): trailing bytes, the slot not in
    // charge, non-zero reserved bytes.
    if (a.trailing_bytes() != 0) {
        warning(std::format("{} bytes after the current commit: an unfinished commit, or something appended (covered by "
                            "no digest or signature; compact before distributing)", a.trailing_bytes()));
    }
    for (std::size_t i = 0; i < 2; ++i) {
        const Slot& s = a.slots_[i];
        if (!s.valid && s.why != "it is empty") warning(std::format("superblock slot {}: {}", i == 0 ? 'A' : 'B', s.why));
    }
    const std::span<const std::byte> h(a.header_);
    if (!all_zero(h.subspan(16, 48)) || !all_zero(h.subspan(128, 3968)) || !all_zero(h.subspan(4160, 4032)) ||
        !all_zero(h.subspan(slot_a + 32, 24)) || !all_zero(h.subspan(slot_b + 32, 24))) {
        warning("non-zero reserved bytes in the header");
    }
    for (const Loaded& l : a.chain_) {
        if (auto rec = a.source_->read(l.commit.offset, commit_size); rec && !all_zero(std::span(*rec).subspan(168, 24))) {
            warning(std::format("non-zero reserved bytes in commit {}'s record", l.commit.sequence));
        }
        for (std::uint64_t i = 0; i < l.commit.index_count; ++i) {
            const std::byte* p = l.index.data() + i * record_size;
            if (!all_zero(std::span(p + 28, 4)) || !all_zero(std::span(p + 104, 24))) {
                warning("non-zero reserved bytes in its entry record", std::string(path_view(l, i)));
            }
        }
    }

    // The whole state (§3.5, §4.1); every live entry, read and verified;
    // its metadata looked at.
    auto state = a.entries();
    if (!state) {
        error(state.error().code, state.error().message, state.error().entry);
        return r;
    }
    struct Run {
        std::uint64_t offset, length;
        std::string owner;
    };
    std::vector<Run> runs;
    std::set<std::uint32_t> dictionaries_used;
    for (const Entry& e : *state) {
        ++r.entries_checked;
        if (e.codec == Codec::zstd_dict) dictionaries_used.insert(e.dictionary);
        if (e.codec == Codec::chunked) {
            auto chunks = a.chunk_table(e);
            if (!chunks) {
                error(chunks.error().code, chunks.error().message, e.path);
                continue;
            }
            runs.push_back({e.offset, e.stored_size, e.path + " (chunk table)"});
            for (std::size_t i = 0; i < chunks->size(); ++i) {
                const Chunk& c = (*chunks)[i];
                if (c.codec == Codec::zstd_dict) dictionaries_used.insert(c.dictionary);
                if (!c.reserved_clear) warning(std::format("non-zero reserved bytes in chunk {}'s record", i), e.path);
                runs.push_back({c.offset, c.stored_size, std::format("{} (chunk {})", e.path, i)});
                auto bytes = a.read_range(e, i * chunk_size, c.size);
                if (!bytes) error(bytes.error().code, bytes.error().message, e.path);
            }
        } else {
            runs.push_back({e.offset, e.stored_size, e.path});
            if (auto bytes = a.read(e); !bytes) error(bytes.error().code, bytes.error().message, e.path);
        }
        if (e.attributes) {
            auto canonical = canonical_json(*e.attributes, a.options_.limits.max_json_depth);
            if (!canonical) {
                error(Code::json, "its attributes: " + canonical.error().message, e.path);
            } else if (*canonical != *e.attributes) {
                warning("its attributes are not in canonical (RFC 8785) form", e.path);
            }
        }
        for (const auto* text : {&e.title, &e.author}) {
            if (*text && deceptive_text(**text)) warning("a title or author with control or bidirectional characters", e.path);
        }
        constexpr std::int64_t exact = (std::int64_t{1} << 53) - 1;
        if (e.created < -exact || e.created > exact || e.updated < -exact || e.updated > exact) {
            error(Code::json, "a created or updated time beyond ±(2^53 − 1) ms, which the content digest cannot hold", e.path);
        }
    }
    for (const Entry& e : *state) {
        if ((e.flags & flag::dictionary) && !e.path.ends_with(".zdict")) continue;
        if (e.flags & flag::dictionary) {
            const std::string_view id = std::string_view(e.path).substr(dictionary_dir.size(), e.path.size() - dictionary_dir.size() - 6);
            std::uint32_t n = 0;
            const auto [ptr, ec] = std::from_chars(id.data(), id.data() + id.size(), n);
            if (ec != std::errc{} || ptr != id.data() + id.size() || !dictionaries_used.contains(n)) {
                warning("a dictionary no live entry uses (compaction drops it)", e.path);
            }
        }
    }

    // Runs: identical or apart, never partly over each other (§3.6, R10).
    std::ranges::sort(runs, {}, [](const Run& x) { return std::pair(x.offset, x.length); });
    for (std::size_t i = 1; i < runs.size(); ++i) {
        const Run& p = runs[i - 1];
        const Run& q = runs[i];
        if (p.length == 0 || q.length == 0) continue;
        if (p.offset == q.offset && p.length == q.length) continue;
        if (q.offset < p.offset + p.length) {
            error(Code::malformed, std::format("its bytes partly overlap {}'s", p.owner), q.owner);
        }
    }

    // The content digest, and the signatures over it.
    if (auto digest = a.compute_content_digest(); !digest) {
        error(digest.error().code, digest.error().message, digest.error().entry);
    } else if (*digest != a.commit().content_digest) {
        error(Code::integrity, std::format("the content digest is {}, the commit says {}", to_hex(*digest), to_hex(a.commit().content_digest)));
    }
    if (auto sigs = a.signatures(); !sigs) {
        error(sigs.error().code, sigs.error().message);
    } else {
        for (const SignatureCheck& s : *sigs) {
            if (s.state == SignatureState::valid) {
                ++r.signatures_valid;
            } else if (s.state == SignatureState::unknown_algorithm) {
                warning("a signature this reader cannot verify: " + s.why);
            } else {
                error(Code::integrity, std::format("a signature by key {} that is invalid: {}", to_hex(s.signature.key_id), s.why));
            }
        }
    }
    return r;
}

}  // namespace sq::cart

export namespace sq::cart {

/// Validate a 3.0 cartridge in full (§12): open it, read and verify every
/// live entry, recompute the content digest, verify the signatures, and
/// report what a reader may ignore.
[[nodiscard]] CheckReport check(const std::filesystem::path& path, const ReadOptions& options = {}) {
    auto a = Archive::open(path, options);
    if (!a) return CheckReport{{{Finding::Severity::error, a.error().code, a.error().message, a.error().entry}}, {}, 0, 0};
    return check_archive(*a, {});
}

[[nodiscard]] CheckReport check_memory(std::span<const std::byte> bytes, const ReadOptions& options = {}) {
    auto a = Archive::open_memory(bytes, options);
    if (!a) return CheckReport{{{Finding::Severity::error, a.error().code, a.error().message, a.error().entry}}, {}, 0, 0};
    return check_archive(*a, {});
}

}  // namespace sq::cart
