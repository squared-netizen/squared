#pragma once

#include <squared/graphics/src/bitmap_font_error.hpp>
#include <squared/graphics/src/glyph_layout_options.hpp>
#include <squared/graphics/src/glyph_line.hpp>
#include <squared/graphics/src/glyph_placement.hpp>

#include <span>
#include <string_view>
#include <vector>

namespace sq::graphics
{

class BitmapFont;

/** @brief Reusable value-semantic layout of UTF-8 text into glyph quads. */
class GlyphLayout final
{
    public:
        /**
     * @brief Decode and lay out explicit newline-delimited UTF-8 text.
     * @param font Valid parsed font; borrowed for this call only.
     * @param text UTF-8 bytes; not retained.
     * @param options Scale, line-box alignment, replacement, and tab policy.
     * @return The failure, true when something failed; an empty error after
     * replacing the layout. A failure leaves the prior layout unchanged.
     * Building and returning the error never allocates.
     *
     * The layout's storage is reused: a call allocates only when this text
     * needs more glyphs or lines than any earlier one did. Keep one layout and
     * call this again rather than making a new one per call.
     */
        [[nodiscard]] BitmapFontError set_text( const BitmapFont &font, std::string_view text,
                                                const GlyphLayoutOptions &options = {} ) noexcept;

        /** @brief Empty the layout, keeping its storage; assign GlyphLayout{} to release it. */
        void clear() noexcept;
        [[nodiscard]] float width() const noexcept { return width_; }
        [[nodiscard]] float height() const noexcept { return height_; }
        [[nodiscard]] float line_height() const noexcept { return line_height_; }
        [[nodiscard]] std::span<const GlyphPlacement> glyphs() const noexcept { return glyphs_; }
        [[nodiscard]] std::span<const GlyphLine> lines() const noexcept { return lines_; }

    private:
        float width_{ 0.0F };
        float height_{ 0.0F };
        float line_height_{ 0.0F };
        std::vector<GlyphPlacement> glyphs_;
        std::vector<GlyphLine> lines_;
};

} // namespace sq::graphics
