#pragma once

#include <squared/graphics2d/glyph_alignment.hpp>

#include <cstddef>

namespace sq::graphics2d {

/** @brief Controls for one UTF-8 glyph layout operation. */
struct GlyphLayoutOptions final {
    /** @brief Uniform metric scale; finite and greater than zero. */
    float scale{1.0F};
    /** @brief Optional line box width; zero uses the widest content line. */
    float target_width{0.0F};
    GlyphAlignment alignment{GlyphAlignment::start};
    /** @brief Fallback scalar used for missing glyphs; zero skips them. */
    char32_t replacement{U'?'};
    /** @brief Reject malformed UTF-8 rather than substituting replacement. */
    bool reject_invalid_utf8{true};
    /** @brief Number of space advances emitted for one tab; must be positive. */
    std::size_t tab_spaces{4};
};

} // namespace sq::graphics2d
