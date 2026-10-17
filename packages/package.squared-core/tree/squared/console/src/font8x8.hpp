#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace sq::console
{

/**
 * @brief The console's glyphs: 128 characters (ASCII), 8 by 8 pixels, 1 KB.
 *
 * Row by row, top first; in each row the low bit is the leftmost pixel. Public
 * domain (font8x8 by Daniel Hepper). Characters 0 to 31 and 127 are blank.
 */
extern const std::array<std::array<std::uint8_t, 8>, 128> font8x8;

/** @brief The glyph drawn as a filled square: backgrounds and the cursor. Blank in the font. */
inline constexpr std::uint8_t solid_glyph = 127;

/** @brief The atlas the renderer uploads: 16 by 8 glyphs, one byte a texel. */
inline constexpr std::size_t atlas_width = 128;
inline constexpr std::size_t atlas_height = 64;
inline constexpr std::size_t atlas_bytes = atlas_width * atlas_height;

/**
 * @brief Expand the font into `out`, 255 where a glyph has a pixel and 0
 * elsewhere; glyph `g` at column `g % 16`, row `g / 16`; solid_glyph filled.
 */
void build_atlas( std::span<std::uint8_t, atlas_bytes> out ) noexcept;

} // namespace sq::console
