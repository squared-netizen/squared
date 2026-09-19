#pragma once

#include <cstddef>

namespace sq::graphics2d {

/** @brief One explicit line in a GlyphLayout. */
struct GlyphLine final {
    std::size_t first_glyph{0};
    std::size_t glyph_count{0};
    float width{0.0F};
    float x_offset{0.0F};
    float y{0.0F};
};

} // namespace sq::graphics2d
