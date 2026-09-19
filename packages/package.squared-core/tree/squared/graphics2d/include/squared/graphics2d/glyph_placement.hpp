#pragma once

#include <cstddef>

namespace sq::graphics2d {

/**
 * @brief One SpriteBatch-ready glyph quad in top-left logical coordinates.
 *
 * `page`, source coordinates, and destination geometry are values. A renderer
 * resolves the page, creates a TextureRegion for the source rectangle, and
 * submits it to SpriteBatch at `(x, y, width, height)`.
 */
struct GlyphPlacement final {
    char32_t codepoint{0};
    std::size_t page{0};
    int source_x{0};
    int source_y{0};
    int source_width{0};
    int source_height{0};
    float x{0.0F};
    float y{0.0F};
    float width{0.0F};
    float height{0.0F};
    float advance{0.0F};
};

} // namespace sq::graphics2d
