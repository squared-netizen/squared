// The terminal core (12.1): the UTF-8 decoder and the parser on their own,
// then the terminal against hand-written streams (text and wrapping,
// cursor movement, erasing, inserting and deleting, scroll regions, SGR
// colours, wide characters, the alternate screen, modes, replies, titles,
// tabs, resizing), the scrollback ring (trimming, eviction, wrap-around),
// a stream fed one byte at a time read the same as whole, random bytes, and
// no allocation while feeding; the screen as a snapshot (12.3c): written and
// read back, row versions, scrolled back into the history, a resize, a
// snapshot too short or of another layout.

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

// A test fails loudly, with its line, and the run goes on.
#define CHECK( condition )                                                                         \
    do {                                                                                           \
        if( !( condition ) ) {                                                                     \
            std::fprintf( stderr, "term_test.cpp:%d: %s\n", __LINE__, #condition );                \
            ++failures;                                                                            \
        }                                                                                          \
    } while( false )

#define CHECK_TEXT( terminal, expected )                                                           \
    do {                                                                                           \
        const std::string got = screen_text( terminal );                                           \
        if( got != ( expected ) ) {                                                                \
            std::fprintf( stderr, "term_test.cpp:%d: screen\n--- got\n%s\n--- expected\n%s\n",     \
                          __LINE__, got.c_str(), std::string( expected ).c_str() );                \
            ++failures;                                                                            \
        }                                                                                          \
    } while( false )

void append_utf8( std::string &out, char32_t code )
{
    if( code < 0x80 ) {
        out += static_cast<char>( code );
    } else if( code < 0x800 ) {
        out += static_cast<char>( 0xC0 | ( code >> 6 ) );
        out += static_cast<char>( 0x80 | ( code & 0x3F ) );
    } else if( code < 0x10000 ) {
        out += static_cast<char>( 0xE0 | ( code >> 12 ) );
        out += static_cast<char>( 0x80 | ( ( code >> 6 ) & 0x3F ) );
        out += static_cast<char>( 0x80 | ( code & 0x3F ) );
    } else {
        out += static_cast<char>( 0xF0 | ( code >> 18 ) );
        out += static_cast<char>( 0x80 | ( ( code >> 12 ) & 0x3F ) );
        out += static_cast<char>( 0x80 | ( ( code >> 6 ) & 0x3F ) );
        out += static_cast<char>( 0x80 | ( code & 0x3F ) );
    }
}

std::string line_text( std::span<const Cell> cells )
{
    std::string out;
    for( const Cell &cell : cells ) {
        if( cell.flags.wide_tail ) {
            continue;
        }
        append_utf8( out, cell.code );
    }
    while( !out.empty() && out.back() == ' ' ) {
        out.pop_back();
    }
    return out;
}

// The screen as text: each row, trailing blanks trimmed, rows joined by
// '\n', trailing empty rows dropped.
std::string screen_text( const Terminal &terminal )
{
    std::string out;
    std::size_t keep = 0;
    for( std::uint16_t r = 0; r < terminal.rows(); ++r ) {
        if( r > 0 ) {
            out += '\n';
        }
        out += line_text( terminal.row( r ) );
        if( !line_text( terminal.row( r ) ).empty() ) {
            keep = out.size();
        }
    }
    out.resize( keep );
    return out;
}

Terminal small( std::uint16_t columns = 10, std::uint16_t rows = 4, std::size_t scrollback = 4096 )
{
    return Terminal{ { .columns = columns, .rows = rows, .scrollback_bytes = scrollback } };
}

std::string replies_of( Terminal &terminal )
{
    const auto bytes = terminal.replies();
    std::string out( reinterpret_cast<const char *>( bytes.data() ), bytes.size() );
    terminal.clear_replies();
    return out;
}

// --- the decoder and the parser on their own ------------------------------------

void utf8_decoder()
{
    using detail::Utf8Decoder;
    const auto decode = []( std::string_view bytes ) {
        Utf8Decoder decoder;
        std::u32string out;
        for( std::size_t i = 0; i < bytes.size(); ++i ) {
            const auto byte = static_cast<std::uint8_t>( bytes[i] );
            const auto result = decoder.push( byte );
            if( result.step != Utf8Decoder::Step::more ) {
                out += result.code;
            }
            if( result.step == Utf8Decoder::Step::code_then_again ) {
                --i;
            }
        }
        return out;
    };
    CHECK( decode( "a\xC3\xA9\xE4\xB8\xAD\xF0\x9F\x98\x80" ) == U"a\u00E9\u4E2D\U0001F600" );
    // Overlong, surrogate, past U+10FFFF, a lone continuation, a cut sequence.
    CHECK( decode( "\xC0\xAF" ) == U"\uFFFD\uFFFD" );
    CHECK( decode( "\xED\xA0\x80" ) == U"\uFFFD\uFFFD\uFFFD" );
    CHECK( decode( "\xF4\x90\x80\x80" ) == U"\uFFFD\uFFFD\uFFFD\uFFFD" );
    CHECK( decode( "\x80z" ) == U"\uFFFDz" );
    CHECK( decode( "\xE4\xB8z" ) == U"\uFFFDz" );
}

struct Recorder
{
        std::vector<std::string> calls;

        void print( char32_t code )
        {
            std::string text = "print ";
            append_utf8( text, code );
            calls.push_back( text );
        }
        void execute( std::uint8_t control )
        {
            calls.push_back( std::format( "exec {:02x}", control ) );
        }
        void escape( const detail::CsiSequence &s )
        {
            calls.push_back( std::format(
                "esc {}{}", std::string_view{ s.intermediates.data(), s.intermediate_count },
                s.final ) );
        }
        void csi( const detail::CsiSequence &s )
        {
            std::string text = "csi ";
            if( s.private_marker != 0 ) {
                text += s.private_marker;
            }
            for( std::size_t i = 0; i < s.count; ++i ) {
                if( i > 0 ) {
                    text += s.is_sub( i ) ? ':' : ';';
                }
                text += std::to_string( s.parameters[i] );
            }
            text += std::string_view{ s.intermediates.data(), s.intermediate_count };
            text += s.final;
            calls.push_back( text );
        }
        void osc( std::string_view text ) { calls.push_back( "osc " + std::string{ text } ); }
};

std::vector<std::string> parse( std::string_view input )
{
    detail::Parser parser;
    Recorder recorder;
    parser.feed( std::as_bytes( std::span{ input } ), recorder );
    return recorder.calls;
}

void parser()
{
    using V = std::vector<std::string>;
    CHECK( parse( "a\x1b[1;2Hb" ) == ( V{ "print a", "csi 1;2H", "print b" } ) );
    CHECK( parse( "\x1b[H" ) == ( V{ "csi H" } ) );
    CHECK( parse( "\x1b[;5H" ) == ( V{ "csi 0;5H" } ) );
    CHECK( parse( "\x1b[5;H" ) == ( V{ "csi 5;0H" } ) );
    CHECK( parse( "\x1b[?1049h" ) == ( V{ "csi ?1049h" } ) );
    CHECK( parse( "\x1b[38:2::255:0:10m" ) == ( V{ "csi 38:2:0:255:0:10m" } ) );
    CHECK( parse( "\x1b[99999m" ) == ( V{ "csi 65535m" } ) );
    CHECK( parse( "\x1b[ q" ) == ( V{ "csi  q" } ) );
    CHECK( parse( "\x1b(B\x1b"
                  "7" ) == ( V{ "esc (B", "esc 7" } ) );
    // C0 inside a sequence runs where it stands; CAN ends the sequence.
    CHECK( parse( "\x1b[1\n2H" ) == ( V{ "exec 0a", "csi 12H" } ) );
    CHECK( parse( "\x1b[12\x18H" ) == ( V{ "print H" } ) );
    // A private marker after a parameter: the sequence is ignored.
    CHECK( parse( "\x1b[1?2Hx" ) == ( V{ "print x" } ) );
    // OSC, ended by BEL or by ST; too long is dropped.
    CHECK( parse( "\x1b]2;title\x07" ) == ( V{ "osc 2;title" } ) );
    CHECK( parse( "\x1b]0;t\x1b\\x" ) == ( V{ "osc 0;t", "esc \\", "print x" } ) );
    CHECK( parse( "\x1b]2;" + std::string( 300, 'a' ) + "\x07z" ) == ( V{ "print z" } ) );
    // DCS is read and ignored.
    CHECK( parse( "\x1bPq#0;1\x1b\\y" ) == ( V{ "esc \\", "print y" } ) );
    // C1 as code points: U+009B is CSI.
    CHECK( parse( "\xC2\x9B"
                  "2J" ) == ( V{ "csi 2J" } ) );
    // UTF-8 inside a sequence ends it; the byte is read as text.
    CHECK( parse( "\x1b[1\xC3\xA9" ) == ( V{ "print \xC3\xA9" } ) );
    // DEL is ignored anywhere.
    CHECK( parse( "a\x7f"
                  "b" ) == ( V{ "print a", "print b" } ) );
}

// --- the terminal -------------------------------------------------------------

void text_and_wrapping()
{
    Terminal t = small();
    t.feed( "hello\r\nworld" );
    CHECK_TEXT( t, "hello\nworld" );
    CHECK( ( t.cursor() == Cursor{ 5, 1 } ) );

    // The last column: the cursor waits there; the next character wraps.
    t = small();
    t.feed( "0123456789" );
    CHECK( ( t.cursor() == Cursor{ 9, 0 } ) );
    t.feed( "ab" );
    CHECK_TEXT( t, "0123456789\nab" );

    // A CR at the waiting position does not wrap.
    t = small();
    t.feed( "0123456789\rX" );
    CHECK_TEXT( t, "X123456789" );

    // Autowrap off: the last column is overwritten.
    t = small();
    t.feed( "\x1b[?7l0123456789ab" );
    CHECK_TEXT( t, "012345678b" );
    CHECK( !t.modes().autowrap );

    // Scrolling at the bottom; the top line goes to the scrollback.
    t = small( 10, 3 );
    t.feed( "1\r\n2\r\n3\r\n4" );
    CHECK_TEXT( t, "2\n3\n4" );
    CHECK( t.history_size() == 1 );
    CHECK( line_text( t.history_line( 0 ) ) == "1" );
}

void cursor_and_erasing()
{
    Terminal t = small();
    t.feed( "abcdefghij\r\nklmnopqrst\r\nuvwxyz" );
    t.feed( "\x1b[2;3H" );
    CHECK( ( t.cursor() == Cursor{ 2, 1 } ) );
    t.feed( "\x1b[K" );
    CHECK_TEXT( t, "abcdefghij\nkl\nuvwxyz" );
    t.feed( "\x1b[1;5H\x1b[1K" );
    CHECK_TEXT( t, "     fghij\nkl\nuvwxyz" );
    t.feed( "\x1b[3;2H\x1b[J" );
    CHECK_TEXT( t, "     fghij\nkl\nu" );
    t.feed( "\x1b[2J" );
    CHECK_TEXT( t, "" );

    // Movement is clamped to the screen.
    t.feed( "\x1b[99;99H" );
    CHECK( ( t.cursor() == Cursor{ 9, 3 } ) );
    t.feed( "\x1b[99A\x1b[99D" );
    CHECK( ( t.cursor() == Cursor{ 0, 0 } ) );
    t.feed( "\x1b[3C\x1b[2B" );
    CHECK( ( t.cursor() == Cursor{ 3, 2 } ) );
    t.feed( "\x1b[5G\x1b[2d" );
    CHECK( ( t.cursor() == Cursor{ 4, 1 } ) );
    t.feed( "\x1b[E" );
    CHECK( ( t.cursor() == Cursor{ 0, 2 } ) );

    // ECH, ICH, DCH.
    t = small();
    t.feed( "abcdefghij\x1b[1;3H\x1b[2X" );
    CHECK_TEXT( t, "ab  efghij" );
    t.feed( "\x1b[2@" );
    CHECK_TEXT( t, "ab    efgh" );
    t.feed( "\x1b[3P" );
    CHECK_TEXT( t, "ab efgh" );

    // REP repeats the last character.
    t = small();
    t.feed( "x\x1b[3b" );
    CHECK_TEXT( t, "xxxx" );
}

void lines_and_regions()
{
    Terminal t = small( 10, 5 );
    t.feed( "1\r\n2\r\n3\r\n4\r\n5" );
    t.feed( "\x1b[2;1H\x1b[L" );
    CHECK_TEXT( t, "1\n\n2\n3\n4" );
    t.feed( "\x1b[M" );
    CHECK_TEXT( t, "1\n2\n3\n4" );

    // A region: rows 2 to 4. Scrolling inside it keeps rows 1 and 5, and
    // sends nothing to the scrollback.
    t = small( 10, 5 );
    t.feed( "1\r\n2\r\n3\r\n4\r\n5" );
    t.feed( "\x1b[2;4r" );
    CHECK( ( t.cursor() == Cursor{ 0, 0 } ) );
    t.feed( "\x1b[4;1H\nx" );
    CHECK_TEXT( t, "1\n3\n4\nx\n5" );
    CHECK( t.history_size() == 0 );
    t.feed( "\x1b[2;1H\x1bMy" );
    CHECK_TEXT( t, "1\ny\n3\n4\n5" );
    t.feed( "\x1b[2S" );
    CHECK_TEXT( t, "1\n4\n\n\n5" );
    t.feed( "\x1b[T" );
    CHECK_TEXT( t, "1\n\n4\n\n5" );
    // CUU stops at the top margin from inside the region.
    t.feed( "\x1b[3;1H\x1b[9A" );
    CHECK( t.cursor().row == 1 );
    // The region reset: the whole screen.
    t.feed( "\x1b[r" );
    t.feed( "\x1b[5;1H\n" );
    CHECK( t.history_size() == 1 );
}

void colours_and_attributes()
{
    Terminal t = small();
    t.feed( "\x1b[1;3;4;31;42ma\x1b[0mb" );
    const Cell a = t.row( 0 )[0];
    CHECK( a.attributes.bold && a.attributes.italic && a.attributes.underline );
    CHECK( !a.flags.default_foreground && a.foreground == ( Rgb{ 205, 0, 0 } ) );
    CHECK( !a.flags.default_background && a.background == ( Rgb{ 0, 205, 0 } ) );
    const Cell b = t.row( 0 )[1];
    CHECK( b.attributes == CellAttributes{} && b.flags.default_foreground &&
           b.flags.default_background );

    t.feed( "\x1b[38;5;196;48;2;1;2;3mc" );
    const Cell c = t.row( 0 )[2];
    CHECK( c.foreground == ( Rgb{ 255, 0, 0 } ) && c.background == ( Rgb{ 1, 2, 3 } ) );

    t.feed( "\x1b[38:2::10:20:30;48:5:21md" );
    const Cell d = t.row( 0 )[3];
    CHECK( d.foreground == ( Rgb{ 10, 20, 30 } ) );
    CHECK( d.background == ( Rgb{ 0, 0, 255 } ) );

    t.feed( "\x1b[38:2:40:50:60;94;4:0me" );
    const Cell e = t.row( 0 )[4];
    CHECK( e.foreground == ( Rgb{ 92, 92, 255 } ) );
    CHECK( !e.attributes.underline );

    t.feed( "\x1b[22;39;49;7mf" );
    const Cell f = t.row( 0 )[5];
    CHECK( f.attributes.inverse && !f.attributes.bold && f.flags.default_foreground );

    // Erasing takes the background.
    t = small();
    t.feed( "\x1b[44m\x1b[2K" );
    CHECK( t.row( 0 )[9].background == ( Rgb{ 0, 0, 238 } ) );
    CHECK( !t.row( 0 )[9].flags.default_background );
}

void wide_and_combining()
{
    Terminal t = small( 6, 3 );
    t.feed( "\xE4\xB8\xAD"
            "a" );
    CHECK( t.row( 0 )[0].flags.wide && t.row( 0 )[1].flags.wide_tail );
    CHECK( ( t.cursor() == Cursor{ 3, 0 } ) );
    CHECK_TEXT( t, "\xE4\xB8\xAD"
                   "a" );

    // A combining mark is dropped; the letter stays.
    t = small( 6, 3 );
    t.feed( "e\xCC\x81x" );
    CHECK_TEXT( t, "ex" );

    // A wide character at the last column wraps first.
    t = small( 6, 3 );
    t.feed( "abcde\xE4\xB8\xAD" );
    CHECK_TEXT( t, "abcde\n\xE4\xB8\xAD" );

    // Overwriting half of a wide character blanks the other half.
    t = small( 6, 3 );
    t.feed( "\xE4\xB8\xAD\r\x1b[Cx" );
    CHECK_TEXT( t, " x" );
    CHECK( !t.row( 0 )[0].flags.wide );

    // Invalid UTF-8 shows U+FFFD.
    t = small( 6, 3 );
    t.feed( "\xFFok" );
    CHECK_TEXT( t, "\xEF\xBF\xBDok" );
}

void alternate_screen_and_modes()
{
    Terminal t = small( 10, 3 );
    t.feed( "shell\x1b[31m" );
    t.feed( "\x1b[?1049h" );
    CHECK( t.modes().alternate_screen );
    CHECK_TEXT( t, "" );
    t.feed( "\x1b[0mnvim\r\n1\r\n2\r\n3\r\n4" );
    CHECK( t.history_size() == 0 );
    t.feed( "\x1b[?1049l" );
    CHECK( !t.modes().alternate_screen );
    CHECK_TEXT( t, "shell" );
    CHECK( ( t.cursor() == Cursor{ 5, 0 } ) );
    t.feed( "x" );
    CHECK( !t.row( 0 )[5].flags.default_foreground ); // the pen came back too

    t.feed( "\x1b[?1h\x1b[?25l\x1b[?2004h\x1b[4h" );
    CHECK( t.modes().application_cursor_keys );
    CHECK( !t.modes().cursor_visible );
    CHECK( t.modes().bracketed_paste );
    CHECK( t.modes().insert );
    t.feed( "\x1b[?1;2004l\x1b[?25h\x1b[4l" );
    CHECK( ( t.modes() == TerminalModes{} ) );

    // Insert mode pushes the line right.
    t = small();
    t.feed( "abc\r\x1b[4hX" );
    CHECK_TEXT( t, "Xabc" );

    // Save and restore the cursor.
    t = small();
    t.feed( "ab\x1b"
            "7\x1b[3;3Hz\x1b"
            "8c" );
    CHECK_TEXT( t, "abc\n\n  z" );
}

void replies_title_and_bell()
{
    Terminal t = small();
    t.feed( "\x1b[3;4H\x1b[6n\x1b[5n\x1b[c\x1b[>c" );
    CHECK( replies_of( t ) == "\x1b[3;4R\x1b[0n\x1b[?62;22c\x1b[>0;0;0c" );
    CHECK( t.replies().empty() );

    t.feed( "\x1b]2;my title\x07" );
    CHECK( t.title() == "my title" );
    TerminalEvents events = t.take_events();
    CHECK( events.title && !events.bell );
    CHECK( !t.take_events() );
    t.feed( "\x07" );
    CHECK( t.take_events().bell );
    t.feed( "\x1b]0;" + std::string( 200, 'x' ) + "\x1b\\" );
    CHECK( t.title().size() == 128 );
    t.feed( "\x1b]52;c;aGk=\x07" );
    CHECK( t.title().size() == 128 );
}

void tabs()
{
    Terminal t = small( 20, 2 );
    t.feed( "a\tb\tc" );
    CHECK( t.row( 0 )[8].code == U'b' && t.row( 0 )[16].code == U'c' );
    t.feed( "\x1b[Z" );
    CHECK( t.cursor().column == 16 );
    t.feed( "\x1b[3g\r\t" );
    CHECK( t.cursor().column == 19 );
    t.feed( "\r\x1b[4C\x1bH\r\t" );
    CHECK( t.cursor().column == 4 );
}

void resizing()
{
    Terminal t = small( 10, 4 );
    t.feed( "1\r\n2\r\n3\r\n4" );
    t.resize( 5, 2 );
    // The cursor's line stays on screen; the lines above went to the scrollback.
    CHECK_TEXT( t, "3\n4" );
    CHECK( t.history_size() == 2 );
    CHECK( ( t.cursor() == Cursor{ 1, 1 } ) );
    t.resize( 8, 3 );
    CHECK( t.columns() == 8 && t.rows() == 3 );
    CHECK_TEXT( t, "3\n4" );
    for( std::uint16_t r = 0; r < t.rows(); ++r ) {
        CHECK( t.row_dirty( r ) );
    }
    t.clear_dirty();
    t.feed( "\x1b[3;1Hx" );
    CHECK( !t.row_dirty( 0 ) && t.row_dirty( 2 ) );
}

void reset()
{
    Terminal t = small();
    t.feed( "\x1b[31m\x1b[?25l\x1b[2;3rtext\x1b"
            "c" );
    CHECK_TEXT( t, "" );
    CHECK( ( t.modes() == TerminalModes{} ) );
    t.feed( "a" );
    CHECK( t.row( 0 )[0].flags.default_foreground );
}

// --- the scrollback ---------------------------------------------------------------

void scrollback_ring()
{
    // Room for 10 cells: a line of 3 takes 5.
    detail::Scrollback ring;
    ring.reset( 10 * sizeof( Cell ) );
    std::array<Cell, 6> line{};
    const auto make = [&line]( std::string_view text ) {
        for( std::size_t i = 0; i < line.size(); ++i ) {
            line[i] = Cell{};
            if( i < text.size() ) {
                line[i].code = static_cast<char32_t>( text[i] );
            }
        }
        return std::span<const Cell>{ line };
    };
    ring.push( make( "abc" ), false );
    ring.push( make( "de" ), true );
    CHECK( ring.size() == 2 );
    CHECK( line_text( ring.line( 0 ).cells ) == "de" && ring.line( 0 ).wrapped );
    CHECK( line_text( ring.line( 1 ).cells ) == "abc" && !ring.line( 1 ).wrapped );
    CHECK( ring.line( 0 ).cells.size() == 2 ); // trimmed
    // 5 + 4 used of 10: a third line of 2 (4 cells) wraps round and pushes out "abc".
    ring.push( make( "fg" ), false );
    CHECK( ring.size() == 2 );
    CHECK( line_text( ring.line( 0 ).cells ) == "fg" );
    CHECK( line_text( ring.line( 1 ).cells ) == "de" );
    // Many more: always the newest, in order.
    for( int i = 0; i < 1000; ++i ) {
        const std::string text = std::to_string( i % 100 );
        ring.push( make( text ), false );
        CHECK( line_text( ring.line( 0 ).cells ) == text );
        if( i > 0 && ring.size() > 1 ) {
            CHECK( line_text( ring.line( 1 ).cells ) == std::to_string( ( i + 99 ) % 100 ) );
        }
    }
    // A blank line takes 2 cells, and is kept.
    ring.push( make( "" ), false );
    CHECK( ring.line( 0 ).cells.empty() );
    // Longer than the ring: cut to fit.
    std::array<Cell, 20> wide_line{};
    std::fill( wide_line.begin(), wide_line.end(), Cell{ .code = U'x' } );
    ring.push( wide_line, false );
    CHECK( ring.size() == 1 && ring.line( 0 ).cells.size() == 8 );
    ring.clear();
    CHECK( ring.size() == 0 );

    // None at all.
    Terminal t = small( 10, 2, 0 );
    t.feed( "1\r\n2\r\n3" );
    CHECK( t.history_size() == 0 );

    // Through the terminal, with a real ring: every line, in order, a model to check.
    Terminal big = small( 20, 5, 64 * 1024 );
    for( int i = 0; i < 5000; ++i ) {
        const std::string text = "line " + std::to_string( i );
        big.feed( text + "\r\n" );
    }
    const std::size_t kept = big.history_size();
    CHECK( kept > 100 );
    for( std::size_t back = 0; back < kept; ++back ) {
        const int expected = 5000 - 4 - static_cast<int>( back ) - 1 + 0;
        CHECK( line_text( big.history_line( back ) ) == "line " + std::to_string( expected ) );
    }
}

// --- whole streams ------------------------------------------------------------------

std::string sample_stream()
{
    std::string s;
    s += "\x1b]0;user@host: ~\x07";
    s += "\x1b[1;32muser@host\x1b[0m:\x1b[1;34m~\x1b[0m$ ls --color\r\n";
    s += "\x1b[0m\x1b[01;34mdir\x1b[0m  file.txt  \x1b[01;32mrun.sh\x1b[0m\r\n";
    s += "caf\xC3\xA9 \xE4\xB8\xAD\xE6\x96\x87 \xF0\x9F\x98\x80 done\r\n";
    s += "\x1b[?1049h\x1b[H\x1b[2J\x1b[1;24r\x1b[38:2::200:100:50mfull\x1b[m\x1b[?1049l";
    s += "\x1b[3A\x1b[2K\x1b[5;10H\x1b[1Pz\x1b[2@\x1b[6n";
    for( int i = 0; i < 40; ++i ) {
        s += "row " + std::to_string( i ) + "\twith\ttabs\r\n";
    }
    return s;
}

void split_feeding()
{
    const std::string stream = sample_stream();
    Terminal whole = small( 30, 8 );
    whole.feed( stream );
    Terminal bytes = small( 30, 8 );
    for( const char c : stream ) {
        bytes.feed( std::string_view{ &c, 1 } );
    }
    CHECK( screen_text( whole ) == screen_text( bytes ) );
    CHECK( whole.cursor() == bytes.cursor() );
    CHECK( whole.history_size() == bytes.history_size() );
    CHECK( whole.title() == bytes.title() );
    CHECK( replies_of( whole ) == replies_of( bytes ) );
    for( std::uint16_t r = 0; r < whole.rows(); ++r ) {
        CHECK( std::ranges::equal( whole.row( r ), bytes.row( r ) ) );
    }
}

void no_allocation_while_feeding()
{
    Terminal t = small( 40, 10, 16 * 1024 );
    const std::string stream = sample_stream();
    const std::size_t before = allocations;
    for( int i = 0; i < 20; ++i ) {
        t.feed( stream );
        t.clear_replies();
    }
    CHECK( allocations == before );
}

void random_bytes()
{
    std::mt19937 random{ 12345 };
    std::uniform_int_distribution<int> byte{ 0, 255 };
    std::uniform_int_distribution<int> kind{ 0, 9 };
    Terminal t = small( 13, 7, 2048 );
    std::string chunk;
    for( int round = 0; round < 2000; ++round ) {
        chunk.clear();
        for( int i = 0; i < 64; ++i ) {
            // Mostly sequence-shaped bytes, so sequences are reached.
            const int k = kind( random );
            if( k < 3 ) {
                chunk += '\x1b';
                chunk += k == 0 ? '[' : ( k == 1 ? ']' : '(' );
            } else if( k < 6 ) {
                chunk += static_cast<char>( "0123456789;:?>"[byte( random ) % 14] );
            } else {
                chunk += static_cast<char>( byte( random ) );
            }
        }
        t.feed( chunk );
        t.clear_replies();
        if( round % 500 == 0 ) {
            t.resize( static_cast<std::uint16_t>( 1 + byte( random ) % 30 ),
                      static_cast<std::uint16_t>( 1 + byte( random ) % 12 ) );
        }
        CHECK( t.cursor().column < t.columns() && t.cursor().row < t.rows() );
    }
    for( std::size_t back = 0; back < t.history_size(); ++back ) {
        CHECK( t.history_line( back ).size() <= 4096 );
    }
}

std::vector<std::byte> publish( ScreenPublisher &publisher, Terminal &terminal )
{
    std::vector<std::byte> bytes( publisher.bytes_for( terminal ) );
    CHECK( publisher.write( terminal, bytes ) == bytes.size() );
    return bytes;
}

std::string view_text( const ScreenView &view )
{
    std::string out;
    for( std::uint16_t r = 0; r < view.rows(); ++r ) {
        const std::string line = line_text( view.row( r ) );
        out += line;
        out += '\n';
    }
    while( out.ends_with( "\n\n" ) ) {
        out.pop_back();
    }
    return out;
}

void screen_snapshots()
{
    Terminal t = small( 10, 3, 4096 );
    ScreenPublisher publisher;
    t.feed( "\x1b]2;shell\x07one\r\ntwo\x1b[31mX" );
    std::vector<std::byte> bytes = publish( publisher, t );
    std::optional<ScreenView> view = ScreenView::read( bytes );
    CHECK( view.has_value() );
    if( !view ) {
        return;
    }
    CHECK( view->columns() == 10 && view->rows() == 3 );
    CHECK( view_text( *view ) == "one\ntwo\n" || view_text( *view ) == "one\ntwoX\n" );
    CHECK( line_text( view->row( 1 ) ) == "twoX" );
    CHECK( view->row( 1 )[3].foreground == ( Rgb{ 205, 0, 0 } ) );
    CHECK( ( view->cursor() == Cursor{ 4, 1 } ) && view->cursor_shown() );
    CHECK( view->title() == "shell" );
    CHECK( view->history_size() == 0 && view->scrolled_back() == 0 );
    const std::array<std::uint32_t, 3> first{ view->version( 0 ), view->version( 1 ),
                                              view->version( 2 ) };
    // Written: the terminal's dirty marks are cleared.
    CHECK( !t.row_dirty( 0 ) && !t.row_dirty( 1 ) );

    // One row changed: only its version moves.
    t.feed( "\x1b[3;1Hthree" );
    bytes = publish( publisher, t );
    view = ScreenView::read( bytes );
    CHECK( view && view->version( 0 ) == first[0] && view->version( 1 ) == first[1] &&
           view->version( 2 ) != first[2] );
    CHECK( view && line_text( view->row( 2 ) ) == "three" );

    // The cursor hidden by the program.
    t.feed( "\x1b[?25l" );
    bytes = publish( publisher, t );
    view = ScreenView::read( bytes );
    CHECK( view && !view->cursor_shown() );
    t.feed( "\x1b[?25h" );

    // Scrolled back: the history's lines, then the screen's; every row new.
    t.feed( "\r\nfour\r\nfive" ); // "one" and "two" scroll off
    CHECK( t.history_size() == 2 );
    publisher.scroll_back( 1 );
    bytes = publish( publisher, t );
    view = ScreenView::read( bytes );
    CHECK( view && view->scrolled_back() == 1 && view->history_size() == 2 );
    CHECK( view && view_text( *view ) == "twoX\nthree\nfour\n" );
    CHECK( view && !view->cursor_shown() );
    // Further back than there is: held to the history.
    publisher.scroll_back( 99 );
    bytes = publish( publisher, t );
    view = ScreenView::read( bytes );
    CHECK( view && view->scrolled_back() == 2 && view_text( *view ) == "one\ntwoX\nthree\n" );
    const std::uint32_t before = view ? view->version( 0 ) : 0;
    // Back to the screen: every row new again.
    publisher.scroll_back( 0 );
    bytes = publish( publisher, t );
    view = ScreenView::read( bytes );
    CHECK( view && view->scrolled_back() == 0 && view->version( 0 ) != before );
    CHECK( view && view_text( *view ) == "three\nfour\nfive\n" );

    // A resize: the new size, every row new.
    t.resize( 6, 2 );
    bytes = publish( publisher, t );
    view = ScreenView::read( bytes );
    CHECK( view && view->columns() == 6 && view->rows() == 2 );

    // Too small a buffer: nothing written.
    std::vector<std::byte> short_buffer( 10 );
    CHECK( publisher.write( t, short_buffer ) == 0 );
    // Not a snapshot: refused.
    CHECK( !ScreenView::read( std::span<const std::byte>{ bytes.data(), 20 } ) );
    std::vector<std::byte> wrong = bytes;
    wrong[0] = std::byte{ 9 }; // another layout
    CHECK( !ScreenView::read( wrong ) );
    CHECK( !ScreenView::read( std::span<const std::byte>{ bytes.data(), bytes.size() - 1 } ) );
}

} // namespace

int main()
{
    utf8_decoder();
    parser();
    text_and_wrapping();
    cursor_and_erasing();
    lines_and_regions();
    colours_and_attributes();
    wide_and_combining();
    alternate_screen_and_modes();
    replies_title_and_bell();
    tabs();
    resizing();
    reset();
    scrollback_ring();
    split_feeding();
    no_allocation_while_feeding();
    random_bytes();
    screen_snapshots();
    if( failures > 0 ) {
        std::fprintf( stderr, "term_test: %d failed\n", failures );
        return 1;
    }
    std::printf( "term_test: ok\n" );
    return 0;
}
