#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

namespace sq::messaging
{

/**
 * @brief What a telegram's value is, in four bits.
 *
 * | Type | Held | Written |
 * |---|---|---|
 * | `none` | nothing | (nothing) |
 * | `boolean` | in the telegram | `true`, `false` |
 * | `integer` | in the telegram, 64-bit signed | `42`, `-7` |
 * | `number` | in the telegram, a double | `1.5`, `2.0`, `1e+21`, `inf` |
 * | `vec2`, `vec3`, `vec4` | in the telegram, floats | `(1.5 2 0 1)` |
 * | `text` | in the data area | `"notes.txt"` |
 * | `uids` | in the data area, 32-bit each | `[17 4 22]` |
 */
enum class ValueType : std::uint8_t {
    none = 0,
    boolean = 1,
    integer = 2,
    number = 3,
    vec2 = 4,
    vec3 = 5,
    vec4 = 6,
    text = 7,
    uids = 8,
};

/** @brief How many floats a `vec2`, `vec3` or `vec4` holds; 0 for any other type. */
[[nodiscard]] constexpr std::size_t float_count( ValueType type ) noexcept
{
    switch( type ) {
        case ValueType::vec2:
            return 2;
        case ValueType::vec3:
            return 3;
        case ValueType::vec4:
            return 4;
        default:
            return 0;
    }
}

/**
 * @brief The C++ type each value type is spelled as in a `Key`, and the
 * value type a C++ type means.
 *
 * `void`, `bool`, `std::int64_t`, `double`, `std::array<float, 2..4>`,
 * `std::string_view` and `std::span<const std::uint32_t>`; nothing else.
 */
template<typename T>
inline constexpr bool is_value_type =
    std::is_same_v<T, void> || std::is_same_v<T, bool> || std::is_same_v<T, std::int64_t> ||
    std::is_same_v<T, double> || std::is_same_v<T, std::array<float, 2>> ||
    std::is_same_v<T, std::array<float, 3>> || std::is_same_v<T, std::array<float, 4>> ||
    std::is_same_v<T, std::string_view> || std::is_same_v<T, std::span<const std::uint32_t>>;

/** @brief The value type a `Key`'s C++ type means. */
template<typename T>
    requires is_value_type<T>
[[nodiscard]] consteval ValueType value_type_of() noexcept
{
    if constexpr( std::is_same_v<T, void> ) {
        return ValueType::none;
    } else if constexpr( std::is_same_v<T, bool> ) {
        return ValueType::boolean;
    } else if constexpr( std::is_same_v<T, std::int64_t> ) {
        return ValueType::integer;
    } else if constexpr( std::is_same_v<T, double> ) {
        return ValueType::number;
    } else if constexpr( std::is_same_v<T, std::array<float, 2>> ) {
        return ValueType::vec2;
    } else if constexpr( std::is_same_v<T, std::array<float, 3>> ) {
        return ValueType::vec3;
    } else if constexpr( std::is_same_v<T, std::array<float, 4>> ) {
        return ValueType::vec4;
    } else if constexpr( std::is_same_v<T, std::string_view> ) {
        return ValueType::text;
    } else {
        return ValueType::uids;
    }
}

} // namespace sq::messaging
