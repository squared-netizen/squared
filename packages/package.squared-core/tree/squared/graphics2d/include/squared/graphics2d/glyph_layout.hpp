#pragma once

#include <squared/graphics2d/glyph_layout_options.hpp>
#include <squared/graphics2d/glyph_line.hpp>
#include <squared/graphics2d/glyph_placement.hpp>

#include <span>
#include <string_view>
#include <vector>

namespace sq::graphics2d {

class BitmapFont;
struct BitmapFontError;

/** @brief Reusable value-semantic layout of UTF-8 text into glyph quads. */
class GlyphLayout final {
public:
    /**
     * @brief Decode and lay out explicit newline-delimited UTF-8 text.
     * @param font Valid parsed font; borrowed for this call only.
     * @param text UTF-8 bytes; not retained.
     * @param options Scale, line-box alignment, replacement, and tab policy.
     * @param error Cleared on success or filled on failure.
     * @return true after transactional replacement; false leaves the prior
     * layout unchanged.
     */
    [[nodiscard]] bool set_text(
        const BitmapFont& font,
        std::string_view text,
        const GlyphLayoutOptions& options,
        BitmapFontError& error
    ) noexcept;

    /** @brief Convenience overload using default options. */
    [[nodiscard]] bool set_text(
        const BitmapFont& font,
        std::string_view text,
        BitmapFontError& error
    ) noexcept
    {
        return set_text(font, text, {}, error);
    }

    void clear() noexcept;
    [[nodiscard]] float width() const noexcept { return width_; }
    [[nodiscard]] float height() const noexcept { return height_; }
    [[nodiscard]] float line_height() const noexcept { return line_height_; }
    [[nodiscard]] std::span<const GlyphPlacement> glyphs() const noexcept
    {
        return glyphs_;
    }
    [[nodiscard]] std::span<const GlyphLine> lines() const noexcept
    {
        return lines_;
    }

private:
    float width_{0.0F};
    float height_{0.0F};
    float line_height_{0.0F};
    std::vector<GlyphPlacement> glyphs_;
    std::vector<GlyphLine> lines_;
};

} // namespace sq::graphics2d
