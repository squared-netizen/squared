// What keys send (12.3d): every named key, plain and with each modifier, in
// both cursor-key modes, against the bytes xterm sends; text with Ctrl and
// Alt; what sends nothing.

#include <stdio.h>
#include <stdlib.h>

import sq.app;
import sq.term;

import std;

namespace
{

#define CHECK( ... )                                                                               \
    do {                                                                                           \
        if( !( __VA_ARGS__ ) ) {                                                                   \
            std::fprintf( stderr, "key_test.cpp:%d: %s\n", __LINE__, #__VA_ARGS__ );               \
            std::abort();                                                                          \
        }                                                                                          \
    } while( false )

using sq::app::Key;
using sq::app::KeyModifiers;
using sq::term::key_bytes;
using sq::term::key_text_bytes;
using sq::term::TerminalModes;

constexpr TerminalModes normal{};
constexpr TerminalModes application{ .application_cursor_keys = true };
constexpr KeyModifiers none{};
constexpr KeyModifiers shift{ .shift = true };
constexpr KeyModifiers alt{ .alt = true };
constexpr KeyModifiers ctrl{ .control = true };
constexpr KeyModifiers ctrl_shift{ .shift = true, .control = true };

bool sends( Key key, KeyModifiers modifiers, TerminalModes modes, std::string_view expected )
{
    const sq::term::KeyBytes got = key_bytes( key, modifiers, modes );
    if( got.view() != expected ) {
        std::fprintf( stderr, "key %d: %zu bytes, not %zu\n", static_cast<int>( key ),
                      got.view().size(), expected.size() );
        return false;
    }
    return true;
}

void cursor_keys()
{
    CHECK( sends( Key::up, none, normal, "\x1b[A" ) );
    CHECK( sends( Key::down, none, normal, "\x1b[B" ) );
    CHECK( sends( Key::right, none, normal, "\x1b[C" ) );
    CHECK( sends( Key::left, none, normal, "\x1b[D" ) );
    CHECK( sends( Key::home, none, normal, "\x1b[H" ) );
    CHECK( sends( Key::end, none, normal, "\x1b[F" ) );
    // DECCKM: SS3, as vim and less ask for.
    CHECK( sends( Key::up, none, application, "\x1bOA" ) );
    CHECK( sends( Key::left, none, application, "\x1bOD" ) );
    CHECK( sends( Key::home, none, application, "\x1bOH" ) );
    // Modifiers: CSI 1 ; m, in either mode.
    CHECK( sends( Key::up, shift, normal, "\x1b[1;2A" ) );
    CHECK( sends( Key::up, alt, normal, "\x1b[1;3A" ) );
    CHECK( sends( Key::right, ctrl, application, "\x1b[1;5C" ) );
    CHECK( sends( Key::left, ctrl_shift, normal, "\x1b[1;6D" ) );
    CHECK( sends( Key::end, KeyModifiers{ .meta = true }, normal, "\x1b[1;9F" ) );
}

void editing_keys()
{
    CHECK( sends( Key::insert, none, normal, "\x1b[2~" ) );
    CHECK( sends( Key::delete_key, none, normal, "\x1b[3~" ) );
    CHECK( sends( Key::page_up, none, normal, "\x1b[5~" ) );
    CHECK( sends( Key::page_down, none, application, "\x1b[6~" ) ); // not affected by DECCKM
    CHECK( sends( Key::page_up, shift, normal, "\x1b[5;2~" ) );
    CHECK( sends( Key::delete_key, ctrl, normal, "\x1b[3;5~" ) );
}

void function_keys()
{
    CHECK( sends( Key::f1, none, normal, "\x1bOP" ) );
    CHECK( sends( Key::f2, none, normal, "\x1bOQ" ) );
    CHECK( sends( Key::f3, none, normal, "\x1bOR" ) );
    CHECK( sends( Key::f4, none, normal, "\x1bOS" ) );
    CHECK( sends( Key::f1, shift, normal, "\x1b[1;2P" ) );
    CHECK( sends( Key::f5, none, normal, "\x1b[15~" ) );
    CHECK( sends( Key::f6, none, normal, "\x1b[17~" ) );
    CHECK( sends( Key::f7, none, normal, "\x1b[18~" ) );
    CHECK( sends( Key::f8, none, normal, "\x1b[19~" ) );
    CHECK( sends( Key::f9, none, normal, "\x1b[20~" ) );
    CHECK( sends( Key::f10, none, normal, "\x1b[21~" ) );
    CHECK( sends( Key::f11, none, normal, "\x1b[23~" ) );
    CHECK( sends( Key::f12, none, normal, "\x1b[24~" ) );
    CHECK( sends( Key::f12, ctrl, normal, "\x1b[24;5~" ) );
}

void control_keys()
{
    CHECK( sends( Key::backspace, none, normal, "\x7f" ) );
    CHECK( sends( Key::backspace, ctrl, normal, "\x08" ) );
    CHECK( sends( Key::backspace, alt, normal, "\x1b\x7f" ) );
    CHECK( sends( Key::enter, none, normal, "\r" ) );
    CHECK( sends( Key::enter, alt, normal, "\x1b\r" ) );
    CHECK( sends( Key::tab, none, normal, "\t" ) );
    CHECK( sends( Key::tab, shift, normal, "\x1b[Z" ) );
    CHECK( sends( Key::escape, none, normal, "\x1b" ) );
    CHECK( sends( Key::escape, alt, normal, "\x1b\x1b" ) );
    // A space is text; unknown keys send nothing.
    CHECK( key_bytes( Key::space, none, normal ).empty() );
    CHECK( key_bytes( Key::unknown, ctrl, normal ).empty() );
}

void text_with_modifiers()
{
    CHECK( key_text_bytes( "c", ctrl ).view() == "\x03" ); // Ctrl-C
    CHECK( key_text_bytes( "C", ctrl_shift ).view() == "\x03" );
    CHECK( key_text_bytes( "d", ctrl ).view() == "\x04" );
    CHECK( key_text_bytes( "z", ctrl ).view() == "\x1a" );
    CHECK( key_text_bytes( "[", ctrl ).view() == "\x1b" );
    CHECK( key_text_bytes( "\\", ctrl ).view() == "\x1c" );
    CHECK( key_text_bytes( "]", ctrl ).view() == "\x1d" );
    CHECK( key_text_bytes( "^", ctrl ).view() == "\x1e" );
    CHECK( key_text_bytes( "_", ctrl ).view() == "\x1f" );
    CHECK( key_text_bytes( "@", ctrl ).view() == std::string_view( "\0", 1 ) );
    CHECK( key_text_bytes( " ", ctrl ).view() == std::string_view( "\0", 1 ) );
    CHECK( key_text_bytes( "2", ctrl ).view() == std::string_view( "\0", 1 ) );
    CHECK( key_text_bytes( "3", ctrl ).view() == "\x1b" );
    CHECK( key_text_bytes( "7", ctrl ).view() == "\x1f" );
    CHECK( key_text_bytes( "8", ctrl ).view() == "\x7f" );
    CHECK( key_text_bytes( "?", ctrl ).view() == "\x7f" );
    CHECK( key_text_bytes( "/", ctrl ).view() == "\x1f" );
    // No control character: the text as it is.
    CHECK( key_text_bytes( "1", ctrl ).view() == "1" );
    CHECK( key_text_bytes( "\xc3\xa9", ctrl ).view() == "\xc3\xa9" );
    // Alt: ESC before, with Ctrl as well.
    CHECK( key_text_bytes( "b", alt ).view() == "\x1b"
                                                "b" );
    CHECK( key_text_bytes( "x", KeyModifiers{ .control = true, .alt = true } ).view() ==
           "\x1b\x18" );
    CHECK( key_text_bytes( "\xc3\xa9", alt ).view() == "\x1b\xc3\xa9" );
    // Nothing for nothing, and for text past what fits.
    CHECK( key_text_bytes( "", ctrl ).empty() );
    CHECK( key_text_bytes( "fifteen bytes!!", alt ).empty() );
    CHECK( key_text_bytes( "fourteen bytes", alt ).view().size() == 15 );
    static_assert( sizeof( sq::term::KeyBytes ) == 16 );
}

} // namespace

int main()
{
    cursor_keys();
    editing_keys();
    function_keys();
    control_keys();
    text_with_modifiers();
    std::printf( "key_test: ok\n" );
    return 0;
}
