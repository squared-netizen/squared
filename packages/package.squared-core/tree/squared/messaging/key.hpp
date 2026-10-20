#pragma once

#include <squared/messaging/name.hpp>
#include <squared/messaging/src/value_type.hpp>
#include <squared/messaging/src/verb.hpp>

#include <concepts>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace sq::messaging
{

/// The third argument of a `Key` whose address is about one object of many.
inline constexpr bool with_uid = true;

/**
 * @brief An address in code: its name, the type of its value, the verbs it
 * carries, and whether it takes a uid.
 *
 * ```cpp
 * inline constexpr sq::Key<std::int64_t, sq::state>          ticks{ "counter.ticks" };
 * inline constexpr sq::Key<bool, sq::state | sq::set>        sound{ "settings.sound" };
 * inline constexpr sq::Key<std::string_view, sq::state, sq::with_uid> item_name{ "item.name" };
 * ```
 *
 * The only way to name an address in an app. Written by hand or generated
 * from `names.json`, it is the same type. The type, the verbs and the uid
 * flag exist for the compiler: sending the wrong type, a verb the address
 * does not carry, or no uid where one is needed does not compile. What is
 * kept is the name (8 bytes) and a pointer to its text (8 bytes), for the
 * name table; a telegram carries only the name's id.
 *
 * @tparam T One of the value types (`is_value_type`); `void` for none.
 */
template<typename T, Verbs V, bool Uid = false>
    requires is_value_type<T>
class Key final
{
    public:
        using value_type = T;
        static constexpr Verbs verbs = V;
        static constexpr bool takes_uid = Uid;
        static constexpr ValueType type = value_type_of<T>();

        static_assert( V.bits != 0, "a key carries at least one verb" );

        /** @brief From a literal: hashed and checked at compile time, like `Name`. */
        template<std::size_t Length>
        // NOLINTNEXTLINE(google-explicit-constructor): a literal is a key, as in "counter.ticks"
        consteval Key( const char ( &text )[Length] ) noexcept
            : name_( text )
            , text_( text )
        {}

        /** @brief The name, as a telegram carries it. */
        [[nodiscard]] constexpr Name name() const noexcept { return name_; }

        /** @brief The name's text, for the name table. */
        [[nodiscard]] constexpr std::string_view text() const noexcept { return text_; }

    private:
        Name name_;
        const char *text_;
};

/** @brief Whether `K` is a `Key`. */
template<typename K>
inline constexpr bool is_key = false;
template<typename T, Verbs V, bool Uid>
inline constexpr bool is_key<Key<T, V, Uid>> = true;

namespace detail
{
/// Whether a value of type `From` may be sent for a key of value type `To`:
/// exactly the type, or for numbers any integer (never a bool), or for text
/// anything that is a string_view.
template<typename To, typename From>
inline constexpr bool sends_as = [] {
    using F = std::remove_cvref_t<From>;
    if constexpr( std::is_same_v<To, bool> ) {
        return std::is_same_v<F, bool>;
    } else if constexpr( std::is_same_v<To, std::int64_t> ) {
        return std::is_integral_v<F> && !std::is_same_v<F, bool>;
    } else if constexpr( std::is_same_v<To, double> ) {
        return std::is_arithmetic_v<F> && !std::is_same_v<F, bool>;
    } else if constexpr( std::is_same_v<To, std::string_view> ) {
        return std::is_convertible_v<F, std::string_view>;
    } else if constexpr( std::is_same_v<To, void> ) {
        return false;
    } else {
        return std::is_convertible_v<F, To>;
    }
}();
} // namespace detail

/**
 * @brief Whether `verb` may be sent to `K` with a value of type `Value`
 * (`void` for none). The Telegraph's calls are constrained by it, so a
 * mistake is a compile error, not a run-time one.
 */
template<typename K, Verb verb, typename Value>
concept sendable = is_key<std::remove_cvref_t<K>> && std::remove_cvref_t<K>::verbs.has( verb ) &&
                   ( std::is_same_v<Value, void>
                         ? std::is_same_v<typename std::remove_cvref_t<K>::value_type, void>
                         : detail::sends_as<typename std::remove_cvref_t<K>::value_type, Value> );

} // namespace sq::messaging
