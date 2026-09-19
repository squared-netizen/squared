#pragma once

#include <squared/graphics2d/bitmap_font_info.hpp>
#include <squared/graphics2d/bitmap_font_page.hpp>
#include <squared/graphics2d/bitmap_font_parse_limits.hpp>
#include <squared/graphics2d/bitmap_glyph.hpp>
#include <squared/graphics2d/bitmap_kerning.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace sq::graphics2d {

struct BitmapFontError;

/**
 * @brief Transactionally parsed text BMFont resource.
 *
 * The resource owns only portable metadata and relative texture-page paths.
 * Applications resolve those pages through their selected asset and graphics
 * systems. The class is value-semantic and not thread-safe during mutation.
 */
class BitmapFont final {
public:
    /**
     * @brief Parse a text BMFont descriptor transactionally.
     * @param descriptor Complete UTF-8/ASCII BMFont text; not retained.
     * @param error Replaced with the first failure, or cleared on success.
     * @param limits Positive resource bounds applied before allocation.
     * @return true after committing a complete font; false leaves this font
     * unchanged.
     */
    [[nodiscard]] bool load(
        std::string_view descriptor,
        BitmapFontError& error,
        const BitmapFontParseLimits& limits = {}
    ) noexcept;

    /** @brief Remove all parsed metadata and return to the invalid state. */
    void clear() noexcept;

    /** @brief Return true when required metadata, pages, and glyphs exist. */
    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] const BitmapFontInfo& info() const noexcept { return info_; }
    [[nodiscard]] int line_height() const noexcept { return line_height_; }
    [[nodiscard]] int baseline() const noexcept { return baseline_; }
    [[nodiscard]] int page_width() const noexcept { return page_width_; }
    [[nodiscard]] int page_height() const noexcept { return page_height_; }
    [[nodiscard]] std::span<const BitmapFontPage> pages() const noexcept
    {
        return pages_;
    }
    [[nodiscard]] std::span<const BitmapGlyph> glyphs() const noexcept
    {
        return glyphs_;
    }
    [[nodiscard]] std::span<const BitmapKerning> kernings() const noexcept
    {
        return kernings_;
    }

    /** @brief Find a glyph by Unicode scalar; returns null on a miss. */
    [[nodiscard]] const BitmapGlyph* glyph(char32_t codepoint) const noexcept;

    /** @brief Return the pair adjustment, or zero when no pair is declared. */
    [[nodiscard]] int kerning(char32_t first, char32_t second) const noexcept;

private:
    friend class GlyphLayout;
    static std::uint64_t kerning_key(char32_t first, char32_t second) noexcept;

    BitmapFontInfo info_;
    int line_height_{0};
    int baseline_{0};
    int page_width_{0};
    int page_height_{0};
    std::vector<BitmapFontPage> pages_;
    std::vector<BitmapGlyph> glyphs_;
    std::vector<BitmapKerning> kernings_;
    std::unordered_map<char32_t, std::size_t> glyph_indices_;
    std::unordered_map<std::uint64_t, int> kerning_amounts_;
};

} // namespace sq::graphics2d
