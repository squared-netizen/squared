#pragma once

// Internal to the squared build: shared helpers lifted out
// of a per-translation-unit anonymous namespace so each
// type can own its own translation unit. Not installed and
// not part of the public API.

#include <squared/graphics2d/bitmap_font_error.hpp>
#include <squared/graphics2d/bitmap_font_error_code.hpp>

#include <charconv>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace sq::graphics2d::detail {

using Fields = std::unordered_map<std::string_view, std::string_view>;

[[nodiscard]] inline bool is_space(char value) noexcept
{
    return value == ' ' || value == '\t';
}

[[nodiscard]] inline bool valid_scalar(char32_t value) noexcept
{
    return value <= 0x10FFFF && !(value >= 0xD800 && value <= 0xDFFF);
}

inline void fail(
    BitmapFontError& error,
    BitmapFontErrorCode code,
    std::size_t line,
    std::string message
)
{
    error = BitmapFontError{code, line, std::move(message)};
}

[[nodiscard]] inline bool parse_record(
    std::string_view line,
    std::string_view& tag,
    Fields& fields,
    BitmapFontError& error,
    std::size_t line_number
)
{
    std::size_t cursor = 0;
    while (cursor < line.size() && is_space(line[cursor])) {
        ++cursor;
    }
    const auto tag_start = cursor;
    while (cursor < line.size() && !is_space(line[cursor])) {
        ++cursor;
    }
    tag = line.substr(tag_start, cursor - tag_start);
    if (tag.empty()) {
        return true;
    }

    while (cursor < line.size()) {
        while (cursor < line.size() && is_space(line[cursor])) {
            ++cursor;
        }
        if (cursor == line.size()) {
            break;
        }

        const auto key_start = cursor;
        while (cursor < line.size() && line[cursor] != '=' &&
               !is_space(line[cursor])) {
            ++cursor;
        }
        if (cursor == line.size() || line[cursor] != '=') {
            fail(error, BitmapFontErrorCode::malformed, line_number,
                 "expected key=value field");
            return false;
        }
        const auto key = line.substr(key_start, cursor - key_start);
        ++cursor;

        std::string_view value;
        if (cursor < line.size() && line[cursor] == '"') {
            ++cursor;
            const auto value_start = cursor;
            while (cursor < line.size() && line[cursor] != '"') {
                ++cursor;
            }
            if (cursor == line.size()) {
                fail(error, BitmapFontErrorCode::malformed, line_number,
                     "unterminated quoted value");
                return false;
            }
            value = line.substr(value_start, cursor - value_start);
            ++cursor;
            if (cursor < line.size() && !is_space(line[cursor])) {
                fail(error, BitmapFontErrorCode::malformed, line_number,
                     "unexpected data after quoted value");
                return false;
            }
        } else {
            const auto value_start = cursor;
            while (cursor < line.size() && !is_space(line[cursor])) {
                ++cursor;
            }
            value = line.substr(value_start, cursor - value_start);
        }

        if (key.empty()) {
            fail(error, BitmapFontErrorCode::malformed, line_number,
                 "empty field name");
            return false;
        }
        if (!fields.emplace(key, value).second) {
            fail(error, BitmapFontErrorCode::duplicate, line_number,
                 "duplicate field in record");
            return false;
        }
    }
    return true;
}

template <typename Integer>
[[nodiscard]] inline bool integer_field(
    const Fields& fields,
    std::string_view name,
    Integer& result,
    BitmapFontError& error,
    std::size_t line
)
{
    const auto found = fields.find(name);
    if (found == fields.end()) {
        fail(error, BitmapFontErrorCode::missing_required, line,
             "missing required field: " + std::string{name});
        return false;
    }
    const auto value = found->second;
    Integer parsed{};
    const auto conversion = std::from_chars(
        value.data(), value.data() + value.size(), parsed
    );
    if (conversion.ec != std::errc{} ||
        conversion.ptr != value.data() + value.size()) {
        fail(error, BitmapFontErrorCode::malformed, line,
             "invalid integer field: " + std::string{name});
        return false;
    }
    result = parsed;
    return true;
}

[[nodiscard]] inline bool safe_page_path(std::string_view path) noexcept
{
    if (path.empty() || path.front() == '/' || path.find('\\') != path.npos ||
        path.find(':') != path.npos) {
        return false;
    }
    std::size_t start = 0;
    while (start <= path.size()) {
        const auto slash = path.find('/', start);
        const auto component = path.substr(
            start, slash == path.npos ? path.size() - start : slash - start
        );
        if (component.empty() || component == "." || component == "..") {
            return false;
        }
        if (slash == path.npos) {
            break;
        }
        start = slash + 1;
    }
    return true;
}

[[nodiscard]] inline std::optional<char32_t> decode_utf8(
    std::string_view text,
    std::size_t& cursor
) noexcept
{
    const auto first = static_cast<unsigned char>(text[cursor]);
    if (first < 0x80) {
        ++cursor;
        return static_cast<char32_t>(first);
    }

    std::size_t count = 0;
    char32_t value = 0;
    char32_t minimum = 0;
    if ((first & 0xE0U) == 0xC0U) {
        count = 2;
        value = first & 0x1FU;
        minimum = 0x80;
    } else if ((first & 0xF0U) == 0xE0U) {
        count = 3;
        value = first & 0x0FU;
        minimum = 0x800;
    } else if ((first & 0xF8U) == 0xF0U) {
        count = 4;
        value = first & 0x07U;
        minimum = 0x10000;
    } else {
        ++cursor;
        return std::nullopt;
    }

    if (count > text.size() - cursor) {
        ++cursor;
        return std::nullopt;
    }
    for (std::size_t index = 1; index < count; ++index) {
        const auto continuation = static_cast<unsigned char>(text[cursor + index]);
        if ((continuation & 0xC0U) != 0x80U) {
            ++cursor;
            return std::nullopt;
        }
        value = (value << 6U) | (continuation & 0x3FU);
    }
    cursor += count;
    if (value < minimum || !valid_scalar(value)) {
        return std::nullopt;
    }
    return value;
}

} // namespace sq::graphics2d::detail
