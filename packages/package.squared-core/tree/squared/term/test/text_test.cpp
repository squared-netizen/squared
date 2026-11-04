// Text out of a terminal and into it (12.4): line numbers that stay with
// their lines as more scroll off and out; copy_text over the screen, the
// scrollback and both (wrapped lines joined, trailing blanks left out, wide
// characters once, the points either way round, held to the lines there
// are, whole characters into a short buffer); the scrollback walked forward
// across its ring's end; the snapshot's top line; paste_text (ESC and C1
// taken out, newlines as CR, bracketed paste, sizing). No allocation in
// either.

#include <stdio.h>

import sq.term;

import std;

namespace
{
std::size_t allocations = 0;
} // namespace

void *operator new( std::size_t size )
{
    ++allocations;
    void *p = std::malloc( size == 0 ? 1 : size ); // NOLINT(cppcoreguidelines-no-malloc)
    if( p == nullptr ) {
        std::abort();
    }
    return p;
}
void *operator new[]( std::size_t size )
{
    return operator new( size );
}
void operator delete( void *p ) noexcept
{
    std::free( p ); // NOLINT(cppcoreguidelines-no-malloc)
}
void operator delete( void *p, std::size_t /*size*/ ) noexcept
{
    std::free( p ); // NOLINT(cppcoreguidelines-no-malloc)
}
void operator delete[]( void *p ) noexcept
{
    std::free( p ); // NOLINT(cppcoreguidelines-no-malloc)
}
void operator delete[]( void *p, std::size_t /*size*/ ) noexcept
{
    std::free( p ); // NOLINT(cppcoreguidelines-no-malloc)
}

using namespace sq::term;

namespace
{

int failures = 0;

#define CHECK( condition )                                                                         \
    do {                                                                                           \
        if( !( condition ) ) {                                                                     \
            std::fprintf( stderr, "text_test.cpp:%d: %s\n", __LINE__, #condition );                \
            ++failures;                                                                            \
        }                                                                                          \
    } while( false )

#define CHECK_EQ( got, expected )                                                                  \
    do {                                                                                           \
        const std::string got_ = ( got );                                                          \
        const std::string expected_ = ( expected );                                                \
        if( got_ != expected_ ) {                                                                  \
            std::fprintf( stderr, "text_test.cpp:%d: got [%s], expected [%s]\n", __LINE__,         \
                          got_.c_str(), expected_.c_str() );                                       \
            ++failures;                                                                            \
        }                                                                                          \
    } while( false )

std::string copy( const Terminal &terminal, TextPoint from, TextPoint to )
{
    std::string text( terminal.copy_text( from, to, {} ), '\0' );
    const std::size_t again = terminal.copy_text( from, to, text );
    CHECK( again == text.size() );
    return text;
}

std::string paste( std::string_view text, bool bracketed )
{
    TerminalModes modes;
    modes.bracketed_paste = bracketed;
    std::string out( paste_text( text, modes, {} ), '\0' );
    CHECK( paste_text( text, modes, out ) == out.size() );
    return out;
}

void line_numbers()
{
    Terminal terminal{ { .columns = 10, .rows = 3 } };
    CHECK( terminal.first_line() == 0 && terminal.screen_line() == 0 );
    terminal.feed( "one\r\ntwo\r\nthree\r\nfour\r\nfive" );
    // Two lines went up: "one" is line 0, the screen starts at line 2.
    CHECK( terminal.first_line() == 0 );
    CHECK( terminal.screen_line() == 2 );
    CHECK_EQ( copy( terminal, { 0, 0 }, { 0, 9 } ), "one" );
    CHECK_EQ( copy( terminal, { 2, 0 }, { 2, 9 } ), "three" );

    // A scrollback with room for few lines: the oldest go, the numbers stay.
    Terminal small{ { .columns = 10, .rows = 2, .scrollback_bytes = 12 * 12 } };
    for( int i = 0; i < 20; ++i ) {
        std::array<char, 16> line{};
        const int n = std::snprintf( line.data(), line.size(), "line%d\r\n", i );
        small.feed( std::string_view( line.data(), static_cast<std::size_t>( n ) ) );
    }
    CHECK( small.screen_line() == 19 );
    CHECK( small.first_line() > 0 );
    CHECK( small.first_line() + small.history_size() == small.screen_line() );
    const std::uint64_t oldest = small.first_line();
    std::array<char, 16> want{};
    const int n = std::snprintf( want.data(), want.size(), "line%d", static_cast<int>( oldest ) );
    CHECK_EQ( copy( small, { oldest, 0 }, { oldest, 9 } ),
              std::string( want.data(), static_cast<std::size_t>( n ) ) );
    // Asked for from before the oldest kept: from the oldest.
    CHECK_EQ( copy( small, { 0, 0 }, { oldest, 9 } ),
              std::string( want.data(), static_cast<std::size_t>( n ) ) );
    // No scrollback at all: still counted.
    Terminal none{ { .columns = 10, .rows = 2, .scrollback_bytes = 0 } };
    none.feed( "a\r\nb\r\nc\r\nd" );
    CHECK( none.screen_line() == 2 && none.first_line() == 2 );
    CHECK_EQ( copy( none, { 0, 0 }, { 9, 9 } ), "c\nd" );
}

void screen_copy()
{
    Terminal terminal{ { .columns = 10, .rows = 4 } };
    terminal.feed( "hello  \r\nworld\r\n\r\nend" );
    // Whole lines: trailing blanks out, LF between, none after the last.
    CHECK_EQ( copy( terminal, { 0, 0 }, { 3, 9 } ), "hello\nworld\n\nend" );
    // Part of one line; either order.
    CHECK_EQ( copy( terminal, { 0, 1 }, { 0, 3 } ), "ell" );
    CHECK_EQ( copy( terminal, { 0, 3 }, { 0, 1 } ), "ell" );
    // From the middle of one line to the middle of another.
    CHECK_EQ( copy( terminal, { 0, 3 }, { 1, 2 } ), "lo\nwor" );
    CHECK_EQ( copy( terminal, { 1, 2 }, { 0, 3 } ), "lo\nwor" );
    // Past the last line: held to it. Past every line: nothing.
    CHECK_EQ( copy( terminal, { 3, 0 }, { 40, 0 } ), "end" );
    CHECK( terminal.copy_text( { 9, 0 }, { 12, 0 }, {} ) == 0 );
    // A selection of blanks only: nothing (but the LF of a line it ends).
    CHECK_EQ( copy( terminal, { 2, 0 }, { 2, 9 } ), "" );
    CHECK_EQ( copy( terminal, { 1, 7 }, { 2, 3 } ), "\n" );
}

void wrapped_lines()
{
    Terminal terminal{ { .columns = 5, .rows = 3 } };
    terminal.feed( "abcdefgh\r\nxy" );
    // "abcde" wrapped into "fgh": one line of text, no LF between.
    CHECK( terminal.row_wrapped( 0 ) );
    CHECK( !terminal.row_wrapped( 1 ) );
    CHECK_EQ( copy( terminal, { 0, 0 }, { 2, 4 } ), "abcdefgh\nxy" );
    // A wrapped line's trailing spaces are the text's: "ab  " + "  cd".
    Terminal spaces{ { .columns = 4, .rows = 2 } };
    spaces.feed( "ab    cd" );
    CHECK( spaces.row_wrapped( 0 ) );
    CHECK_EQ( copy( spaces, { 0, 0 }, { 1, 3 } ), "ab    cd" );
    // Wrapped lines in the scrollback too.
    Terminal history{ { .columns = 5, .rows = 2 } };
    history.feed( "0123456789ab\r\nz" );
    CHECK( history.history_size() >= 1 );
    CHECK_EQ( copy( history, { history.first_line(), 0 }, { history.screen_line() + 1, 4 } ),
              "0123456789ab\nz" );
}

void wide_characters()
{
    Terminal terminal{ { .columns = 10, .rows = 2 } };
    terminal.feed( "a\xe4\xb8\xad"
                   "b" ); // a, U+4E2D (two cells), b
    CHECK_EQ( copy( terminal, { 0, 0 }, { 0, 9 } ), "a\xe4\xb8\xad"
                                                    "b" );
    // Starting on its second cell gives nothing of it.
    CHECK_EQ( copy( terminal, { 0, 2 }, { 0, 9 } ), "b" );
    terminal.feed( "\r\n\xf0\x9f\x98\x80" ); // U+1F600, four bytes
    CHECK_EQ( copy( terminal, { 1, 0 }, { 1, 9 } ), "\xf0\x9f\x98\x80" );
}

void across_scrollback()
{
    Terminal terminal{ { .columns = 8, .rows = 3 } };
    for( int i = 0; i < 10; ++i ) {
        std::array<char, 16> line{};
        const int n = std::snprintf( line.data(), line.size(), "row%d\r\n", i );
        terminal.feed( std::string_view( line.data(), static_cast<std::size_t>( n ) ) );
    }
    // Lines 0..9 written, the screen is rows 8, 9 and the blank line 10.
    CHECK( terminal.screen_line() == 8 );
    CHECK_EQ( copy( terminal, { 6, 1 }, { 9, 2 } ), "ow6\nrow7\nrow8\nrow" );
    CHECK_EQ( copy( terminal, { 0, 0 }, { 2, 9 } ), "row0\nrow1\nrow2" );
    CHECK_EQ( copy( terminal, { 7, 0 }, { 7, 9 } ), "row7" );
    // The alternate screen: the scrollback is the main screen's, the rows the program's.
    terminal.feed( "\x1b[?1049h\x1b[H"
                   "alt" ); // the cursor stays where it was: home first
    CHECK_EQ( copy( terminal, { 7, 0 }, { 8, 9 } ), "row7\nalt" );
    terminal.feed( "\x1b[?1049l" );
}

void ring_end()
{
    // A ring that wraps: the forward walk goes on at its start.
    Terminal terminal{ { .columns = 6, .rows = 1, .scrollback_bytes = 12 * 20 } };
    for( int i = 0; i < 30; ++i ) {
        std::array<char, 16> line{};
        const int n = std::snprintf( line.data(), line.size(), "%c%d\r\n", 'a' + ( i % 26 ), i );
        terminal.feed( std::string_view( line.data(), static_cast<std::size_t>( n ) ) );
    }
    const std::uint64_t first = terminal.first_line();
    const std::uint64_t screen = terminal.screen_line();
    CHECK( screen - first == terminal.history_size() );
    std::string expected;
    for( std::uint64_t line = first; line < screen; ++line ) {
        std::array<char, 16> text{};
        const int n =
            std::snprintf( text.data(), text.size(), "%c%d\n",
                           static_cast<char>( 'a' + ( line % 26 ) ), static_cast<int>( line ) );
        expected.append( text.data(), static_cast<std::size_t>( n ) );
    }
    CHECK_EQ( copy( terminal, { first, 0 }, { screen, 5 } ), expected );
    // Each line alone agrees with history_line.
    for( std::uint64_t line = first; line < screen; ++line ) {
        const std::span<const Cell> cells =
            terminal.history_line( static_cast<std::size_t>( screen - 1 - line ) );
        std::string got = copy( terminal, { line, 0 }, { line, 5 } );
        CHECK( got.size() == cells.size() );
    }
}

void short_buffer()
{
    Terminal terminal{ { .columns = 10, .rows = 2 } };
    terminal.feed( "ab\xc3\xa9"
                   "cd" ); // a b é c d
    std::array<char, 3> three{};
    // "ab" then é (two bytes) does not fit: it stops there, whole.
    CHECK( terminal.copy_text( { 0, 0 }, { 0, 9 }, three ) == 6 );
    CHECK( three[0] == 'a' && three[1] == 'b' && three[2] == '\0' );
    std::array<char, 4> four{};
    CHECK( terminal.copy_text( { 0, 0 }, { 0, 9 }, four ) == 6 );
    CHECK( std::string_view( four.data(), 4 ) == "ab\xc3\xa9" );
}

void pasted()
{
    CHECK_EQ( paste( "ls -l", false ), "ls -l" );
    CHECK_EQ( paste( "one\ntwo\r\nthree\rfour", false ), "one\rtwo\rthree\rfour" );
    CHECK_EQ( paste( "a\x1b[201~b", false ), "a[201~b" );
    CHECK_EQ( paste( "x\xc2\x9b"
                     "1m\xc2\xa9",
                     false ),
              "x1m\xc2\xa9" ); // CSI as C1 out; © kept
    CHECK_EQ( paste( "echo hi\n", true ), "\x1b[200~echo hi\r\x1b[201~" );
    CHECK_EQ( paste( "", true ), "\x1b[200~\x1b[201~" );
    CHECK_EQ( paste( "tab\there", false ), "tab\there" );
    CHECK_EQ( paste( "\xe4\xb8\xad\xf0\x9f\x98\x80", false ), "\xe4\xb8\xad\xf0\x9f\x98\x80" );
    // Invalid UTF-8 passes as it is.
    CHECK_EQ( paste( "a\xff"
                     "b\xc3",
                     false ),
              "a\xff"
              "b\xc3" );
    // A short buffer: whole characters, then nothing.
    TerminalModes modes;
    std::array<char, 4> out{};
    CHECK( paste_text( "ab\xe4\xb8\xad", modes, out ) == 5 );
    CHECK( out[0] == 'a' && out[1] == 'b' && out[2] == '\0' );
}

void snapshot_top_line()
{
    Terminal terminal{ { .columns = 6, .rows = 2 } };
    terminal.feed( "a\r\nb\r\nc\r\nd" );
    ScreenPublisher publisher;
    std::vector<std::byte> bytes( publisher.bytes_for( terminal ) );
    CHECK( publisher.write( terminal, bytes ) == bytes.size() );
    auto view = ScreenView::read( bytes );
    CHECK( view && view->top_line() == terminal.screen_line() );
    publisher.scroll_back( 1 );
    CHECK( publisher.write( terminal, bytes ) == bytes.size() );
    view = ScreenView::read( bytes );
    CHECK( view && view->top_line() == terminal.screen_line() - 1 );
    // The view's row r is line top_line + r: the same text either way.
    const std::span<const Cell> row0 = view->row( 0 );
    CHECK( row0[0].code == U'b' );
    CHECK_EQ( copy( terminal, { view->top_line(), 0 }, { view->top_line(), 5 } ), "b" );
}

void no_allocation()
{
    Terminal terminal{ { .columns = 20, .rows = 4 } };
    for( int i = 0; i < 40; ++i ) {
        terminal.feed( "some text that wraps across the columns\r\n" );
    }
    std::array<char, 4096> out{};
    TerminalModes modes;
    modes.bracketed_paste = true;
    const std::size_t before = allocations;
    const std::size_t copied =
        terminal.copy_text( { terminal.first_line(), 0 }, { terminal.screen_line() + 3, 19 }, out );
    const std::size_t pasted = paste_text( "a\nb\x1b", modes, out );
    CHECK( allocations == before );
    CHECK( copied > 0 && pasted == 15 );
}

} // namespace

int main()
{
    line_numbers();
    screen_copy();
    wrapped_lines();
    wide_characters();
    across_scrollback();
    ring_end();
    short_buffer();
    pasted();
    snapshot_top_line();
    no_allocation();
    if( failures != 0 ) {
        std::fprintf( stderr, "text_test: %d failed\n", failures );
        return 1;
    }
    return 0;
}
