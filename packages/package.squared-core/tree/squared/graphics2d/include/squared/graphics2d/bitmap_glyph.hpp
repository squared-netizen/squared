#pragma once

#include <cstddef>

namespace sq::graphics2d {

/** @brief One Unicode scalar's source rectangle and layout metrics. */
struct BitmapGlyph final {
    char32_t codepoint{0};
    std::size_t page{0};
    int x{0};
    int y{0};
    int width{0};
    int height{0};
    int x_offset{0};
    int y_offset{0};
    int x_advance{0};
};

} // namespace sq::graphics2d
