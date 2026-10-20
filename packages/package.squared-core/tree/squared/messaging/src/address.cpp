#include <squared/messaging/address.hpp>

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <optional>
#include <string_view>
#include <system_error>

namespace sq::messaging
{

namespace
{

/// `[digits]` at the start of `text`, as a uid; `used` is how much it took.
std::optional<std::uint32_t> read_uid( std::string_view text, std::size_t &used ) noexcept
{
    used = 0;
    if( text.empty() || text.front() != '[' ) {
        return std::uint32_t{ 0 };
    }
    const std::size_t close = text.find( ']' );
    if( close == std::string_view::npos || close == 1 ) {
        return std::nullopt;
    }
    const std::string_view digits = text.substr( 1, close - 1 );
    std::uint32_t uid = 0;
    const auto [end, error] = std::from_chars( digits.data(), digits.data() + digits.size(), uid );
    if( error != std::errc{} || end != digits.data() + digits.size() ) {
        return std::nullopt;
    }
    used = close + 1;
    return uid;
}

} // namespace

std::optional<Address> parse_address( std::string_view text ) noexcept
{
    // `#` and 16 hex digits: a name's id with no text.
    if( text.starts_with( '#' ) ) {
        if( text.size() < 17 ) {
            return std::nullopt;
        }
        const std::string_view hex = text.substr( 1, 16 );
        std::uint64_t id = 0;
        const auto [end, error] = std::from_chars( hex.data(), hex.data() + hex.size(), id, 16 );
        if( error != std::errc{} || end != hex.data() + hex.size() ) {
            return std::nullopt;
        }
        std::size_t used = 0;
        const auto uid = read_uid( text.substr( 17 ), used );
        if( !uid || 17 + used != text.size() ) {
            return std::nullopt;
        }
        return Address{ Name::from_id( id ), *uid };
    }

    // `first[uid].rest`: the uid comes out, the name is first.rest, hashed in two pieces.
    const std::size_t first_end = std::min( text.find_first_of( ".[" ), text.size() );
    const std::string_view first = text.substr( 0, first_end );
    std::size_t used = 0;
    const auto uid = read_uid( text.substr( first_end ), used );
    if( !uid ) {
        return std::nullopt;
    }
    const std::string_view rest = text.substr( first_end + used );
    if( !rest.empty() && rest.front() != '.' ) {
        return std::nullopt;
    }
    // Validate the name as it will be registered: first, then rest.
    if( !valid_name( first ) || ( !rest.empty() && !valid_name( rest.substr( 1 ) ) ) ||
        first.size() + rest.size() > 255 ) {
        return std::nullopt;
    }
    return Address{ Name::from_id( name_hash( rest, name_hash( first ) ) ), *uid };
}

} // namespace sq::messaging
