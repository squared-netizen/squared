#include <squared/messaging/telegram_text.hpp>

#include <array>
#include <charconv>
#include <cmath>
#include <cstring>
#include <system_error>

namespace sq::messaging
{

namespace
{

/// Writes into a span; once something does not fit, everything after is refused.
class Out
{
    public:
        explicit Out( std::span<char> out ) noexcept
            : out_( out )
        {}

        void put( std::string_view text ) noexcept
        {
            if( failed_ || text.size() > out_.size() - used_ ) {
                failed_ = true;
                return;
            }
            std::memcpy( out_.data() + used_, text.data(), text.size() );
            used_ += text.size();
        }

        void put( char c ) noexcept { put( std::string_view( &c, 1 ) ); }

        [[nodiscard]] std::size_t result() const noexcept { return failed_ ? 0 : used_; }

    private:
        std::span<char> out_;
        std::size_t used_ = 0;
        bool failed_ = false;
};

template<typename T>
void put_number( Out &out, T value ) noexcept
{
    std::array<char, 40> digits{};
    const auto result = std::to_chars( digits.data(), digits.data() + digits.size(), value );
    out.put(
        std::string_view( digits.data(), static_cast<std::size_t>( result.ptr - digits.data() ) ) );
}

/// A double, written so it reads back as a double and not an integer.
void put_double( Out &out, double value ) noexcept
{
    std::array<char, 40> digits{};
    const auto result = std::to_chars( digits.data(), digits.data() + digits.size(), value );
    const std::string_view text( digits.data(),
                                 static_cast<std::size_t>( result.ptr - digits.data() ) );
    out.put( text );
    if( text.find_first_of( ".ein" ) == std::string_view::npos ) {
        out.put( ".0" );
    }
}

void put_text( Out &out, std::span<const std::byte> bytes ) noexcept
{
    constexpr std::string_view hex = "0123456789abcdef";
    out.put( '"' );
    for( const std::byte b : bytes ) {
        const auto c = static_cast<unsigned char>( b );
        if( c == '\\' ) {
            out.put( "\\\\" );
        } else if( c == '"' ) {
            out.put( "\\\"" );
        } else if( c == '\n' ) {
            out.put( "\\n" );
        } else if( c == '\t' ) {
            out.put( "\\t" );
        } else if( c < 0x20 || c == 0x7F ) {
            const std::array<char, 4> escape{ '\\', 'x', hex[c >> 4], hex[c & 0xF] };
            out.put( std::string_view( escape.data(), escape.size() ) );
        } else {
            out.put( static_cast<char>( c ) );
        }
    }
    out.put( '"' );
}

// ---- reading -------------------------------------------------------------------------

/// The line, read from the front.
class In
{
    public:
        explicit In( std::string_view line ) noexcept
            : line_( line )
        {}

        void skip_spaces() noexcept
        {
            while( at_ < line_.size() && ( line_[at_] == ' ' || line_[at_] == '\t' ) ) {
                ++at_;
            }
        }

        [[nodiscard]] bool done() const noexcept { return at_ >= line_.size(); }
        [[nodiscard]] char peek() const noexcept { return done() ? '\0' : line_[at_]; }
        char take() noexcept { return done() ? '\0' : line_[at_++]; }

        /// Up to the next space or tab, or the end.
        std::string_view word() noexcept
        {
            const std::size_t start = at_;
            while( at_ < line_.size() && line_[at_] != ' ' && line_[at_] != '\t' ) {
                ++at_;
            }
            return line_.substr( start, at_ - start );
        }

        /// Up to (not including) `stop`; the stop itself is taken. Nothing when absent.
        std::optional<std::string_view> until( char stop ) noexcept
        {
            const std::size_t end = line_.find( stop, at_ );
            if( end == std::string_view::npos ) {
                return std::nullopt;
            }
            const std::string_view inside = line_.substr( at_, end - at_ );
            at_ = end + 1;
            return inside;
        }

        [[nodiscard]] std::string_view rest() const noexcept { return line_.substr( at_ ); }
        void advance( std::size_t count ) noexcept { at_ += count; }

    private:
        std::string_view line_;
        std::size_t at_ = 0;
};

template<typename T>
bool parse_whole( std::string_view text, T &value ) noexcept
{
    if( text.empty() ) {
        return false;
    }
    const auto [end, error] = std::from_chars( text.data(), text.data() + text.size(), value );
    return error == std::errc{} && end == text.data() + text.size();
}

std::optional<Verb> parse_verb( std::string_view word ) noexcept
{
    if( word == "set" ) {
        return Verb::set;
    }
    if( word == "do" ) {
        return Verb::do_;
    }
    if( word == "state" ) {
        return Verb::state;
    }
    if( word == "event" ) {
        return Verb::event;
    }
    return std::nullopt;
}

int hex_digit( char c ) noexcept
{
    if( c >= '0' && c <= '9' ) {
        return c - '0';
    }
    if( c >= 'a' && c <= 'f' ) {
        return c - 'a' + 10;
    }
    if( c >= 'A' && c <= 'F' ) {
        return c - 'A' + 10;
    }
    return -1;
}

/// A quoted text, after its opening quote: decoded into `data`. Two passes,
/// so the bytes go straight to where they stay.
std::optional<DataRef> read_text( In &in, TelegramData &data ) noexcept
{
    const std::string_view rest = in.rest();
    std::size_t length = 0;
    std::size_t i = 0;
    for( ; i < rest.size() && rest[i] != '"'; ++i ) {
        if( rest[i] == '\\' ) {
            if( i + 1 >= rest.size() ) {
                return std::nullopt;
            }
            const char e = rest[++i];
            if( e == 'x' ) {
                if( i + 2 >= rest.size() || hex_digit( rest[i + 1] ) < 0 ||
                    hex_digit( rest[i + 2] ) < 0 ) {
                    return std::nullopt;
                }
                i += 2;
            } else if( e != '\\' && e != '"' && e != 'n' && e != 't' ) {
                return std::nullopt;
            }
        }
        ++length;
    }
    if( i >= rest.size() ) {
        return std::nullopt; // no closing quote
    }
    const auto where = data.reserve( length );
    if( !where ) {
        return std::nullopt;
    }
    const std::span<std::byte> into = data.writable( *where );
    std::size_t put = 0;
    for( std::size_t j = 0; j < i; ++j ) {
        char c = rest[j];
        if( c == '\\' ) {
            const char e = rest[++j];
            if( e == 'n' ) {
                c = '\n';
            } else if( e == 't' ) {
                c = '\t';
            } else if( e == 'x' ) {
                c = static_cast<char>( hex_digit( rest[j + 1] ) * 16 + hex_digit( rest[j + 2] ) );
                j += 2;
            } else {
                c = e;
            }
        }
        into[put++] = static_cast<std::byte>( c );
    }
    in.advance( i + 1 );
    return where;
}

/// Space-separated words inside, each through `each`; false on any bad one.
template<typename Each>
bool for_each_word( std::string_view inside, const Each &each ) noexcept
{
    In words( inside );
    words.skip_spaces();
    while( !words.done() ) {
        if( !each( words.word() ) ) {
            return false;
        }
        words.skip_spaces();
    }
    return true;
}

std::optional<Telegram> read_value( Verb verb, Address to, In &in, TelegramData &data ) noexcept
{
    const char first = in.peek();
    if( first == '"' ) {
        in.take();
        const auto where = read_text( in, data );
        if( !where ) {
            return std::nullopt;
        }
        return Telegram::data( verb, to, ValueType::text, *where );
    }
    if( first == '(' ) {
        in.take();
        const auto inside = in.until( ')' );
        if( !inside ) {
            return std::nullopt;
        }
        std::array<float, 4> values{};
        std::size_t count = 0;
        const bool good = for_each_word( *inside, [&]( std::string_view word ) {
            float value = 0;
            if( count == 4 || !parse_whole( word, value ) || std::isnan( value ) ) {
                return false;
            }
            values[count++] = value;
            return true;
        } );
        if( !good || count < 2 ) {
            return std::nullopt;
        }
        return Telegram::floats( verb, to, std::span<const float>( values.data(), count ) );
    }
    if( first == '[' ) {
        in.take();
        const auto inside = in.until( ']' );
        if( !inside ) {
            return std::nullopt;
        }
        std::size_t count = 0;
        if( !for_each_word( *inside, [&]( std::string_view word ) {
                std::uint32_t uid = 0;
                ++count;
                return parse_whole( word, uid );
            } ) ) {
            return std::nullopt;
        }
        const auto where = data.reserve( count * sizeof( std::uint32_t ) );
        if( !where ) {
            return std::nullopt;
        }
        std::byte *write = data.writable( *where ).data();
        static_cast<void>( for_each_word( *inside, [&]( std::string_view word ) {
            std::uint32_t uid = 0;
            static_cast<void>( parse_whole( word, uid ) );
            std::memcpy( write, &uid, sizeof( uid ) );
            write += sizeof( uid );
            return true;
        } ) );
        return Telegram::data( verb, to, ValueType::uids, *where );
    }
    const std::string_view word = in.word();
    if( word == "true" || word == "false" ) {
        return Telegram::boolean( verb, to, word == "true" );
    }
    if( word.find_first_of( ".eEin" ) != std::string_view::npos ) {
        double value = 0;
        if( !parse_whole( word, value ) || std::isnan( value ) ) {
            return std::nullopt;
        }
        return Telegram::number( verb, to, value );
    }
    std::int64_t value = 0;
    if( !parse_whole( word, value ) ) {
        return std::nullopt;
    }
    return Telegram::integer( verb, to, value );
}

/// The value of `telegram`, as the text form writes it; false when its data is outside `data`.
bool put_value( Out &line, const Telegram &telegram, std::span<const std::byte> data ) noexcept
{
    const ValueType type = telegram.type();
    switch( type ) {
        case ValueType::none:
            break;
        case ValueType::boolean:
            line.put( telegram.as_boolean() ? "true" : "false" );
            break;
        case ValueType::integer:
            put_number( line, telegram.as_integer() );
            break;
        case ValueType::number:
            put_double( line, telegram.as_number() );
            break;
        case ValueType::vec2:
        case ValueType::vec3:
        case ValueType::vec4: {
            const auto values = telegram.as_floats();
            line.put( '(' );
            for( std::size_t i = 0; i < float_count( type ); ++i ) {
                if( i > 0 ) {
                    line.put( ' ' );
                }
                put_number( line, values[i] );
            }
            line.put( ')' );
            break;
        }
        case ValueType::text:
        case ValueType::uids: {
            const DataRef where = telegram.as_data();
            if( where.offset > data.size() || where.length > data.size() - where.offset ) {
                return false;
            }
            const auto bytes = data.subspan( where.offset, where.length );
            if( type == ValueType::text ) {
                put_text( line, bytes );
                break;
            }
            line.put( '[' );
            for( std::size_t i = 0; i < bytes.size() / 4; ++i ) {
                std::uint32_t uid = 0;
                std::memcpy( &uid, bytes.data() + i * 4, sizeof( uid ) );
                if( i > 0 ) {
                    line.put( ' ' );
                }
                put_number( line, uid );
            }
            line.put( ']' );
            break;
        }
    }
    return true;
}

} // namespace

std::string_view verb_text( Verb verb ) noexcept
{
    switch( verb ) {
        case Verb::set:
            return "set";
        case Verb::do_:
            return "do";
        case Verb::state:
            return "state";
        case Verb::event:
            return "event";
    }
    return "?";
}

std::size_t write_telegram( const Telegram &telegram, std::span<const std::byte> data,
                            const NameTable &names, std::span<char> out ) noexcept
{
    if( !telegram.valid() ) {
        return 0;
    }
    Out line( out );
    line.put( verb_text( telegram.verb() ) );
    line.put( ' ' );
    std::array<char, 300> address{};
    const std::size_t address_length = names.format( telegram.address(), address );
    line.put( std::string_view( address.data(), address_length ) );

    if( telegram.type() != ValueType::none ) {
        line.put( ' ' );
    }
    if( !put_value( line, telegram, data ) ) {
        return 0;
    }
    if( telegram.correlation() != 0 ) {
        line.put( " @" );
        put_number( line, telegram.correlation() );
    }
    return line.result();
}

std::size_t write_value( const Telegram &telegram, std::span<const std::byte> data,
                         std::span<char> out ) noexcept
{
    if( !telegram.valid() || telegram.type() == ValueType::none ) {
        return 0;
    }
    Out line( out );
    if( !put_value( line, telegram, data ) ) {
        return 0;
    }
    return line.result();
}

std::optional<Telegram> read_telegram( std::string_view line, TelegramData &data ) noexcept
{
    const std::size_t data_before = data.used();
    const auto fail = [&]() -> std::optional<Telegram> {
        data.rewind( data_before );
        return std::nullopt;
    };
    In in( line );
    in.skip_spaces();
    const auto verb = parse_verb( in.word() );
    in.skip_spaces();
    const auto address = parse_address( in.word() );
    if( !verb || !address ) {
        return fail();
    }
    in.skip_spaces();
    std::optional<Telegram> telegram;
    if( in.done() || in.peek() == '@' ) {
        telegram = Telegram::empty( *verb, *address );
    } else {
        telegram = read_value( *verb, *address, in, data );
        if( !telegram ) {
            return fail();
        }
    }
    in.skip_spaces();
    if( in.peek() == '@' ) {
        in.take();
        std::uint16_t correlation = 0;
        if( !parse_whole( in.word(), correlation ) ) {
            return fail();
        }
        telegram = telegram->with_correlation( correlation );
        in.skip_spaces();
    }
    if( !in.done() ) {
        return fail();
    }
    return telegram;
}

} // namespace sq::messaging
