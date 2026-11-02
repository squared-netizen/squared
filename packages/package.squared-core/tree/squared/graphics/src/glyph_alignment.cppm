export module sq.graphics:src.glyph_alignment;

import std;

export namespace sq::graphics
{

/** @brief Horizontal placement within an optional target width. */
enum class GlyphAlignment : std::uint8_t { start, center, end };

} // namespace sq::graphics
