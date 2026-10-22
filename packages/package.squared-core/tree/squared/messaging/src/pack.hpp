#pragma once

#include <squared/messaging/address.hpp>
#include <squared/messaging/src/value_type.hpp>
#include <squared/messaging/src/verb.hpp>
#include <squared/messaging/telegram.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

namespace sq::messaging::detail
{

/// A telegram and its text or uid list, from a key's value.
struct Packed
{
        Telegram telegram;
        std::span<const std::byte> data;
};

template<typename T, typename V>
[[nodiscard]] Packed pack( Verb verb, Address to, const V &value ) noexcept
{
    if constexpr( std::is_same_v<T, bool> ) {
        return { Telegram::boolean( verb, to, value ), {} };
    } else if constexpr( std::is_same_v<T, std::int64_t> ) {
        return { Telegram::integer( verb, to, static_cast<std::int64_t>( value ) ), {} };
    } else if constexpr( std::is_same_v<T, double> ) {
        return { Telegram::number( verb, to, static_cast<double>( value ) ), {} };
    } else if constexpr( std::is_same_v<T, std::string_view> ) {
        const std::string_view text( value );
        const auto bytes = std::as_bytes( std::span<const char>( text.data(), text.size() ) );
        return { Telegram::data( verb, to, ValueType::text, {} ), bytes };
    } else if constexpr( std::is_same_v<T, std::span<const std::uint32_t>> ) {
        const std::span<const std::uint32_t> uids( value );
        return { Telegram::data( verb, to, ValueType::uids, {} ), std::as_bytes( uids ) };
    } else {
        const T floats( value ); // std::array<float, 2..4>
        return { Telegram::floats( verb, to, std::span<const float>( floats ) ), {} };
    }
}

} // namespace sq::messaging::detail
