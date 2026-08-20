#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace squared::graphics2d {

/** @brief Stable failure categories for BMFont parsing and glyph layout. */
enum class BitmapFontErrorCode {
    none,
    invalid_argument,
    limit_exceeded,
    malformed,
    unsupported,
    duplicate,
    missing_required,
    invalid_utf8
};

/** @brief One owned BMFont parsing or glyph-layout failure. */
struct BitmapFontError final {
    BitmapFontErrorCode code{BitmapFontErrorCode::none};
    /** @brief One-based descriptor line, or zero for non-line failures. */
    std::size_t line{0};
    std::string message;

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return code != BitmapFontErrorCode::none;
    }
};

/** @brief Explicit resource limits for one text BMFont descriptor. */
struct BitmapFontParseLimits final {
    std::size_t maximum_bytes{1024U * 1024U};
    std::size_t maximum_line_bytes{4096};
    std::size_t maximum_pages{64};
    std::size_t maximum_glyphs{65536};
    std::size_t maximum_kernings{262144};
    std::size_t maximum_name_bytes{512};
};

/** @brief Portable BMFont face metadata. */
struct BitmapFontInfo final {
    std::string face;
    int size{0};
    bool bold{false};
    bool italic{false};
    bool unicode{false};
};

/** @brief One texture-page dependency declared by a BMFont. */
struct BitmapFontPage final {
    std::size_t id{0};
    /** @brief Safe relative asset path resolved by the application. */
    std::string file;
};

/** @brief One Unicode scalar's source rectangle and layout metrics. */
struct BitmapGlyph final {
    char32_t codepoint{0};
    std::size_t page{0};
    int x{0};
    int y{0};
    int width{0};
    int height{0};
    int x_offset{0};
    int y_offset{0};
    int x_advance{0};
};

/** @brief Pair-specific horizontal advance adjustment in source pixels. */
struct BitmapKerning final {
    char32_t first{0};
    char32_t second{0};
    int amount{0};
};

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

/** @brief Horizontal placement within an optional target width. */
enum class GlyphAlignment { start, center, end };

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

/** @brief One explicit line in a GlyphLayout. */
struct GlyphLine final {
    std::size_t first_glyph{0};
    std::size_t glyph_count{0};
    float width{0.0F};
    float x_offset{0.0F};
    float y{0.0F};
};

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

} // namespace squared::graphics2d
