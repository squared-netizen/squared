export module sq.font:glyph_bitmap;

import std;

export namespace sq::font
{

/**
 * @brief One rendered glyph: 8-bit coverage, 0 (none) to 255 (full), row by
 * row from the top, and where it goes from the pen.
 *
 * It points into the face's own buffer: valid until the face renders
 * another glyph. A space has no pixels and only an advance.
 */
struct GlyphBitmap
{
        std::span<const std::uint8_t> coverage;
        std::uint16_t width = 0;
        std::uint16_t height = 0;
        /** Bytes from one row to the next (at least `width`). */
        std::uint16_t pitch = 0;
        /** The bitmap's left edge, right of the pen. */
        std::int16_t left = 0;
        /** Its top edge, above the baseline. */
        std::int16_t top = 0;
        /** How far the pen moves on. */
        std::int16_t advance = 0;

        /** Row `y`, from the top; `y` is below `height`. */
        [[nodiscard]] std::span<const std::uint8_t> row( std::uint16_t y ) const noexcept
        {
            return coverage.subspan( std::size_t{ y } * pitch, width );
        }
};

} // namespace sq::font
