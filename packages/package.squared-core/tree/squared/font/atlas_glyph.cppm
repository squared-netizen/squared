export module sq.font:atlas_glyph;

import std;

export namespace sq::font
{

/**
 * @brief Where a glyph is in a `GlyphAtlas`, and how it is placed: the
 * rectangle of its coverage (empty for a space), and the bitmap's offset
 * from the pen on the baseline, as `GlyphBitmap` gives it.
 */
struct AtlasGlyph
{
        std::uint16_t x = 0;
        std::uint16_t y = 0;
        std::uint16_t width = 0;
        std::uint16_t height = 0;
        std::int16_t left = 0;
        std::int16_t top = 0;
        std::int16_t advance = 0;
        /** The `FontSet` face that drew it (0 the primary). */
        std::uint8_t face = 0;

        friend constexpr bool operator==( const AtlasGlyph &,
                                          const AtlasGlyph & ) noexcept = default;
};

} // namespace sq::font
