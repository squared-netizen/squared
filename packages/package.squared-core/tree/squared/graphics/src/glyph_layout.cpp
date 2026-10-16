#include "detail/bitmap_font_detail.hpp"

#include <squared/graphics/bitmap_font.hpp>
#include <squared/graphics/glyph_layout.hpp>
#include <squared/graphics/src/bitmap_font_error.hpp>
#include <squared/graphics/src/bitmap_font_error_code.hpp>
#include <squared/graphics/src/glyph_alignment.hpp>
#include <squared/graphics/src/glyph_layout_options.hpp>
#include <squared/graphics/src/glyph_line.hpp>
#include <squared/graphics/src/glyph_placement.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace sq::graphics
{

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

namespace
{

// The most glyphs one call may produce: a bound on what a tab-heavy text can
// expand to, well above any real text and far below what reserve() rejects.
constexpr std::size_t glyph_bound_limit = std::size_t{ 1 } << 24U;

} // namespace

BitmapFontError GlyphLayout::set_text( const BitmapFont &font, std::string_view text,
                                       const GlyphLayoutOptions &options ) noexcept
{
    BitmapFontError error;
    if( !font.valid() || !std::isfinite( options.scale ) || options.scale <= 0.0F ||
        !std::isfinite( options.target_width ) || options.target_width < 0.0F ||
        options.tab_spaces == 0 ||
        ( options.replacement != 0 && !valid_scalar( options.replacement ) ) ) {
        fail( error, BitmapFontErrorCode::invalid_argument, 0,
              "invalid font or glyph-layout options" );
        return error;
    }

    try {
        // Pass 1 reads the text without touching this layout: it rejects
        // invalid UTF-8 when asked to, and counts the most glyphs and lines the
        // text can produce. Growing the storage to that before anything is
        // written is what keeps the call transactional without building a
        // second layout: reserve() either succeeds or leaves the contents as
        // they were, and nothing after it allocates.
        std::size_t glyph_bound = 0;
        std::size_t line_count = 1;
        const auto add_glyphs = [&]( std::size_t count ) {
            if( count > glyph_bound_limit - glyph_bound ) {
                return false;
            }
            glyph_bound += count;
            return true;
        };
        for( std::size_t cursor = 0; cursor < text.size(); ) {
            const auto byte_offset = cursor;
            const auto decoded = decode_utf8( text, cursor );
            bool fits = true;
            if( !decoded ) {
                if( options.reject_invalid_utf8 ) {
                    fail( error, BitmapFontErrorCode::invalid_utf8, 0, "invalid UTF-8" );
                    error.byte = static_cast<std::uint32_t>( byte_offset );
                    return error;
                }
                fits = options.replacement == 0 || add_glyphs( 1 );
            } else if( *decoded == U'\r' ) {
                if( cursor < text.size() && text[cursor] == '\n' ) {
                    ++cursor;
                }
                ++line_count;
            } else if( *decoded == U'\n' ) {
                ++line_count;
            } else if( *decoded == U'\t' ) {
                fits = add_glyphs( options.tab_spaces );
            } else {
                fits = add_glyphs( 1 );
            }
            if( !fits ) {
                fail( error, BitmapFontErrorCode::limit_exceeded, 0, "text expands too far" );
                return error;
            }
        }
        glyphs_.reserve( glyph_bound );
        lines_.reserve( line_count );

        // Pass 2 lays the text out in place. Within the capacity reserved above
        // nothing here allocates or fails, so once it starts it finishes.
        glyphs_.clear();
        lines_.clear();
        width_ = 0.0F;
        line_height_ = static_cast<float>( font.line_height() ) * options.scale;

        std::size_t line_first = 0;
        float pen_x = 0.0F;
        float line_width = 0.0F;
        std::optional<char32_t> previous;

        const auto finish_line = [&]() {
            lines_.push_back( GlyphLine{ line_first, glyphs_.size() - line_first, line_width, 0.0F,
                                         static_cast<float>( lines_.size() ) * line_height_ } );
            width_ = std::max( width_, line_width );
            line_first = glyphs_.size();
            pen_x = 0.0F;
            line_width = 0.0F;
            previous.reset();
        };

        const auto place_scalar = [&]( char32_t scalar ) {
            const auto *glyph = font.glyph( scalar );
            char32_t rendered = scalar;
            if( glyph == nullptr && options.replacement != 0 ) {
                glyph = font.glyph( options.replacement );
                rendered = options.replacement;
            }
            if( glyph == nullptr ) {
                previous.reset();
                return;
            }
            if( previous ) {
                pen_x += static_cast<float>( font.kerning( *previous, rendered ) ) * options.scale;
            }
            const float width = static_cast<float>( glyph->width ) * options.scale;
            const float height = static_cast<float>( glyph->height ) * options.scale;
            const float x = pen_x + static_cast<float>( glyph->x_offset ) * options.scale;
            const float y = static_cast<float>( lines_.size() ) * line_height_ +
                            static_cast<float>( glyph->y_offset ) * options.scale;
            const float advance = static_cast<float>( glyph->x_advance ) * options.scale;
            glyphs_.push_back( GlyphPlacement{ rendered, glyph->page, glyph->x, glyph->y,
                                               glyph->width, glyph->height, x, y, width, height,
                                               advance } );
            pen_x += advance;
            line_width = std::max( { line_width, pen_x, x + width } );
            previous = rendered;
        };

        std::size_t cursor = 0;
        while( cursor < text.size() ) {
            const auto decoded = decode_utf8( text, cursor );
            if( !decoded ) {
                // Only reached when invalid UTF-8 is not rejected: pass 1 has
                // already returned otherwise.
                if( options.replacement != 0 ) {
                    place_scalar( options.replacement );
                }
                continue;
            }
            if( *decoded == U'\r' ) {
                if( cursor < text.size() && text[cursor] == '\n' ) {
                    ++cursor;
                }
                finish_line();
            } else if( *decoded == U'\n' ) {
                finish_line();
            } else if( *decoded == U'\t' ) {
                for( std::size_t index = 0; index < options.tab_spaces; ++index ) {
                    place_scalar( U' ' );
                }
            } else {
                place_scalar( *decoded );
            }
        }
        finish_line();

        if( options.target_width > 0.0F ) {
            width_ = options.target_width;
            for( auto &line : lines_ ) {
                const float remaining = std::max( 0.0F, options.target_width - line.width );
                if( options.alignment == GlyphAlignment::center ) {
                    line.x_offset = remaining * 0.5F;
                } else if( options.alignment == GlyphAlignment::end ) {
                    line.x_offset = remaining;
                }
                for( std::size_t index = line.first_glyph;
                     index < line.first_glyph + line.glyph_count; ++index ) {
                    glyphs_[index].x += line.x_offset;
                }
            }
        }
        height_ = static_cast<float>( lines_.size() ) * line_height_;
        return {};
    } catch( const std::bad_alloc & ) {
        fail( error, BitmapFontErrorCode::limit_exceeded, 0, "out of memory while laying out" );
        return error;
    } catch( ... ) {
        fail( error, BitmapFontErrorCode::malformed, 0, "text could not be laid out" );
        return error;
    }
}

void GlyphLayout::clear() noexcept
{
    // Keeps the storage, so the next set_text() of no longer text does not
    // allocate. Assign GlyphLayout{} to release it.
    width_ = 0.0F;
    height_ = 0.0F;
    line_height_ = 0.0F;
    glyphs_.clear();
    lines_.clear();
}

} // namespace sq::graphics
