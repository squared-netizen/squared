module sq.term;

import sq.app;
import std;

namespace sq::term
{

namespace
{

constexpr char esc = '\x1b';

void put( KeyBytes &out, std::string_view text ) noexcept
{
    const std::size_t room = out.bytes.size() - out.size;
    const std::size_t n = text.size() < room ? text.size() : room;
    std::copy_n( text.data(), n, out.bytes.begin() + out.size );
    out.size = static_cast<std::uint8_t>( out.size + n );
}

void put( KeyBytes &out, char c ) noexcept
{
    put( out, std::string_view( &c, 1 ) );
}

void put_number( KeyBytes &out, unsigned n ) noexcept
{
    std::array<char, 4> digits{};
    const auto made = std::to_chars( digits.data(), digits.data() + digits.size(), n );
    put( out, std::string_view( digits.data(), made.ptr ) );
}

/// xterm's modifier parameter: 1 + Shift 1 + Alt 2 + Ctrl 4 + Meta 8; 1 is none.
unsigned parameter_of( app::KeyModifiers m ) noexcept
{
    return 1U + ( m.shift ? 1U : 0U ) + ( m.alt ? 2U : 0U ) + ( m.control ? 4U : 0U ) +
           ( m.meta ? 8U : 0U );
}

/// A key ending in a letter: `CSI A`, `SS3 A` (application mode, or F1 to F4
/// unmodified), `CSI 1 ; m A` with modifiers.
KeyBytes letter_key( char final, unsigned parameter, bool ss3 ) noexcept
{
    KeyBytes out;
    put( out, esc );
    if( parameter != 1 ) {
        put( out, "[1;" );
        put_number( out, parameter );
    } else {
        put( out, ss3 ? 'O' : '[' );
    }
    put( out, final );
    return out;
}

/// A key ending in a tilde: `CSI n ~`, `CSI n ; m ~` with modifiers.
KeyBytes tilde_key( unsigned number, unsigned parameter ) noexcept
{
    KeyBytes out;
    put( out, "\x1b[" );
    put_number( out, number );
    if( parameter != 1 ) {
        put( out, ';' );
        put_number( out, parameter );
    }
    put( out, '~' );
    return out;
}

/// One byte, ESC before it with Alt.
KeyBytes byte_key( char c, bool alt ) noexcept
{
    KeyBytes out;
    if( alt ) {
        put( out, esc );
    }
    put( out, c );
    return out;
}

/// What Ctrl makes of an ASCII character; -1 for none.
int control_of( char c ) noexcept
{
    if( c >= 'a' && c <= 'z' ) {
        return c - 'a' + 1;
    }
    if( c >= '@' && c <= '_' ) { // capitals and @ [ \ ] ^ _
        return c - '@';
    }
    switch( c ) {
        case ' ':
        case '2':
            return 0;
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
            return 0x1b + ( c - '3' );
        case '8':
        case '?':
            return 0x7f;
        case '/':
            return 0x1f;
        default:
            return -1;
    }
}

} // namespace

KeyBytes key_bytes( app::Key key, app::KeyModifiers modifiers, TerminalModes modes ) noexcept
{
    using app::Key;
    const unsigned parameter = parameter_of( modifiers );
    const bool app_keys = modes.application_cursor_keys;
    switch( key ) {
        case Key::up:
            return letter_key( 'A', parameter, app_keys );
        case Key::down:
            return letter_key( 'B', parameter, app_keys );
        case Key::right:
            return letter_key( 'C', parameter, app_keys );
        case Key::left:
            return letter_key( 'D', parameter, app_keys );
        case Key::home:
            return letter_key( 'H', parameter, app_keys );
        case Key::end:
            return letter_key( 'F', parameter, app_keys );
        case Key::insert:
            return tilde_key( 2, parameter );
        case Key::delete_key:
            return tilde_key( 3, parameter );
        case Key::page_up:
            return tilde_key( 5, parameter );
        case Key::page_down:
            return tilde_key( 6, parameter );
        case Key::f1:
            return letter_key( 'P', parameter, true );
        case Key::f2:
            return letter_key( 'Q', parameter, true );
        case Key::f3:
            return letter_key( 'R', parameter, true );
        case Key::f4:
            return letter_key( 'S', parameter, true );
        case Key::f5:
            return tilde_key( 15, parameter );
        case Key::f6:
            return tilde_key( 17, parameter );
        case Key::f7:
            return tilde_key( 18, parameter );
        case Key::f8:
            return tilde_key( 19, parameter );
        case Key::f9:
            return tilde_key( 20, parameter );
        case Key::f10:
            return tilde_key( 21, parameter );
        case Key::f11:
            return tilde_key( 23, parameter );
        case Key::f12:
            return tilde_key( 24, parameter );
        case Key::backspace:
            return byte_key( modifiers.control ? '\x08' : '\x7f', modifiers.alt );
        case Key::enter:
            return byte_key( '\r', modifiers.alt );
        case Key::tab:
            if( modifiers.shift ) {
                KeyBytes out;
                put( out, "\x1b[Z" );
                return out;
            }
            return byte_key( '\t', modifiers.alt );
        case Key::escape:
            return byte_key( esc, modifiers.alt );
        case Key::space:
        case Key::unknown:
            return {};
    }
    return {};
}

KeyBytes key_text_bytes( std::string_view text, app::KeyModifiers modifiers ) noexcept
{
    KeyBytes out;
    if( text.empty() || text.size() > out.bytes.size() - 1 ) {
        return out;
    }
    if( modifiers.alt ) {
        put( out, esc );
    }
    const int control = modifiers.control && text.size() == 1 ? control_of( text[0] ) : -1;
    if( control >= 0 ) {
        put( out, static_cast<char>( control ) );
    } else {
        put( out, text );
    }
    return out;
}

} // namespace sq::term
