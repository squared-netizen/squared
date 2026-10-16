#pragma once

#include <squared/graphics/src/bitmap_font_error_code.hpp>

#include <cstdint>
#include <type_traits>

namespace sq::graphics
{

/**
 * @brief What went wrong in `BitmapFont::load` or `GlyphLayout::set_text`.
 *
 * Returned by value, and true when something failed:
 * `if( const auto error = font.load( text ) ) { ... }`. It owns nothing: the
 * texts are fixed strings in the library, so building, copying or returning
 * one never allocates, even on the per-frame text path.
 */
struct BitmapFontError final
{
        /** @brief What failed, in words; never null, empty on success. */
        const char *message{ "" };
        /** @brief The descriptor field involved, such as "lineHeight"; empty when none. */
        const char *field{ "" };
        /** @brief One-based descriptor line, or zero when the failure is not about a line. */
        std::uint32_t line{ 0 };
        /** @brief Offset of the bad byte in the text, for `invalid_utf8`; zero otherwise. */
        std::uint32_t byte{ 0 };
        /** @brief The failure's category; `none` on success. */
        BitmapFontErrorCode code{ BitmapFontErrorCode::none };

        /** @brief True when something failed. */
        [[nodiscard]] explicit operator bool() const noexcept
        {
            return code != BitmapFontErrorCode::none;
        }
};

static_assert( sizeof( BitmapFontError ) == ( sizeof( void * ) == 8 ? 32 : 20 ),
               "BitmapFontError is 32 bytes (20 on 32-bit); see docs/developer/graphics.md" );
static_assert( std::is_trivially_copyable_v<BitmapFontError>,
               "BitmapFontError owns nothing, so it copies without allocating" );

} // namespace sq::graphics
