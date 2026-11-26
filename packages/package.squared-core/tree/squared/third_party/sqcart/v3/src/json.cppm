// SPDX-License-Identifier: MIT
//
// sq.cart:json — JSON as format 3.0 wants it, through squared's JSON facade
// (sq.data, third_party/squared-data): I-JSON (RFC 7493, §5), refused rather
// than guessed at, and the canonical form of RFC 8785 (JCS) that the content
// digest hashes metadata in (§9.2). Also the UTF-8 checks paths and strings
// need, which are not JSON's.

export module sq.cart:json;

import std;
import sq.data;
import :error;

namespace sq::cart {

/// Decode UTF-8, refusing what is not Unicode: overlong forms, surrogates,
/// anything past U+10FFFF.
[[nodiscard]] bool decode_utf8(std::string_view s, std::u32string* out) {
    for (std::size_t i = 0; i < s.size();) {
        const auto c = static_cast<unsigned char>(s[i]);
        std::size_t n = 1;
        char32_t cp = 0, min = 0;
        if (c < 0x80) {
            cp = c;
        } else if ((c >> 5) == 6) {
            n = 2, cp = c & 0x1f, min = 0x80;
        } else if ((c >> 4) == 14) {
            n = 3, cp = c & 0x0f, min = 0x800;
        } else if ((c >> 3) == 30) {
            n = 4, cp = c & 0x07, min = 0x10000;
        } else {
            return false;
        }
        if (i + n > s.size()) return false;
        for (std::size_t k = 1; k < n; ++k) {
            const auto d = static_cast<unsigned char>(s[i + k]);
            if ((d >> 6) != 2) return false;
            cp = cp << 6 | (d & 0x3f);
        }
        if (n > 1 && (cp < min || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff))) return false;
        if (out != nullptr) out->push_back(cp);
        i += n;
    }
    return true;
}

/// Parse `json` as I-JSON, within `max_depth`.
[[nodiscard]] Result<data::JsonValue> parse_ijson(std::string_view json, std::uint32_t max_depth) {
    data::JsonParseOptions options;
    options.maximum_depth = max_depth;
    options.ijson = true;
    auto parsed = data::parse_json(json, options);
    if (!parsed) return fail(Code::json, "not I-JSON: " + parsed.error.message);
    return std::move(parsed.value);
}

[[nodiscard]] Result<std::string> write_canonical(const data::JsonValue& value) {
    data::JsonWriteOptions options;
    options.canonical = true;
    auto written = data::write_json(value, options);
    if (!written) return fail(Code::json, written.error.message);
    return std::move(written.text);
}

/// A top-level string member of a JSON object, or nothing.
[[nodiscard]] std::optional<std::string> json_string_member(const data::JsonValue& object, std::string_view name) {
    const data::JsonValue* v = object.find(name);
    if (v == nullptr || v->string_if() == nullptr) return std::nullopt;
    return *v->string_if();
}

}  // namespace sq::cart

export namespace sq::cart {

/// `json` in the canonical form of RFC 8785, once it has been checked to be
/// I-JSON (§5): UTF-8 with no byte order mark, no member twice in one
/// object, no integer beyond ±(2^53 − 1), nesting within `max_depth`.
[[nodiscard]] Result<std::string> canonical_json(std::string_view json, std::uint32_t max_depth = 64) {
    auto value = parse_ijson(json, max_depth);
    if (!value) return std::unexpected(value.error());
    return write_canonical(*value);
}

/// Whether `json` is I-JSON (§5).
[[nodiscard]] Result<void> check_ijson(std::string_view json, std::uint32_t max_depth = 64) {
    if (auto r = parse_ijson(json, max_depth); !r) return std::unexpected(r.error());
    return {};
}

/// Whether `s` is valid UTF-8 (no surrogates, nothing overlong).
[[nodiscard]] bool valid_utf8(std::string_view s) { return decode_utf8(s, nullptr); }

}  // namespace sq::cart
