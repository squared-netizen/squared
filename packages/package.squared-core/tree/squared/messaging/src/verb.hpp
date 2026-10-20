#pragma once

#include <cstdint>

namespace sq::messaging
{

/**
 * @brief What a telegram does, in two bits.
 *
 * | Verb | Direction | Means |
 * |---|---|---|
 * | `set` | UI to model | the user changed a value |
 * | `do_` | UI to model | an intent (`do file.open "notes.txt"`) |
 * | `state` | model to UI | a value is now this |
 * | `event` | model to UI | something happened |
 */
enum class Verb : std::uint8_t {
    set = 0,
    do_ = 1, ///< `do` is a keyword; its text form is `do`
    state = 2,
    event = 3,
};

/**
 * @brief A set of verbs: the ones an address carries (`sq::state | sq::set`).
 *
 * A structural type, so it can be a template argument of `Key`.
 */
struct Verbs
{
        std::uint8_t bits = 0;

        /** @brief Whether `verb` is in the set. */
        [[nodiscard]] constexpr bool has( Verb verb ) const noexcept
        {
            return ( bits & ( 1U << static_cast<unsigned>( verb ) ) ) != 0;
        }

        friend constexpr Verbs operator|( Verbs a, Verbs b ) noexcept
        {
            return Verbs{ static_cast<std::uint8_t>( a.bits | b.bits ) };
        }
        friend constexpr bool operator==( Verbs, Verbs ) noexcept = default;
};

/// The four verbs as sets of one, to combine with `|`.
inline constexpr Verbs set{ 1U << static_cast<unsigned>( Verb::set ) };
inline constexpr Verbs do_{ 1U << static_cast<unsigned>( Verb::do_ ) };
inline constexpr Verbs state{ 1U << static_cast<unsigned>( Verb::state ) };
inline constexpr Verbs event{ 1U << static_cast<unsigned>( Verb::event ) };

} // namespace sq::messaging
