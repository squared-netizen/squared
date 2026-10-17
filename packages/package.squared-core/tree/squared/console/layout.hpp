#pragma once

#include <squared/console/console.hpp>
#include <squared/console/src/colour.hpp>
#include <squared/console/src/font8x8.hpp>
#include <squared/console/src/frame.hpp>
#include <squared/console/src/quad.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sq::console
{

/**
 * @brief Where every character of the console goes, with no drawing.
 *
 * Calls `emit( Quad )` for each rectangle, back to front: the background, the
 * title row with its close button `[x]`, the newest lines that fit (wrapped,
 * newest at the bottom, moved back by `frame.scroll` rows), then the input row
 * with its prompt and cursor. A renderer batches the quads; a test collects
 * them. Nothing is allocated; it walks the scrollback twice.
 *
 * Characters outside ASCII are drawn as `?`: the font has 128 glyphs.
 *
 * ```cpp
 * sq::console::layout( console, frame, [&]( const sq::console::Quad &quad ) { batch.add( quad ); } );
 * ```
 */
template<typename Emit>
void layout( const Console &console, const Frame &frame, Emit emit );

/** @brief Columns and rows of characters `frame` holds. */
struct GridSize final
{
        int columns{ 0 };
        int rows{ 0 };
};

/** @brief The character grid of `frame`; zero when the cell size is not positive. */
[[nodiscard]] inline GridSize grid( const Frame &frame ) noexcept
{
    if( frame.cell <= 0 ) {
        return {};
    }
    return { std::max( frame.width, 0 ) / frame.cell, std::max( frame.height, 0 ) / frame.cell };
}

/** @brief True when the point (pixels) is on the close button, the last three cells of the title row. */
[[nodiscard]] inline bool touches_close( const Frame &frame, int x, int y ) noexcept
{
    const GridSize size = grid( frame );
    if( size.columns < 8 || size.rows < 3 ) {
        return false;
    }
    const int left = frame.x + ( size.columns - 3 ) * frame.cell;
    return x >= left && x < frame.x + size.columns * frame.cell && y >= frame.y &&
           y < frame.y + frame.cell;
}

namespace detail
{

/// The glyph for the character starting at text[at], and the start of the next.
[[nodiscard]] inline std::uint8_t glyph_at( std::string_view text, std::size_t &at ) noexcept
{
    const auto lead = static_cast<unsigned char>( text[at++] );
    while( at < text.size() && ( static_cast<unsigned char>( text[at] ) & 0xC0U ) == 0x80U ) {
        ++at;
    }
    if( lead == '\t' ) {
        return ' ';
    }
    return lead >= 0x20 && lead < 0x7F ? lead : static_cast<std::uint8_t>( '?' );
}

[[nodiscard]] inline std::size_t characters( std::string_view text ) noexcept
{
    std::size_t count = 0;
    for( const char c : text ) {
        count += ( static_cast<unsigned char>( c ) & 0xC0U ) != 0x80U ? 1 : 0;
    }
    return count;
}

[[nodiscard]] inline Colour colour_of( Level level ) noexcept
{
    switch( level ) {
        case Level::warning:
            return Colour::warning;
        case Level::error:
            return Colour::error;
        case Level::info:
            break;
    }
    return Colour::text;
}

} // namespace detail

template<typename Emit>
void layout( const Console &console, const Frame &frame, Emit emit )
{
    const GridSize size = grid( frame );
    const int cell = frame.cell;
    if( size.columns <= 0 || size.rows <= 0 ) {
        return;
    }
    emit( Quad{ frame.x, frame.y, size.columns * cell, size.rows * cell, solid_glyph,
                Colour::background } );
    if( size.columns < 8 || size.rows < 3 ) {
        return; // too small for anything but the background
    }

    const auto put = [&]( int column, int row, std::uint8_t glyph, Colour colour ) {
        emit( Quad{ frame.x + column * cell, frame.y + row * cell, cell, cell, glyph, colour } );
    };
    const auto text_at = [&]( int column, int row, std::string_view text, Colour colour ) {
        for( std::size_t at = 0; at < text.size() && column < size.columns; ++column ) {
            const std::uint8_t glyph = detail::glyph_at( text, at );
            if( glyph != ' ' ) {
                put( column, row, glyph, colour );
            }
        }
    };

    // The title row.
    text_at( 0, 0, " console", Colour::title );
    text_at( size.columns - 3, 0, "[x]", Colour::title );

    // The lines: the last ones that fit, wrapped at the grid's width.
    const int body = size.rows - 2;
    const auto columns = static_cast<std::size_t>( size.columns );
    std::size_t total = 0;
    for( const Line line : console.lines() ) {
        total +=
            std::max<std::size_t>( 1, ( detail::characters( line.text ) + columns - 1 ) / columns );
    }
    const auto visible = static_cast<std::size_t>( body );
    const std::size_t most_scroll = total > visible ? total - visible : 0;
    const std::size_t scroll =
        std::min( static_cast<std::size_t>( std::max( frame.scroll, 0 ) ), most_scroll );
    const std::size_t first = total > visible ? total - visible - scroll : 0;
    std::size_t wrapped_row = 0;
    for( const Line line : console.lines() ) {
        if( wrapped_row >= first + visible ) {
            break;
        }
        const Colour colour = detail::colour_of( line.level );
        std::size_t at = 0;
        do {
            const bool shown = wrapped_row >= first;
            const int row = 1 + static_cast<int>( wrapped_row - first );
            for( int column = 0; column < size.columns && at < line.text.size(); ++column ) {
                const std::uint8_t glyph = detail::glyph_at( line.text, at );
                if( shown && glyph != ' ' ) {
                    put( column, row, glyph, colour );
                }
            }
            ++wrapped_row;
        } while( at < line.text.size() && wrapped_row < first + visible );
    }

    // The input row: the prompt, then as much of the line as fits, scrolled so
    // the cursor shows.
    const int input_row = size.rows - 1;
    put( 0, input_row, '>', Colour::dim );
    const std::string_view typed = console.input().text();
    const std::size_t cursor = detail::characters( typed.substr( 0, console.input().cursor() ) );
    const auto room = static_cast<std::size_t>( size.columns - 2 );
    const std::size_t start = cursor >= room ? cursor - room + 1 : 0;
    put( 2 + static_cast<int>( cursor - start ), input_row, solid_glyph, Colour::cursor );
    std::size_t at = 0;
    for( std::size_t skip = start; skip > 0 && at < typed.size(); --skip ) {
        static_cast<void>( detail::glyph_at( typed, at ) );
    }
    text_at( 2, input_row, typed.substr( at ), Colour::text );
}

} // namespace sq::console
