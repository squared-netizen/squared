module sq.tui;

import sq.term;
import std;

namespace sq::tui
{

namespace
{

constexpr std::uint8_t esc = 0x1B;
constexpr std::size_t longest_sequence = 64; // longer, and it is not one of ours
constexpr std::string_view paste_end = "\x1b[201~";

/** xterm's modifier parameter: 1 + shift 1, alt 2, control 4. */
Modifiers modifiers_of( unsigned parameter ) noexcept
{
    const unsigned bits = parameter > 0 ? parameter - 1 : 0;
    return { .shift = ( bits & 1U ) != 0,
             .alt = ( bits & 2U ) != 0,
             .control = ( bits & 4U ) != 0 };
}

Key named( KeyName code, Modifiers modifiers = {} ) noexcept
{
    return { code, 0, modifiers };
}

/** A control byte (below 0x20, or DEL) as the key it is. */
Key control_key( std::uint8_t byte ) noexcept
{
    switch( byte ) {
        case 0x0D:
        case 0x0A:
            return named( KeyName::enter );
        case 0x09:
            return named( KeyName::tab );
        case 0x7F:
            return named( KeyName::backspace );
        case 0x08:
            return named( KeyName::backspace,
                          { .control = true } ); // as key_bytes sends Ctrl+Backspace
        case 0x00:
            return { KeyName::unknown, U' ', { .control = true } };
        default:
            break;
    }
    if( byte >= 0x01 && byte <= 0x1A ) {
        return { KeyName::unknown, static_cast<char32_t>( U'a' + byte - 1 ), { .control = true } };
    }
    // 0x1C..0x1F: Ctrl with \ ] ^ _
    return { KeyName::unknown, static_cast<char32_t>( byte + 0x40 ), { .control = true } };
}

/** The key a `CSI n ~` names. */
std::optional<KeyName> tilde_key( unsigned n ) noexcept
{
    switch( n ) {
        case 1:
        case 7:
            return KeyName::home;
        case 2:
            return KeyName::insert;
        case 3:
            return KeyName::delete_key;
        case 4:
        case 8:
            return KeyName::end;
        case 5:
            return KeyName::page_up;
        case 6:
            return KeyName::page_down;
        case 11:
            return KeyName::f1;
        case 12:
            return KeyName::f2;
        case 13:
            return KeyName::f3;
        case 14:
            return KeyName::f4;
        case 15:
            return KeyName::f5;
        case 17:
            return KeyName::f6;
        case 18:
            return KeyName::f7;
        case 19:
            return KeyName::f8;
        case 20:
            return KeyName::f9;
        case 21:
            return KeyName::f10;
        case 23:
            return KeyName::f11;
        case 24:
            return KeyName::f12;
        default:
            return std::nullopt;
    }
}

/** The key a final byte names, after CSI or SS3 (arrows, Home, End, F1-F4). */
std::optional<KeyName> letter_key( std::uint8_t final_byte ) noexcept
{
    switch( final_byte ) {
        case 'A':
            return KeyName::up;
        case 'B':
            return KeyName::down;
        case 'C':
            return KeyName::right;
        case 'D':
            return KeyName::left;
        case 'H':
            return KeyName::home;
        case 'F':
            return KeyName::end;
        case 'P':
            return KeyName::f1;
        case 'Q':
            return KeyName::f2;
        case 'R':
            return KeyName::f3;
        case 'S':
            return KeyName::f4;
        default:
            return std::nullopt;
    }
}

} // namespace

void InputDecoder::feed( std::span<const std::byte> bytes, std::vector<Event> &out )
{
    for( const std::byte b : bytes ) {
        pending_.push_back( static_cast<std::uint8_t>( b ) );
    }
    std::size_t used = 0;
    for( ;; ) {
        if( in_paste_ ) {
            pending_.erase( pending_.begin(),
                            pending_.begin() + static_cast<std::ptrdiff_t>( used ) );
            used = 0;
            if( paste_bytes( out ) == 0 ) {
                return;
            }
            continue;
        }
        if( used >= pending_.size() ) {
            break;
        }
        const std::size_t n = decode( std::span{ pending_ }.subspan( used ), out );
        if( n == 0 ) {
            break;
        }
        used += n;
    }
    pending_.erase( pending_.begin(), pending_.begin() + static_cast<std::ptrdiff_t>( used ) );
}

void InputDecoder::flush( std::vector<Event> &out )
{
    if( in_paste_ || pending_.empty() ) {
        return;
    }
    // A lone escape, or an escape and a sequence never finished: Escape,
    // and the rest decoded again on its own.
    if( pending_.size() == 2 && pending_[0] == esc && pending_[1] == esc ) {
        out.emplace_back( named( KeyName::escape, { .alt = true } ) );
        pending_.clear();
        return;
    }
    if( pending_.front() == esc ) {
        out.emplace_back( named( KeyName::escape ) );
        pending_.erase( pending_.begin() );
        std::vector<std::uint8_t> rest = std::move( pending_ );
        pending_.clear();
        feed( std::as_bytes( std::span{ rest } ), out );
        if( !pending_.empty() && pending_.front() == esc ) {
            flush( out );
        }
        return;
    }
    pending_.clear(); // half a UTF-8 character: nothing to say
}

std::size_t InputDecoder::paste_bytes( std::vector<Event> &out )
{
    const std::string_view held{ reinterpret_cast<const char *>( pending_.data() ),
                                 pending_.size() };
    const std::size_t end = held.find( paste_end );
    if( end == std::string_view::npos ) {
        // Keep what could be the start of the end marker; the rest is text.
        std::size_t keep = 0;
        for( std::size_t k = std::min( held.size(), paste_end.size() - 1 ); k > 0; --k ) {
            if( held.ends_with( paste_end.substr( 0, k ) ) ) {
                keep = k;
                break;
            }
        }
        paste_.append( held.substr( 0, held.size() - keep ) );
        pending_.erase( pending_.begin(), pending_.end() - static_cast<std::ptrdiff_t>( keep ) );
        return 0;
    }
    paste_.append( held.substr( 0, end ) );
    out.emplace_back( Paste{ std::exchange( paste_, {} ) } );
    in_paste_ = false;
    pending_.erase( pending_.begin(),
                    pending_.begin() + static_cast<std::ptrdiff_t>( end + paste_end.size() ) );
    return 1;
}

std::size_t InputDecoder::decode( std::span<const std::uint8_t> bytes, std::vector<Event> &out )
{
    const std::uint8_t first = bytes[0];
    if( first == esc ) {
        if( bytes.size() < 2 ) {
            return 0; // a lone escape, or the start of something: flush decides
        }
        const std::uint8_t second = bytes[1];
        if( second == '[' ) {
            return decode_csi( bytes, out );
        }
        if( second == 'O' ) {
            if( bytes.size() < 3 ) {
                return 0;
            }
            if( const auto key = letter_key( bytes[2] ) ) {
                out.emplace_back( named( *key ) );
            }
            return 3;
        }
        if( second == esc ) {
            // Alt+Escape (ESC ESC, as key_bytes sends it), or Alt and a
            // sequence (ESC ESC [ A): which, the third byte says.
            if( bytes.size() < 3 ) {
                return 0;
            }
            if( bytes[2] != '[' && bytes[2] != 'O' ) {
                out.emplace_back( named( KeyName::escape, { .alt = true } ) );
                return 2;
            }
        }
        // Alt and a key: the key, decoded on its own, with alt.
        std::vector<Event> one;
        const std::size_t n = decode( bytes.subspan( 1 ), one );
        if( n == 0 ) {
            return 0;
        }
        for( Event &event : one ) {
            if( Key *key = std::get_if<Key>( &event ) ) {
                key->modifiers.alt = true;
            }
            out.push_back( std::move( event ) );
        }
        return n + 1;
    }
    if( first < 0x20 || first == 0x7F ) {
        out.emplace_back( control_key( first ) );
        return 1;
    }
    sq::term::detail::Utf8Decoder utf8;
    using Step = sq::term::detail::Utf8Decoder::Step;
    for( std::size_t i = 0; i < bytes.size(); ++i ) {
        const auto result = utf8.push( bytes[i] );
        if( result.step == Step::code ) {
            out.emplace_back( Key{ KeyName::unknown, result.code, {} } );
            return i + 1;
        }
        if( result.step == Step::code_then_again ) {
            out.emplace_back( Key{ KeyName::unknown, result.code, {} } );
            return i; // this byte starts again
        }
    }
    return 0; // the rest of the character is still coming
}

std::size_t InputDecoder::decode_csi( std::span<const std::uint8_t> bytes, std::vector<Event> &out )
{
    // ESC [ then parameter bytes (0x30-0x3F), intermediates (0x20-0x2F), a final byte (0x40-0x7E).
    std::size_t end = 2;
    while( end < bytes.size() && ( bytes[end] < 0x40 || bytes[end] > 0x7E ) ) {
        if( bytes[end] < 0x20 || end >= longest_sequence ) {
            return end; // not a sequence we know: dropped
        }
        ++end;
    }
    if( end >= bytes.size() ) {
        return 0;
    }
    const std::uint8_t final_byte = bytes[end];
    const std::size_t used = end + 1;
    const bool sgr_mouse = end > 2 && bytes[2] == '<';

    // The numbers, separated by ';' (':' sub-parameters ignored).
    std::array<unsigned, 4> number{};
    std::size_t count = 0;
    bool any = false;
    bool sub = false;
    for( std::size_t i = sgr_mouse ? 3 : 2; i < end; ++i ) {
        const std::uint8_t b = bytes[i];
        if( b >= '0' && b <= '9' ) {
            if( !sub && count < number.size() ) {
                number[count] = std::min( number[count] * 10U + ( b - '0' ), 100000U );
                any = true;
            }
        } else if( b == ';' ) {
            sub = false;
            if( count < number.size() ) {
                ++count;
            }
        } else if( b == ':' ) {
            sub = true;
        } else {
            return used; // private markers we do not read
        }
    }
    if( any || count > 0 ) {
        count = std::min( count + 1, number.size() );
    }

    if( sgr_mouse ) {
        if( count < 3 || ( final_byte != 'M' && final_byte != 'm' ) ) {
            return used;
        }
        const unsigned b = number[0];
        Mouse mouse;
        mouse.x =
            static_cast<std::uint16_t>( number[1] > 0 ? std::min( number[1] - 1, 0xFFFFU ) : 0 );
        mouse.y =
            static_cast<std::uint16_t>( number[2] > 0 ? std::min( number[2] - 1, 0xFFFFU ) : 0 );
        mouse.modifiers = { .shift = ( b & 4U ) != 0,
                            .alt = ( b & 8U ) != 0,
                            .control = ( b & 16U ) != 0 };
        if( ( b & 64U ) != 0 ) {
            mouse.button = ( b & 1U ) != 0 ? MouseButton::wheel_down : MouseButton::wheel_up;
            mouse.action = MouseAction::press;
        } else {
            constexpr std::array<MouseButton, 4> buttons{ MouseButton::left, MouseButton::middle,
                                                          MouseButton::right, MouseButton::none };
            mouse.button = buttons[b & 3U];
            mouse.action = ( b & 32U ) != 0    ? MouseAction::move
                           : final_byte == 'm' ? MouseAction::release
                                               : MouseAction::press;
        }
        out.emplace_back( mouse );
        return used;
    }

    const Modifiers modifiers = count >= 2 ? modifiers_of( number[1] ) : Modifiers{};
    if( final_byte == '~' ) {
        if( number[0] == 200 ) {
            in_paste_ = true;
            paste_.clear();
            return used;
        }
        if( const auto key = tilde_key( number[0] ) ) {
            out.emplace_back( named( *key, modifiers ) );
        }
        return used;
    }
    if( final_byte == 'Z' ) {
        out.emplace_back( named( KeyName::tab, { .shift = true } ) );
        return used;
    }
    if( final_byte == 'I' && count == 0 ) {
        out.emplace_back( Focus{ true } );
        return used;
    }
    if( final_byte == 'O' && count == 0 ) {
        out.emplace_back( Focus{ false } );
        return used;
    }
    if( const auto key = letter_key( final_byte ) ) {
        out.emplace_back( named( *key, modifiers ) );
    }
    return used;
}

} // namespace sq::tui
