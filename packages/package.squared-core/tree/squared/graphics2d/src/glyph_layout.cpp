#include <squared/graphics2d/glyph_layout.hpp>

#include "detail/bitmap_font_detail.hpp"
#include <squared/graphics2d/bitmap_font.hpp>
#include <squared/graphics2d/bitmap_font_error.hpp>
#include <squared/graphics2d/bitmap_font_error_code.hpp>
#include <squared/graphics2d/glyph_alignment.hpp>
#include <squared/graphics2d/glyph_layout_options.hpp>
#include <squared/graphics2d/glyph_line.hpp>
#include <squared/graphics2d/glyph_placement.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace sq::graphics2d {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

bool GlyphLayout::set_text(
    const BitmapFont& font,
    std::string_view text,
    const GlyphLayoutOptions& options,
    BitmapFontError& error
) noexcept
{
    error = {};
    if (!font.valid() || !std::isfinite(options.scale) || options.scale <= 0.0F ||
        !std::isfinite(options.target_width) || options.target_width < 0.0F ||
        options.tab_spaces == 0 ||
        (options.replacement != 0 && !valid_scalar(options.replacement))) {
        fail(error, BitmapFontErrorCode::invalid_argument, 0,
             "invalid font or glyph-layout options");
        return false;
    }

    try {
        GlyphLayout candidate;
        candidate.line_height_ = static_cast<float>(font.line_height()) * options.scale;
        candidate.glyphs_.reserve(text.size());

        std::size_t line_first = 0;
        float pen_x = 0.0F;
        float line_width = 0.0F;
        std::optional<char32_t> previous;

        const auto finish_line = [&]() {
            candidate.lines_.push_back(GlyphLine{
                line_first,
                candidate.glyphs_.size() - line_first,
                line_width,
                0.0F,
                static_cast<float>(candidate.lines_.size()) * candidate.line_height_
            });
            candidate.width_ = std::max(candidate.width_, line_width);
            line_first = candidate.glyphs_.size();
            pen_x = 0.0F;
            line_width = 0.0F;
            previous.reset();
        };

        const auto place_scalar = [&](char32_t scalar) {
            const auto* glyph = font.glyph(scalar);
            char32_t rendered = scalar;
            if (glyph == nullptr && options.replacement != 0) {
                glyph = font.glyph(options.replacement);
                rendered = options.replacement;
            }
            if (glyph == nullptr) {
                previous.reset();
                return;
            }
            if (previous) {
                pen_x += static_cast<float>(font.kerning(*previous, rendered)) *
                         options.scale;
            }
            const float width = static_cast<float>(glyph->width) * options.scale;
            const float height = static_cast<float>(glyph->height) * options.scale;
            const float x = pen_x +
                            static_cast<float>(glyph->x_offset) * options.scale;
            const float y = static_cast<float>(candidate.lines_.size()) *
                                candidate.line_height_ +
                            static_cast<float>(glyph->y_offset) * options.scale;
            const float advance =
                static_cast<float>(glyph->x_advance) * options.scale;
            candidate.glyphs_.push_back(GlyphPlacement{
                rendered, glyph->page, glyph->x, glyph->y, glyph->width,
                glyph->height, x, y, width, height, advance
            });
            pen_x += advance;
            line_width = std::max({line_width, pen_x, x + width});
            previous = rendered;
        };

        std::size_t cursor = 0;
        while (cursor < text.size()) {
            const auto byte_offset = cursor;
            const auto decoded = decode_utf8(text, cursor);
            if (!decoded) {
                if (options.reject_invalid_utf8) {
                    fail(error, BitmapFontErrorCode::invalid_utf8, 0,
                         "invalid UTF-8 at byte " + std::to_string(byte_offset));
                    return false;
                }
                if (options.replacement != 0) {
                    place_scalar(options.replacement);
                }
                continue;
            }
            if (*decoded == U'\r') {
                if (cursor < text.size() && text[cursor] == '\n') {
                    ++cursor;
                }
                finish_line();
            } else if (*decoded == U'\n') {
                finish_line();
            } else if (*decoded == U'\t') {
                for (std::size_t index = 0; index < options.tab_spaces; ++index) {
                    place_scalar(U' ');
                }
            } else {
                place_scalar(*decoded);
            }
        }
        finish_line();

        if (options.target_width > 0.0F) {
            candidate.width_ = options.target_width;
            for (auto& line : candidate.lines_) {
                const float remaining =
                    std::max(0.0F, options.target_width - line.width);
                if (options.alignment == GlyphAlignment::center) {
                    line.x_offset = remaining * 0.5F;
                } else if (options.alignment == GlyphAlignment::end) {
                    line.x_offset = remaining;
                }
                for (std::size_t index = line.first_glyph;
                     index < line.first_glyph + line.glyph_count; ++index) {
                    candidate.glyphs_[index].x += line.x_offset;
                }
            }
        }
        candidate.height_ = static_cast<float>(candidate.lines_.size()) *
                            candidate.line_height_;
        *this = std::move(candidate);
        return true;
    } catch (const std::bad_alloc&) {
        error.code = BitmapFontErrorCode::limit_exceeded;
        error.line = 0;
        error.message.clear();
        return false;
    } catch (...) {
        error.code = BitmapFontErrorCode::malformed;
        error.line = 0;
        error.message.clear();
        return false;
    }
}

void GlyphLayout::clear() noexcept
{
    *this = GlyphLayout{};
}

} // namespace sq::graphics2d
