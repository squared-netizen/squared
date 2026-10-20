#include <squared/messaging/telegram.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>

namespace sq::messaging
{

namespace
{

constexpr unsigned verb_bits = 2;
constexpr unsigned type_mask = 0xF;

template<typename T>
void store( std::array<std::byte, 16> &into, const T &value ) noexcept
{
    static_assert( sizeof( T ) <= 16 );
    std::memcpy( into.data(), &value, sizeof( T ) );
}

template<typename T>
T load( const std::array<std::byte, 16> &from ) noexcept
{
    T value{};
    std::memcpy( &value, from.data(), sizeof( T ) );
    return value;
}

/// How many leading bytes of the value a type uses; the rest must be zero.
std::size_t used_bytes( ValueType type ) noexcept
{
    switch( type ) {
        case ValueType::none:
            return 0;
        case ValueType::boolean:
            return 1;
        case ValueType::integer:
        case ValueType::number:
        case ValueType::text:
        case ValueType::uids:
            return 8;
        case ValueType::vec2:
        case ValueType::vec3:
        case ValueType::vec4:
            return float_count( type ) * sizeof( float );
    }
    return 16;
}

} // namespace

Telegram Telegram::make( Verb verb, Address to, ValueType type ) noexcept
{
    Telegram telegram;
    telegram.name_ = to.name;
    telegram.uid_ = to.uid;
    telegram.kind_ = static_cast<std::uint16_t>( static_cast<unsigned>( verb ) |
                                                 ( static_cast<unsigned>( type ) << verb_bits ) );
    return telegram;
}

Telegram Telegram::empty( Verb verb, Address to ) noexcept
{
    return make( verb, to, ValueType::none );
}

Telegram Telegram::boolean( Verb verb, Address to, bool value ) noexcept
{
    Telegram telegram = make( verb, to, ValueType::boolean );
    telegram.value_[0] = static_cast<std::byte>( value ? 1 : 0 );
    return telegram;
}

Telegram Telegram::integer( Verb verb, Address to, std::int64_t value ) noexcept
{
    Telegram telegram = make( verb, to, ValueType::integer );
    store( telegram.value_, value );
    return telegram;
}

Telegram Telegram::number( Verb verb, Address to, double value ) noexcept
{
    assert( !std::isnan( value ) && "a NaN is not a value" );
    Telegram telegram = make( verb, to, ValueType::number );
    store( telegram.value_, value );
    return telegram;
}

Telegram Telegram::floats( Verb verb, Address to, std::span<const float> values ) noexcept
{
    assert( values.size() >= 2 && values.size() <= 4 && "two to four floats" );
    const std::size_t count = std::min<std::size_t>( std::max<std::size_t>( values.size(), 2 ), 4 );
    const auto type =
        static_cast<ValueType>( static_cast<unsigned>( ValueType::vec2 ) + count - 2 );
    Telegram telegram = make( verb, to, type );
    for( std::size_t i = 0; i < count && i < values.size(); ++i ) {
        assert( !std::isnan( values[i] ) && "a NaN is not a value" );
        std::memcpy( telegram.value_.data() + i * sizeof( float ), &values[i], sizeof( float ) );
    }
    return telegram;
}

Telegram Telegram::data( Verb verb, Address to, ValueType type, DataRef where ) noexcept
{
    assert( ( type == ValueType::text || type == ValueType::uids ) && "text or uids" );
    assert( ( type != ValueType::uids || where.length % 4 == 0 ) && "4 bytes a uid" );
    Telegram telegram = make( verb, to, type );
    store( telegram.value_, where );
    return telegram;
}

Telegram Telegram::with_correlation( std::uint16_t correlation ) const noexcept
{
    Telegram copy = *this;
    copy.correlation_ = correlation;
    return copy;
}

Verb Telegram::verb() const noexcept
{
    return static_cast<Verb>( kind_ & ( ( 1U << verb_bits ) - 1 ) );
}

ValueType Telegram::type() const noexcept
{
    return static_cast<ValueType>( ( kind_ >> verb_bits ) & type_mask );
}

bool Telegram::as_boolean() const noexcept
{
    return value_[0] != std::byte{ 0 };
}

std::int64_t Telegram::as_integer() const noexcept
{
    return load<std::int64_t>( value_ );
}

double Telegram::as_number() const noexcept
{
    return load<double>( value_ );
}

std::array<float, 4> Telegram::as_floats() const noexcept
{
    return load<std::array<float, 4>>( value_ );
}

DataRef Telegram::as_data() const noexcept
{
    return load<DataRef>( value_ );
}

bool Telegram::valid() const noexcept
{
    const auto raw_type = ( kind_ >> verb_bits ) & type_mask;
    if( ( kind_ >> ( verb_bits + 4 ) ) != 0 ||
        raw_type > static_cast<unsigned>( ValueType::uids ) ) {
        return false;
    }
    const ValueType value_type = type();
    for( std::size_t i = used_bytes( value_type ); i < value_.size(); ++i ) {
        if( value_[i] != std::byte{ 0 } ) {
            return false;
        }
    }
    switch( value_type ) {
        case ValueType::boolean:
            return value_[0] == std::byte{ 0 } || value_[0] == std::byte{ 1 };
        case ValueType::number:
            return !std::isnan( as_number() );
        case ValueType::vec2:
        case ValueType::vec3:
        case ValueType::vec4: {
            const auto values = as_floats();
            for( std::size_t i = 0; i < float_count( value_type ); ++i ) {
                if( std::isnan( values[i] ) ) {
                    return false;
                }
            }
            return true;
        }
        case ValueType::uids:
            return as_data().length % 4 == 0;
        default:
            return true;
    }
}

std::array<std::byte, 32> TelegramBytes::of( const Telegram &telegram ) noexcept
{
    std::array<std::byte, 32> bytes{};
    std::memcpy( bytes.data(), &telegram, sizeof( Telegram ) );
    return bytes;
}

Telegram TelegramBytes::from( const std::array<std::byte, 32> &bytes ) noexcept
{
    Telegram telegram;
    std::memcpy( &telegram, bytes.data(), sizeof( Telegram ) );
    return telegram;
}

} // namespace sq::messaging
