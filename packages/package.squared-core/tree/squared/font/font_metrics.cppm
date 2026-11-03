export module sq.font:font_metrics;

import std;

export namespace sq::font
{

/**
 * @brief A face's sizes at its pixel size, in whole pixels: what a grid of
 * character cells is laid out by.
 */
struct FontMetrics
{
        /** A cell's width: the face's advance (a monospace face has one). */
        std::uint16_t cell_width = 0;
        /** A line's height: ascender to descender, and the line gap. */
        std::uint16_t cell_height = 0;
        /** The baseline, down from a cell's top. */
        std::int16_t ascender = 0;
        /** How far below the baseline a glyph may reach (negative or 0). */
        std::int16_t descender = 0;
        /** An underline's top, below the baseline (positive is down). */
        std::int16_t underline_position = 0;
        std::uint16_t underline_thickness = 1;

        friend constexpr bool operator==( const FontMetrics &,
                                          const FontMetrics & ) noexcept = default;
};

} // namespace sq::font
