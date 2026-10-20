#pragma once

#include <squared/messaging/name.hpp>

#include <cstdint>
#include <optional>
#include <string_view>

namespace sq::messaging
{

/**
 * @brief Where a telegram goes: a name (the schema) and a uid (which object).
 *
 * `player.hp` is a name with no uid; `item[17].name` is the name `item.name`
 * about the object with uid 17, the model's own id for it. A uid of 0 means
 * none. Sixteen bytes; the telegram envelope packs it tighter.
 */
struct Address
{
        Name name;
        std::uint32_t uid = 0;

        friend constexpr bool operator==( const Address &, const Address & ) noexcept = default;
};

static_assert( sizeof( Address ) == 16 );

/**
 * @brief An address from its text form, with no name table: the name is hashed.
 *
 * `player.hp`, `item[17].name`, `inventory[20]`: the uid in brackets after the
 * first segment, in decimal. `#` and 16 hex digits stand for a name's id
 * written without its text, with the uid after it (`#0123456789abcdef[17]`),
 * which is how an unregistered name is written. `[0]` reads as no uid.
 * @return The address, or nothing when `text` is not one (not a valid name, a
 * uid past 4294967295, brackets anywhere else).
 */
[[nodiscard]] std::optional<Address> parse_address( std::string_view text ) noexcept;

} // namespace sq::messaging
