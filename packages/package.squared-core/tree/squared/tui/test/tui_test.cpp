// squared-tui's first layers: the screen and its views; the renderer, read
// back through sq.term's emulator (what a person would see, and that a
// frame writes only what changed); the input decoder against the bytes
// xterm sends, split across reads; the Terminal's raw mode in a pty, and
// that it puts the terminal back.

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

import sq.term;
import sq.tui;

import std;

namespace
{

#define CHECK( ... )                                                                               \
    do {                                                                                           \
        if( !( __VA_ARGS__ ) ) {                                                                   \
            std::fprintf( stderr, "tui_test.cpp:%d: %s\n", __LINE__, #__VA_ARGS__ );               \
            std::abort();                                                                          \
        }                                                                                          \
    } while( false )

using sq::tui::Event;
using sq::tui::Key;
using sq::tui::KeyName;
using sq::tui::Modifiers;
using sq::tui::Mouse;
using sq::tui::MouseAction;
using sq::tui::MouseButton;
using sq::tui::Paste;
using sq::tui::Rect;
using sq::tui::Style;

Key named( KeyName code, Modifiers modifiers = {} )
{
    return { code, 0, modifiers };
}

Key ch( char32_t c, Modifiers modifiers = {} )
{
    return { KeyName::unknown, c, modifiers };
}

bool one( const std::vector<Event> &events, const Event &want )
{
    return events.size() == 1 && events[0] == want;
}

void screen_and_views()
{
    sq::tui::Screen screen( 10, 3 );
    CHECK( screen.put( 0, 0, "hello" ) == 5 );
    CHECK( screen.cell( 4, 0 ).code == U'o' );
    // Clipped at the right edge.
    CHECK( screen.put( 7, 1, "abcdef" ) == 10 );
    CHECK( screen.cell( 9, 1 ).code == U'c' );
    // A wide character takes two cells; with one left, a blank.
    CHECK( screen.put( 0, 2, "中x" ) == 3 );
    CHECK( screen.cell( 0, 2 ).flags.wide );
    CHECK( screen.cell( 1, 2 ).flags.wide_tail );
    CHECK( screen.cell( 2, 2 ).code == U'x' );
    CHECK( screen.put( 9, 2, U'中' ) == 10 );
    CHECK( screen.cell( 9, 2 ).code == U' ' );
    // Writing over half of one blanks the other half.
    screen.put( 1, 2, "y" );
    CHECK( screen.cell( 0, 2 ).code == U' ' );
    CHECK( !screen.cell( 0, 2 ).flags.wide );
    CHECK( screen.cell( 1, 2 ).code == U'y' );
    // A view: its own (0, 0), clipped to itself.
    sq::tui::Screen s( 20, 5 );
    sq::tui::View v = s.view( Rect{ 5, 1, 4, 2 } );
    CHECK( v.put( 0, 0, "abcdefg" ) == 4 );
    CHECK( s.cell( 5, 1 ).code == U'a' );
    CHECK( s.cell( 8, 1 ).code == U'd' );
    CHECK( s.cell( 9, 1 ).code == U' ' );
    v.put( 0, 5, "nothing" ); // below it
    CHECK( s.cell( 5, 3 ).code == U' ' );
    sq::tui::View inner = v.view( Rect{ 1, 1, 10, 10 } );
    CHECK( inner.area() == ( Rect{ 6, 2, 3, 1 } ) );
    s.box( Rect{ 0, 0, 3, 3 } );
    CHECK( s.cell( 0, 0 ).code == U'┌' );
    CHECK( s.cell( 2, 2 ).code == U'┘' );
    CHECK( s.cell( 1, 1 ).code == U' ' );
    // Bad UTF-8 is U+FFFD, not dropped.
    sq::tui::Screen u( 5, 1 );
    u.put( 0, 0,
           "a\xFF"
           "b" );
    CHECK( u.cell( 1, 0 ).code == U'�' );
    CHECK( u.cell( 2, 0 ).code == U'b' );
}

void renders_what_a_person_sees()
{
    sq::tui::Test t( 20, 4 );
    t.screen().put( 2, 1, "> Open" );
    t.screen().put( 0, 3, "中文", Style{ .foreground = sq::tui::rgb( 0xFF8800 ) } );
    t.show();
    CHECK( t.text( 0, 1 ) == "  > Open" );
    CHECK( t.text( 2, 1 ) == "> Open" );
    CHECK( t.text( 0, 3 ) == "中文" );
    CHECK( t.cell( 0, 3 ).flags.wide );
    CHECK( !t.cell( 0, 3 ).flags.default_foreground );
    CHECK( t.cell( 0, 3 ).foreground == sq::tui::rgb( 0xFF8800 ) );
    CHECK( t.cell( 2, 1 ).flags.default_foreground );

    // The same frame again writes nothing.
    t.show();
    CHECK( t.output().empty() );

    // One cell changed: one cell written, not the screen.
    t.screen().put( 2, 1, "]" );
    t.show();
    CHECK( t.text( 2, 1 ) == "] Open" );
    CHECK( t.output().size() < 40 );
    CHECK( t.output().find( "Open" ) == std::string_view::npos );

    // Attributes, and back to plain after them.
    sq::tui::Style bold;
    bold.attributes.bold = true;
    t.screen().put( 0, 0, "B", bold );
    t.screen().put( 1, 0, "p" );
    t.show();
    CHECK( t.cell( 0, 0 ).attributes.bold );
    CHECK( !t.cell( 1, 0 ).attributes.bold );

    // The cursor: hidden unless asked for, then where it was put.
    CHECK( !t.cursor() );
    t.screen().show_cursor( { 4, 2 } );
    t.show();
    CHECK( t.cursor() == ( sq::tui::Point{ 4, 2 } ) );

    // A wide character replaced by two narrow ones, and back.
    t.screen().put( 0, 3, "ab" );
    t.show();
    CHECK( t.text( 0, 3 ) == "ab文" );
    t.screen().put( 1, 3, "中" );
    t.show();
    CHECK( t.text( 0, 3 ) == "a中" );

    // A new size: the next frame is written whole.
    t.resize( 10, 2 );
    t.screen().clear();
    t.screen().put( 0, 1, "small" );
    t.show();
    CHECK( t.text( 0, 1 ) == "small" );
    CHECK( t.text( 0, 0 ).empty() );
}

void decodes_keys()
{
    sq::tui::Test t( 10, 2 );
    CHECK( one( t.type( "a" ), Event{ ch( U'a' ) } ) );
    CHECK( one( t.type( "é" ), Event{ ch( U'é' ) } ) );
    CHECK( ch( U'q' ) == U'q' );
    CHECK( !( ch( U'q', { .control = true } ) == U'q' ) );
    CHECK( one( t.type( "\x01" ), Event{ ch( U'a', { .control = true } ) } ) );
    CHECK( one( t.type( "\r" ), Event{ named( KeyName::enter ) } ) );
    CHECK( one( t.type( "\t" ), Event{ named( KeyName::tab ) } ) );
    CHECK( one( t.type( "\x7f" ), Event{ named( KeyName::backspace ) } ) );
    CHECK( one( t.type( "\x08" ), Event{ named( KeyName::backspace, { .control = true } ) } ) );
    CHECK( one( t.type( "\x1b\x1b" ), Event{ named( KeyName::escape, { .alt = true } ) } ) );
    CHECK( one( t.type( "\x1b[A" ), Event{ named( KeyName::up ) } ) );
    CHECK( one( t.type( "\x1bOB" ), Event{ named( KeyName::down ) } ) );
    CHECK( one( t.type( "\x1b[1;5C" ), Event{ named( KeyName::right, { .control = true } ) } ) );
    CHECK( one( t.type( "\x1b[1;2D" ), Event{ named( KeyName::left, { .shift = true } ) } ) );
    CHECK( one( t.type( "\x1b[H" ), Event{ named( KeyName::home ) } ) );
    CHECK( one( t.type( "\x1b[4~" ), Event{ named( KeyName::end ) } ) );
    CHECK( one( t.type( "\x1b[3~" ), Event{ named( KeyName::delete_key ) } ) );
    CHECK( one( t.type( "\x1b[3;3~" ), Event{ named( KeyName::delete_key, { .alt = true } ) } ) );
    CHECK( one( t.type( "\x1b[5~" ), Event{ named( KeyName::page_up ) } ) );
    CHECK( one( t.type( "\x1bOP" ), Event{ named( KeyName::f1 ) } ) );
    CHECK( one( t.type( "\x1b[1;2P" ), Event{ named( KeyName::f1, { .shift = true } ) } ) );
    CHECK( one( t.type( "\x1b[15~" ), Event{ named( KeyName::f5 ) } ) );
    CHECK( one( t.type( "\x1b[24~" ), Event{ named( KeyName::f12 ) } ) );
    CHECK( one( t.type( "\x1b[Z" ), Event{ named( KeyName::tab, { .shift = true } ) } ) );
    CHECK( one( t.type( "\x1bx" ), Event{ ch( U'x', { .alt = true } ) } ) );
    CHECK( one( t.type( "\x1b" ), Event{ named( KeyName::escape ) } ) );
    // Several in one read.
    const auto three = t.type( "ab\x1b[B" );
    CHECK( three.size() == 3 && three[2] == Event{ named( KeyName::down ) } );
    // A sequence it does not know goes, whole, and what follows is read.
    CHECK( one( t.type( "\x1b[99;99zq" ), Event{ ch( U'q' ) } ) );
}

/**
 * The decoder is the inverse of sq.term's key_bytes, what sqterm sends for
 * a key: every named key, with every mix of Shift, Ctrl and Alt, in both
 * cursor-key modes, decodes back to itself, except where the bytes are the
 * same for two keys (below).
 */
void inverse_of_key_bytes()
{
    using sq::term::key_bytes;
    using sq::term::TerminalModes;
    const std::array<TerminalModes, 2> modes{ TerminalModes{},
                                              TerminalModes{ .application_cursor_keys = true } };
    int checked = 0;
    for( auto k = static_cast<int>( KeyName::left ); k <= static_cast<int>( KeyName::f12 ); ++k ) {
        const auto key = static_cast<KeyName>( k );
        if( key == KeyName::space ) {
            continue; // a space is text
        }
        for( unsigned bits = 0; bits < 8; ++bits ) {
            const Modifiers m{ .shift = ( bits & 1U ) != 0,
                               .control = ( bits & 2U ) != 0,
                               .alt = ( bits & 4U ) != 0 };
            for( const TerminalModes mode : modes ) {
                const sq::term::KeyBytes bytes = key_bytes( key, m, mode );
                if( bytes.empty() ) {
                    continue;
                }
                // What a terminal cannot tell apart: Enter, Tab, Escape and
                // Backspace send one byte that Shift (and, but for
                // Backspace and Tab's Shift, Ctrl) does not change; Shift+Tab
                // is one sequence, Alt or not.
                Modifiers want = m;
                if( key == KeyName::enter || key == KeyName::escape ) {
                    want.shift = false;
                    want.control = false;
                }
                if( key == KeyName::backspace ) {
                    want.shift = false;
                }
                if( key == KeyName::tab ) {
                    want.control = false;
                    if( m.shift ) {
                        want.alt = false; // Shift+Tab is CSI Z, with Alt or without
                    }
                }
                sq::tui::InputDecoder d;
                std::vector<Event> out;
                d.feed( bytes.view(), out );
                d.flush( out );
                if( !one( out, Event{ named( key, want ) } ) ) {
                    std::fprintf( stderr,
                                  "key %d shift %d ctrl %d alt %d app %d: decoded %zu event(s)\n",
                                  k, int{ m.shift }, int{ m.control }, int{ m.alt },
                                  int{ mode.application_cursor_keys }, out.size() );
                }
                CHECK( one( out, Event{ named( key, want ) } ) );
                ++checked;
            }
        }
    }
    CHECK( checked > 300 );

    // Text with Ctrl or Alt (key_text_bytes): the letter back, lower case
    // under Ctrl.
    for( char c = 'a'; c <= 'z'; ++c ) {
        const std::string text( 1, c );
        for( unsigned bits = 1; bits < 4; ++bits ) {
            const Modifiers m{ .control = ( bits & 1U ) != 0, .alt = ( bits & 2U ) != 0 };
            const sq::term::KeyBytes bytes = sq::term::key_text_bytes( text, m );
            sq::tui::InputDecoder d;
            std::vector<Event> out;
            d.feed( bytes.view(), out );
            d.flush( out );
            const bool tab_or_enter = m.control && ( c == 'i' || c == 'm' || c == 'j' || c == 'h' );
            if( !tab_or_enter ) {
                CHECK( one( out, Event{ ch( static_cast<char32_t>( c ), m ) } ) );
            }
        }
    }
}

/** A paste as sqterm sends it (sq.term's paste_text, bracketed) is one Paste. */
void inverse_of_paste_text()
{
    const std::string text = "line one\nline two \xE4\xB8\xAD";
    sq::term::TerminalModes modes;
    modes.bracketed_paste = true;
    std::string bytes( sq::term::paste_text( text, modes, {} ), '\0' );
    sq::term::paste_text( text, modes, bytes );
    sq::tui::InputDecoder d;
    std::vector<Event> out;
    d.feed( bytes, out );
    CHECK( out.size() == 1 );
    const Paste *paste = std::get_if<Paste>( &out[0] );
    CHECK( paste != nullptr );
    // paste_text sends newlines as CR, as a terminal does.
    std::string want = text;
    std::ranges::replace( want, '\n', '\r' );
    CHECK( paste->text == want || paste->text == text );
}

void decodes_across_reads()
{
    sq::tui::InputDecoder d;
    std::vector<Event> out;
    d.feed( "\x1b[1;", out );
    CHECK( out.empty() );
    CHECK( d.waiting() );
    d.feed( "5A", out );
    CHECK( one( out, Event{ named( KeyName::up, { .control = true } ) } ) );
    out.clear();
    // Half a UTF-8 character, then the rest.
    d.feed( "\xE4\xB8", out );
    CHECK( out.empty() );
    d.feed( "\xAD", out );
    CHECK( one( out, Event{ ch( U'中' ) } ) );
    out.clear();
    // An escape, then nothing: flush says Escape.
    d.feed( "\x1b", out );
    CHECK( out.empty() );
    d.flush( out );
    CHECK( one( out, Event{ named( KeyName::escape ) } ) );
}

void decodes_paste_mouse_focus()
{
    sq::tui::Test t( 10, 2 );
    CHECK( one( t.type( "\x1b[200~hi\nthere\x1b[201~" ), Event{ Paste{ "hi\nthere" } } ) );
    // Split anywhere, the end marker too; an escape inside the text is text.
    sq::tui::InputDecoder d;
    std::vector<Event> out;
    d.feed( "\x1b[200~a\x1b[A", out );
    d.feed( "b\x1b[20", out );
    CHECK( out.empty() );
    d.feed( "1~q", out );
    CHECK( out.size() == 2 );
    CHECK( out[0] == Event{ Paste{ "a\x1b[Ab" } } );
    CHECK( out[1] == Event{ ch( U'q' ) } );

    Mouse press{ 4, 2, MouseButton::left, MouseAction::press, {} };
    CHECK( one( t.type( "\x1b[<0;5;3M" ), Event{ press } ) );
    Mouse release{ 4, 2, MouseButton::left, MouseAction::release, {} };
    CHECK( one( t.type( "\x1b[<0;5;3m" ), Event{ release } ) );
    Mouse drag{ 9, 0, MouseButton::right, MouseAction::move, { .control = true } };
    CHECK( one( t.type( "\x1b[<50;10;1M" ), Event{ drag } ) );
    Mouse wheel{ 0, 0, MouseButton::wheel_down, MouseAction::press, {} };
    CHECK( one( t.type( "\x1b[<65;1;1M" ), Event{ wheel } ) );
    CHECK( one( t.type( "\x1b[I" ), Event{ sq::tui::Focus{ true } } ) );
    CHECK( one( t.type( "\x1b[O" ), Event{ sq::tui::Focus{ false } } ) );
}

/** A Terminal on a pty's terminal end: raw while open, put back after. */
void terminal_in_a_pty()
{
    const int master = ::posix_openpt( O_RDWR | O_NOCTTY );
    CHECK( master >= 0 );
    CHECK( ::grantpt( master ) == 0 && ::unlockpt( master ) == 0 );
    const char *name = ::ptsname( master );
    CHECK( name != nullptr );
    const int slave = ::open( name, O_RDWR | O_NOCTTY );
    CHECK( slave >= 0 );
    termios before{};
    CHECK( ::tcgetattr( slave, &before ) == 0 );
    CHECK( ( before.c_lflag & ECHO ) != 0 );

    {
        auto opened = sq::tui::Terminal::open( { .input = slave, .output = slave } );
        CHECK( opened.has_value() );
        sq::tui::Terminal &terminal = *opened;
        termios raw{};
        CHECK( ::tcgetattr( slave, &raw ) == 0 );
        CHECK( ( raw.c_lflag & ( ECHO | ICANON | ISIG ) ) == 0 );
        // A second at the same time is refused.
        CHECK( !sq::tui::Terminal::open( { .input = slave, .output = slave } ).has_value() );

        CHECK( ::write( master, "q\x1b[A", 4 ) == 4 );
        const auto q = terminal.next( std::chrono::milliseconds{ 1000 } );
        CHECK( q && *q == Event{ ch( U'q' ) } );
        const auto up = terminal.next( std::chrono::milliseconds{ 1000 } );
        CHECK( up && *up == Event{ named( KeyName::up ) } );
        // A lone escape becomes Escape once nothing follows it.
        CHECK( ::write( master, "\x1b", 1 ) == 1 );
        const auto escape = terminal.next( std::chrono::milliseconds{ 1000 } );
        CHECK( escape && *escape == Event{ named( KeyName::escape ) } );
        CHECK( !terminal.next( std::chrono::milliseconds{ 10 } ) );

        sq::tui::Screen screen( terminal.columns(), terminal.rows() );
        screen.put( 0, 0, "hi" );
        CHECK( terminal.show( screen ) );
    }

    termios after{};
    CHECK( ::tcgetattr( slave, &after ) == 0 );
    CHECK( ( after.c_lflag & ( ECHO | ICANON ) ) == ( before.c_lflag & ( ECHO | ICANON ) ) );
    // What reached the terminal: the alternate screen on, the frame, then off.
    std::string seen;
    ::fcntl( master, F_SETFL, O_NONBLOCK );
    std::array<char, 4096> buffer{};
    for( ssize_t n; ( n = ::read( master, buffer.data(), buffer.size() ) ) > 0; ) {
        seen.append( buffer.data(), static_cast<std::size_t>( n ) );
    }
    const auto on = seen.find( "\x1b[?1049h" );
    const auto frame = seen.find( "hi" );
    const auto off = seen.find( "\x1b[?1049l" );
    CHECK( on != std::string::npos && frame != std::string::npos && off != std::string::npos );
    CHECK( on < frame && frame < off );
    // And a new one may open now.
    auto again = sq::tui::Terminal::open( { .input = slave, .output = slave } );
    CHECK( again.has_value() );
    ::close( slave );
    ::close( master );
}

/** Simple on a pty: a key read, a frame shown, the terminal put back. */
void simple_in_a_pty()
{
    const int master = ::posix_openpt( O_RDWR | O_NOCTTY );
    CHECK( master >= 0 && ::grantpt( master ) == 0 && ::unlockpt( master ) == 0 );
    const int slave = ::open( ::ptsname( master ), O_RDWR | O_NOCTTY );
    CHECK( slave >= 0 );
    {
        sq::tui::Simple screen( { .input = slave, .output = slave } );
        CHECK( screen.width() > 0 && screen.height() > 0 );
        screen.put( -1, 0, "off the screen" );
        screen.put( 1, 0, "@" );
        screen.show();
        CHECK( screen.screen().cell( 1, 0 ).code == U'@' );
        CHECK( !screen.key( std::chrono::milliseconds{ 10 } ) );
        CHECK( ::write( master, "l", 1 ) == 1 );
        CHECK( screen.key() == U'l' );
    }
    termios after{};
    CHECK( ::tcgetattr( slave, &after ) == 0 );
    CHECK( ( after.c_lflag & ECHO ) != 0 );
    ::close( slave );
    ::close( master );
}

} // namespace

int main()
{
    screen_and_views();
    renders_what_a_person_sees();
    decodes_keys();
    inverse_of_key_bytes();
    inverse_of_paste_text();
    decodes_across_reads();
    decodes_paste_mouse_focus();
    terminal_in_a_pty();
    simple_in_a_pty();
    std::puts( "tui_test: ok" );
    return 0;
}
