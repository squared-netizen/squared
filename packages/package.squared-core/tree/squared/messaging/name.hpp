#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sq::messaging
{

/**
 * @brief 64-bit FNV-1a of a name's text, continuing from `seed`.
 *
 * The id a Name carries. Continuing from a seed lets a name be hashed in
 * pieces, as `item[17].name` is: "item", then ".name", with nothing copied.
 */
[[nodiscard]] constexpr std::uint64_t
name_hash( std::string_view text, std::uint64_t seed = 14695981039346656037ULL ) noexcept
{
    std::uint64_t hash = seed;
    for( const char c : text ) {
        hash ^= static_cast<unsigned char>( c );
        hash *= 1099511628211ULL;
    }
    return hash;
}

/**
 * @brief Whether `text` is a name: dot-separated segments of `a`-`z`, `0`-`9`
 * and `_`, none empty, at most 255 bytes in all (`player.hp`, `item.name`).
 *
 * One spelling per name, so one id per name: no capitals, no spaces. Which
 * object a telegram is about never goes in the name; that is the uid.
 */
[[nodiscard]] constexpr bool valid_name( std::string_view text ) noexcept
{
    if( text.empty() || text.size() > 255 || text.front() == '.' || text.back() == '.' ) {
        return false;
    }
    char previous = '\0';
    for( const char c : text ) {
        const bool word = ( c >= 'a' && c <= 'z' ) || ( c >= '0' && c <= '9' ) || c == '_';
        if( !word && ( c != '.' || previous == '.' ) ) {
            return false;
        }
        previous = c;
    }
    return true;
}

namespace detail
{
/// Not constexpr: a literal that is not a valid name calls it, and the
/// consteval constructor below stops compiling.
void not_a_valid_name() noexcept;
} // namespace detail

/**
 * @brief The schema part of an address (`player.hp`), as its 64-bit hash.
 *
 * Eight bytes, compared and copied as one integer. A literal is hashed at
 * compile time and checked there: `Name n = "player.hp";` costs nothing at
 * run time, and `Name n = "Player HP";` does not compile. A name made at run
 * time is hashed once with `Name::of`, and checked when it is registered.
 */
struct Name
{
        std::uint64_t id = 0;

        /** @brief No name (id 0). */
        constexpr Name() noexcept = default;

        /** @brief A literal, hashed and checked at compile time. */
        template<std::size_t Length>
        // NOLINTNEXTLINE(google-explicit-constructor): a literal is a name, as in "player.hp"
        consteval Name( const char ( &text )[Length] ) noexcept
            : id( name_hash( std::string_view( text, Length - 1 ) ) )
        {
            if( !valid_name( std::string_view( text, Length - 1 ) ) ) {
                detail::not_a_valid_name();
            }
        }

        /** @brief A name made at run time: hashed, not checked (NameTable::add checks). */
        [[nodiscard]] static constexpr Name of( std::string_view text ) noexcept
        {
            Name name;
            name.id = name_hash( text );
            return name;
        }

        /** @brief A name from its id, as stored or sent. */
        [[nodiscard]] static constexpr Name from_id( std::uint64_t id ) noexcept
        {
            Name name;
            name.id = id;
            return name;
        }

        friend constexpr bool operator==( Name, Name ) noexcept = default;
        friend constexpr auto operator<=>( Name, Name ) noexcept = default;
};

static_assert( sizeof( Name ) == 8 );

} // namespace sq::messaging
