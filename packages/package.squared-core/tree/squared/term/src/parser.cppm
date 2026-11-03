export module sq.term:src.parser;

export import :src.csi_sequence;
export import :src.utf8_decoder;
import std;

export namespace sq::term::detail
{

/**
 * What the parser calls: a printable character, a control function (a C0
 * byte, or a C1 code U+0080..U+009F), an escape sequence (`ESC`, up to two
 * intermediates, a final), a control sequence, an operating system command's
 * text. The terminal is the one handler; a test records the calls.
 */
template<typename H>
concept ParserHandler = requires( H &handler, char32_t code, std::uint8_t control,
                                  const CsiSequence &csi, std::string_view text ) {
    handler.print( code );
    handler.execute( control );
    handler.escape( csi );
    handler.csi( csi );
    handler.osc( text );
};

/**
 * ECMA-48's syntax, from a byte stream: the state machine of Paul Williams'
 * "A parser for DEC's ANSI-compatible video terminals" (vt100.net), on
 * UTF-8. Bytes arrive as the program wrote them, in reads of any size; a
 * sequence or a character split between two reads is put together. It owns
 * no memory: the sequence being read and up to 256 bytes of an operating
 * system command are its own members (about 360 bytes in all), so feeding it
 * never allocates.
 *
 * The bytes 0x80..0xFF are UTF-8, never 8-bit controls (a C1 control comes
 * as its code point, U+009B for CSI). A byte of 0x80 or more inside a
 * sequence ends it, unread, and is read again as text. An operating system
 * command longer than 256 bytes is dropped whole. DCS, SOS, PM and APC
 * strings are read and ignored.
 */
class Parser
{
    public:
        static constexpr std::size_t max_osc = 256;

        template<ParserHandler H>
        void feed( std::span<const std::byte> bytes, H &handler ) noexcept
        {
            const std::size_t size = bytes.size();
            std::size_t i = 0;
            while( i < size ) {
                // Most of a terminal's input is printable ASCII: straight
                // through, with no state to look at.
                if( state_ == State::ground && !utf8_.pending() ) {
                    while( i < size ) {
                        const auto byte = static_cast<std::uint8_t>( bytes[i] );
                        if( byte < 0x20 || byte > 0x7E ) {
                            break;
                        }
                        handler.print( byte );
                        ++i;
                    }
                    if( i == size ) {
                        break;
                    }
                }
                take( static_cast<std::uint8_t>( bytes[i] ), handler );
                ++i;
            }
        }

        /** Back to text, anything half-read forgotten. */
        void reset() noexcept
        {
            state_ = State::ground;
            utf8_.reset();
        }

    private:
        enum class State : std::uint8_t {
            ground,
            escape,
            escape_intermediate,
            csi_entry,
            csi_param,
            csi_intermediate,
            csi_ignore,
            osc_string,
            ignore_string,
        };

        static constexpr std::uint8_t esc = 0x1B;
        static constexpr std::uint8_t bel = 0x07;
        static constexpr std::uint8_t can = 0x18;
        static constexpr std::uint8_t sub = 0x1A;
        static constexpr std::uint8_t del = 0x7F;

        template<ParserHandler H>
        void take( std::uint8_t byte, H &handler ) noexcept
        {
            switch( state_ ) {
                case State::osc_string:
                    osc_byte( byte, handler );
                    return;
                case State::ignore_string:
                    if( byte == esc ) {
                        enter_escape();
                    } else if( byte == can || byte == sub ) {
                        state_ = State::ground;
                    }
                    return;
                case State::ground:
                    ground_byte( byte, handler );
                    return;
                default:
                    break;
            }
            // In a sequence.
            if( byte >= 0x80 ) {
                state_ = State::ground;
                ground_byte( byte, handler );
                return;
            }
            if( byte < 0x20 || byte == del ) {
                control( byte, handler );
                return;
            }
            switch( state_ ) {
                case State::escape:
                    escape_byte( byte, handler );
                    return;
                case State::escape_intermediate:
                    if( byte < 0x30 ) {
                        collect( byte );
                    } else {
                        dispatch_escape( byte, handler );
                    }
                    return;
                case State::csi_entry:
                    if( byte >= 0x3C && byte <= 0x3F ) {
                        csi_.private_marker = static_cast<char>( byte );
                        state_ = State::csi_param;
                        return;
                    }
                    state_ = State::csi_param;
                    csi_param_byte( byte, handler );
                    return;
                case State::csi_param:
                    csi_param_byte( byte, handler );
                    return;
                case State::csi_intermediate:
                    if( byte < 0x30 ) {
                        collect( byte );
                    } else if( byte < 0x40 ) {
                        state_ = State::csi_ignore;
                    } else {
                        dispatch_csi( byte, handler );
                    }
                    return;
                case State::csi_ignore:
                    if( byte >= 0x40 ) {
                        state_ = State::ground;
                    }
                    return;
                default:
                    return;
            }
        }

        template<ParserHandler H>
        void ground_byte( std::uint8_t byte, H &handler ) noexcept
        {
            if( byte < 0x80 && !utf8_.pending() ) {
                if( byte < 0x20 || byte == del ) {
                    control( byte, handler );
                } else {
                    handler.print( byte );
                }
                return;
            }
            const Utf8Decoder::Result result = utf8_.push( byte );
            if( result.step == Utf8Decoder::Step::more ) {
                return;
            }
            code_point( result.code, handler );
            if( result.step == Utf8Decoder::Step::code_then_again ) {
                take( byte, handler );
            }
        }

        template<ParserHandler H>
        void code_point( char32_t code, H &handler ) noexcept
        {
            if( code < 0x80 || code > 0x9F ) {
                handler.print( code );
                return;
            }
            // A C1 control, as its code point.
            switch( code ) {
                case 0x9B:
                    enter_csi();
                    return;
                case 0x9D:
                    enter_osc();
                    return;
                case 0x90:
                case 0x98:
                case 0x9E:
                case 0x9F:
                    state_ = State::ignore_string;
                    return;
                case 0x9C:
                    return;
                default:
                    handler.execute( static_cast<std::uint8_t>( code ) );
                    return;
            }
        }

        // A C0 control in text or inside a sequence: executed where it
        // stands (ECMA-48 allows it), except ESC, CAN and SUB, which end the
        // sequence.
        template<ParserHandler H>
        void control( std::uint8_t byte, H &handler ) noexcept
        {
            if( byte == esc ) {
                enter_escape();
            } else if( byte == can || byte == sub ) {
                state_ = State::ground;
            } else if( byte != del ) {
                handler.execute( byte );
            }
        }

        template<ParserHandler H>
        void escape_byte( std::uint8_t byte, H &handler ) noexcept
        {
            if( byte < 0x30 ) {
                collect( byte );
                state_ = State::escape_intermediate;
                return;
            }
            switch( byte ) {
                case '[':
                    enter_csi();
                    return;
                case ']':
                    enter_osc();
                    return;
                case 'P':
                case 'X':
                case '^':
                case '_':
                    state_ = State::ignore_string;
                    return;
                default:
                    dispatch_escape( byte, handler );
                    return;
            }
        }

        template<ParserHandler H>
        void csi_param_byte( std::uint8_t byte, H &handler ) noexcept
        {
            if( byte >= '0' && byte <= '9' ) {
                const std::uint32_t value = value_ * 10U + ( byte - std::uint32_t{ '0' } );
                value_ = value > 0xFFFFU ? 0xFFFFU : value;
                in_parameter_ = true;
            } else if( byte == ';' || byte == ':' ) {
                end_parameter();
                next_is_sub_ = byte == ':';
                in_parameter_ = true;
            } else if( byte < 0x30 ) {
                if( in_parameter_ ) {
                    end_parameter();
                }
                collect( byte );
                state_ = State::csi_intermediate;
            } else if( byte < 0x40 ) {
                state_ = State::csi_ignore;
            } else {
                if( in_parameter_ ) {
                    end_parameter();
                }
                dispatch_csi( byte, handler );
            }
        }

        template<ParserHandler H>
        void osc_byte( std::uint8_t byte, H &handler ) noexcept
        {
            if( byte == bel || byte == esc ) {
                if( !osc_overflow_ ) {
                    handler.osc( std::string_view{ osc_.data(), osc_size_ } );
                }
                if( byte == esc ) {
                    enter_escape();
                } else {
                    state_ = State::ground;
                }
            } else if( byte == can || byte == sub ) {
                state_ = State::ground;
            } else if( byte >= 0x20 ) {
                if( osc_size_ < max_osc ) {
                    osc_[osc_size_++] = static_cast<char>( byte );
                } else {
                    osc_overflow_ = true;
                }
            }
        }

        void enter_escape() noexcept
        {
            state_ = State::escape;
            csi_.intermediate_count = 0;
            intermediate_overflow_ = false;
        }

        void enter_csi() noexcept
        {
            state_ = State::csi_entry;
            csi_.count = 0;
            csi_.sub = 0;
            csi_.private_marker = 0;
            csi_.intermediate_count = 0;
            value_ = 0;
            in_parameter_ = false;
            next_is_sub_ = false;
            intermediate_overflow_ = false;
        }

        void enter_osc() noexcept
        {
            state_ = State::osc_string;
            osc_size_ = 0;
            osc_overflow_ = false;
        }

        void collect( std::uint8_t byte ) noexcept
        {
            if( csi_.intermediate_count < csi_.intermediates.size() ) {
                csi_.intermediates[csi_.intermediate_count++] = static_cast<char>( byte );
            } else {
                intermediate_overflow_ = true;
            }
        }

        void end_parameter() noexcept
        {
            if( csi_.count < CsiSequence::max_parameters ) {
                csi_.parameters[csi_.count] = static_cast<std::uint16_t>( value_ );
                if( next_is_sub_ ) {
                    csi_.sub |= std::uint32_t{ 1 } << csi_.count;
                }
                ++csi_.count;
            }
            value_ = 0;
            next_is_sub_ = false;
        }

        template<ParserHandler H>
        void dispatch_escape( std::uint8_t final, H &handler ) noexcept
        {
            state_ = State::ground;
            if( intermediate_overflow_ ) {
                return;
            }
            csi_.final = static_cast<char>( final );
            csi_.count = 0;
            csi_.private_marker = 0;
            handler.escape( csi_ );
        }

        template<ParserHandler H>
        void dispatch_csi( std::uint8_t final, H &handler ) noexcept
        {
            state_ = State::ground;
            if( intermediate_overflow_ ) {
                return;
            }
            csi_.final = static_cast<char>( final );
            handler.csi( csi_ );
        }

        CsiSequence csi_{};
        std::array<char, max_osc> osc_{};
        std::uint32_t value_ = 0;
        std::uint16_t osc_size_ = 0;
        Utf8Decoder utf8_{};
        State state_ = State::ground;
        bool in_parameter_ = false;
        bool next_is_sub_ = false;
        bool intermediate_overflow_ = false;
        bool osc_overflow_ = false;
};

} // namespace sq::term::detail
