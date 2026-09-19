#pragma once

// Internal to the squared build: shared helpers lifted out
// of a per-translation-unit anonymous namespace so each
// type can own its own translation unit. Not installed and
// not part of the public API.

#include <squared/graphics/color.hpp>
#include <squared/gui/rectangle.hpp>
#include <squared/gui/widget.hpp>

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>

namespace sq::gui::detail {

inline constexpr float default_padding = 8.0F;
inline constexpr float default_spacing = 6.0F;
inline constexpr float default_touch_size = 44.0F;

inline graphics::Color multiply(
    graphics::Color left,
    graphics::Color right
) noexcept
{
    return {
        left.red * right.red,
        left.green * right.green,
        left.blue * right.blue,
        left.alpha * right.alpha
    };
}

inline Rectangle bounds_of(const Widget& widget, float x, float y) noexcept
{
    return {x, y, widget.width(), widget.height()};
}

inline float clamp_dimension(float value, float minimum, float maximum) noexcept
{
    return std::min(std::max(value, minimum), maximum);
}

inline bool valid_font_path(std::string_view path) noexcept
{
    if (path.empty() || path.front() == '/' ||
        path.find('\\') != std::string_view::npos) {
        return false;
    }
    std::size_t start = 0;
    while (start <= path.size()) {
        const auto slash = path.find('/', start);
        const auto part = path.substr(
            start, slash == path.npos ? path.size() - start : slash - start
        );
        if (part.empty() || part == "." || part == "..") return false;
        if (slash == path.npos) break;
        start = slash + 1;
    }
    return true;
}

inline std::size_t previous_codepoint(const std::string& text, std::size_t cursor)
{
    if (cursor == 0) return 0;
    --cursor;
    while (cursor > 0 &&
           (static_cast<unsigned char>(text[cursor]) & 0xC0U) == 0x80U) {
        --cursor;
    }
    return cursor;
}

inline std::size_t next_codepoint(const std::string& text, std::size_t cursor)
{
    if (cursor >= text.size()) return text.size();
    ++cursor;
    while (cursor < text.size() &&
           (static_cast<unsigned char>(text[cursor]) & 0xC0U) == 0x80U) {
        ++cursor;
    }
    return cursor;
}

template <typename Map>
inline const typename Map::mapped_type& style_or_default(
    const Map& styles,
    std::string_view name
)
{
    const auto found = styles.find(std::string(name));
    if (found != styles.end()) return found->second;
    return styles.at("default");
}

} // namespace sq::gui::detail
