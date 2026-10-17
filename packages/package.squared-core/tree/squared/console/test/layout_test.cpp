// Where the Console's characters go, with nothing drawn: the quads layout()
// emits, read back into a grid of characters and checked row by row. Also the
// font atlas, and that laying out allocates nothing.

#include <squared/console/layout.hpp>
#include <squared/dev/frame_allocations.hpp>

#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace sq::console;

namespace
{

struct Screen
{
        std::vector<std::string> rows;   // the characters, ' ' where none
        std::vector<std::string> colour; // per character: t w e d c T, or ' '
        std::vector<Quad> quads;
        int cursor_column = -1;
        int cursor_row = -1;
};

char colour_letter( Colour colour )
{
    switch( colour ) {
        case Colour::text:
            return 't';
        case Colour::warning:
            return 'w';
        case Colour::error:
            return 'e';
        case Colour::dim:
            return 'd';
        case Colour::title:
            return 'T';
        case Colour::cursor:
            return 'c';
        case Colour::background:
            break;
    }
    return 'b';
}

Screen draw( const Console &console, const Frame &frame )
{
    Screen screen;
    const GridSize size = grid( frame );
    screen.rows.assign( static_cast<std::size_t>( size.rows ),
                        std::string( static_cast<std::size_t>( size.columns ), ' ' ) );
    screen.colour = screen.rows;
    layout( console, frame, [&]( const Quad &quad ) { screen.quads.push_back( quad ); } );
    for( const Quad &quad : screen.quads ) {
        if( quad.colour == Colour::background ) {
            continue;
        }
        assert( quad.width == frame.cell && quad.height == frame.cell );
        assert( ( quad.x - frame.x ) % frame.cell == 0 && ( quad.y - frame.y ) % frame.cell == 0 );
        const int column = ( quad.x - frame.x ) / frame.cell;
        const int row = ( quad.y - frame.y ) / frame.cell;
        assert( column >= 0 && column < size.columns && row >= 0 && row < size.rows );
        if( quad.colour == Colour::cursor ) {
            assert( quad.glyph == solid_glyph );
            screen.cursor_column = column;
            screen.cursor_row = row;
            continue;
        }
        auto &cell =
            screen.rows[static_cast<std::size_t>( row )][static_cast<std::size_t>( column )];
        assert( cell == ' ' ); // one character a cell
        cell = static_cast<char>( quad.glyph );
        screen.colour[static_cast<std::size_t>( row )][static_cast<std::size_t>( column )] =
            colour_letter( quad.colour );
    }
    return screen;
}

} // namespace

int main()
{
    // --- the atlas ----------------------------------------------------------------
    {
        auto atlas = std::make_unique<std::array<std::uint8_t, atlas_bytes>>();
        build_atlas( *atlas );
        const auto texel = [&]( std::size_t glyph, std::size_t column, std::size_t row ) {
            return ( *atlas )[( glyph / 16 * 8 + row ) * atlas_width + glyph % 16 * 8 + column];
        };
        int lit = 0;
        for( std::size_t row = 0; row < 8; ++row ) {
            for( std::size_t column = 0; column < 8; ++column ) {
                assert( texel( ' ', column, row ) == 0 );
                assert( texel( solid_glyph, column, row ) == 255 );
                lit += texel( 'A', column, row ) == 255 ? 1 : 0;
                // Row 0 of 'A' is 0x0C: bits 2 and 3, the font's bit 0 at the left.
                if( row == 0 ) {
                    assert( ( texel( 'A', column, row ) == 255 ) ==
                            ( column == 2 || column == 3 ) );
                }
            }
        }
        assert( lit > 10 && lit < 40 );
        for( const std::uint8_t value : *atlas ) {
            assert( value == 0 || value == 255 );
        }
    }

    auto console = std::make_unique<Console>();
    // 20 columns by 10 rows of 8-pixel cells, placed at (4, 6); 2 pixels left over.
    const Frame frame{ .x = 4, .y = 6, .width = 162, .height = 82, .cell = 8 };
    assert( grid( frame ).columns == 20 && grid( frame ).rows == 10 );

    // --- empty: the title, the prompt, the cursor -----------------------------------------
    {
        const Screen screen = draw( *console, frame );
        const Quad &back = screen.quads.front();
        assert( back.colour == Colour::background && back.glyph == solid_glyph );
        assert( back.x == 4 && back.y == 6 && back.width == 160 && back.height == 80 );
        assert( screen.rows[0] == " console         [x]" );
        assert( screen.colour[0] == " TTTTTTT         TTT" );
        for( int row = 1; row < 9; ++row ) {
            assert( screen.rows[static_cast<std::size_t>( row )] == std::string( 20, ' ' ) );
        }
        assert( screen.rows[9] == ">                   " && screen.colour[9][0] == 'd' );
        assert( screen.cursor_column == 2 && screen.cursor_row == 9 );
    }

    // --- lines: in order, coloured by level, cut into rows, non-ASCII as ? -------------
    {
        console->print( "hello", Level::info );
        console->print( "careful", Level::warning );
        console->print( "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHI",
                        Level::error );                            // 45: 3 rows
        console->print( "", Level::info );                         // still a row
        console->print( "caf\xC3\xA9 \xE2\x82\xAC", Level::info ); // é and €
        const Screen screen = draw( *console, frame );
        assert( screen.rows[1] == "hello               " &&
                screen.colour[1].substr( 0, 5 ) == "ttttt" );
        assert( screen.rows[2] == "careful             " && screen.colour[2][0] == 'w' );
        assert( screen.rows[3] == "abcdefghijklmnopqrst" && screen.colour[3][19] == 'e' );
        assert( screen.rows[4] == "uvwxyz0123456789ABCD" );
        assert( screen.rows[5] == "EFGHI               " && screen.colour[5][4] == 'e' );
        assert( screen.rows[6] == std::string( 20, ' ' ) );
        assert( screen.rows[7] == "caf? ?              " );
        assert( screen.rows[8] == std::string( 20, ' ' ) );
    }

    // --- more than fits: the newest at the bottom; scrolling back; clamped ------------------
    {
        console->run( "clear" );
        for( int i = 0; i < 20; ++i ) {
            console->printf( Level::info, "line %d", i );
        }
        Screen screen = draw( *console, frame );
        assert( screen.rows[1].rfind( "line 12", 0 ) == 0 );
        assert( screen.rows[8].rfind( "line 19", 0 ) == 0 );
        Frame back = frame;
        back.scroll = 3;
        screen = draw( *console, back );
        assert( screen.rows[1].rfind( "line 9 ", 0 ) == 0 &&
                screen.rows[8].rfind( "line 16", 0 ) == 0 );
        back.scroll = 1000;
        screen = draw( *console, back );
        assert( screen.rows[1].rfind( "line 0 ", 0 ) == 0 &&
                screen.rows[8].rfind( "line 7 ", 0 ) == 0 );
        back.scroll = -5;
        screen = draw( *console, back );
        assert( screen.rows[8].rfind( "line 19", 0 ) == 0 );

        // A wrapped line partly above the view shows only its last rows.
        console->print( "0123456789abcdefghij0123456789abcdefghijXY", Level::info ); // 42: 3 rows
        for( int i = 0; i < 6; ++i ) {
            console->printf( Level::info, "after %d", i );
        }
        screen = draw( *console, frame ); // 9 rows from the long line on; 8 shown
        assert( screen.rows[1] == "0123456789abcdefghij" );
        assert( screen.rows[2] == "XY                  " );
        assert( screen.rows[8].rfind( "after 5", 0 ) == 0 );

        // And one cut at the bottom of the view shows only its first rows.
        back.scroll = 7; // rows 14 to 21 of 29: the long line's first two at the bottom
        screen = draw( *console, back );
        assert( screen.rows[6].rfind( "line 19", 0 ) == 0 );
        assert( screen.rows[7] == "0123456789abcdefghij" &&
                screen.rows[8] == "0123456789abcdefghij" );
        assert( screen.rows[9] == ">                   " );
    }

    // --- control characters: a tab as a space, the rest as ? ------------------------
    {
        console->run( "clear" );
        console->print( "a\tb\x01"
                        "c\x7F"
                        "d",
                        Level::info );
        const Screen screen = draw( *console, frame );
        assert( screen.rows[1] == "a b?c?d             " );
    }

    // --- scroll_limit: the rows beyond the view ---------------------------------------------
    {
        console->run( "clear" ); // the echo is gone with the rest
        assert( scroll_limit( *console, frame ) == 0 );
        for( int i = 0; i < 8; ++i ) {
            console->print( "x" );
        }
        assert( scroll_limit( *console, frame ) == 0 ); // 8 rows, 8 shown
        console->print( "0123456789abcdefghij0" );      // 2 rows
        assert( scroll_limit( *console, frame ) == 2 );
        Frame back = frame;
        back.scroll = scroll_limit( *console, frame );
        const Screen screen = draw( *console, back );
        assert( screen.rows[1] == "x                   " &&
                screen.rows[8] == "x                   " );
        back.scroll = 1;
        assert( draw( *console, back ).rows[8] == "0123456789abcdefghij" );
        assert( scroll_limit( *console, Frame{ .width = 56, .height = 80, .cell = 8 } ) == 0 );
    }

    // --- the input row: typed text, the cursor, scrolled to keep the cursor in view ------
    {
        console->text( "help" );
        Screen screen = draw( *console, frame );
        assert( screen.rows[9] == "> help              " && screen.cursor_column == 6 );
        console->input().left();
        console->input().left();
        screen = draw( *console, frame );
        assert( screen.cursor_column == 4 && screen.rows[9] == "> help              " );

        console->input().clear();
        console->text( "abcdefghijklmnopqrstuvwxyz" ); // 26 into 18 columns
        screen = draw( *console, frame );
        assert( screen.cursor_column == 19 );
        assert( screen.rows[9] == "> jklmnopqrstuvwxyz " );
        console->input().home();
        screen = draw( *console, frame );
        assert( screen.cursor_column == 2 && screen.rows[9] == "> abcdefghijklmnopqr" );

        console->input().clear();
        console->text( "\xC3\xA9t\xC3\xA9" ); // été: 3 characters, 5 bytes
        screen = draw( *console, frame );
        assert( screen.rows[9] == "> ?t?               " && screen.cursor_column == 5 );
        console->input().clear();
    }

    // --- small frames: the background only; nothing at all -------------------------------
    {
        Screen screen =
            draw( *console, Frame{ .width = 56, .height = 80, .cell = 8 } ); // 7 columns
        assert( screen.quads.size() == 1 && screen.quads[0].colour == Colour::background );
        screen = draw( *console, Frame{ .width = 160, .height = 16, .cell = 8 } ); // 2 rows
        assert( screen.quads.size() == 1 );
        screen = draw( *console, Frame{ .width = 160, .height = 160, .cell = 0 } );
        assert( screen.quads.empty() );
        screen = draw( *console, Frame{ .width = 4, .height = 4, .cell = 8 } );
        assert( screen.quads.empty() );
    }

    // --- the close button --------------------------------------------------------------------
    {
        // Columns 17 to 19 of row 0: x from 4 + 136 = 140 to 164, y from 6 to 14.
        assert( touches_close( frame, 140, 6 ) && touches_close( frame, 163, 13 ) );
        assert( !touches_close( frame, 139, 6 ) && !touches_close( frame, 164, 6 ) );
        assert( !touches_close( frame, 150, 14 ) && !touches_close( frame, 150, 5 ) );
        assert( !touches_close( Frame{ .width = 56, .height = 80, .cell = 8 }, 50, 2 ) );
    }

    // --- nothing allocates ------------------------------------------------------------------------
    {
        for( int i = 0; i < 2000; ++i ) {
            console->printf( i % 3 == 0 ? Level::error : Level::info,
                             "row %d of a longer line, wrapped", i );
        }
        console->text( "spawn goblin" );
        std::size_t count = 0;
        const Frame tall{ .width = 1080, .height = 1200, .cell = 24, .scroll = 4 };
        sq::dev::frame_begin();
        for( int i = 0; i < 50; ++i ) {
            layout( *console, tall, [&]( const Quad & ) { ++count; } );
        }
        const sq::dev::FrameAllocations used = sq::dev::frame_end();
        assert( used.count == 0 && count > std::size_t{ 50 } * 400 );
        std::printf( "layout: all assertions passed  %zu quads a frame at 45x50 cells; "
                     "sizeof(Quad)=%zu sizeof(Frame)=%zu\n",
                     count / 50, sizeof( Quad ), sizeof( Frame ) );
    }
    return 0;
}
