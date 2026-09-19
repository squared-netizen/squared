#pragma once

#include <squared/app/key_modifier.hpp>

#include <cstdint>

namespace sq::app {

/** @brief Compact value object containing zero or more modifier keys. */
class KeyModifiers final {
public:
    /** @brief Construct an empty modifier set. */
    constexpr KeyModifiers() noexcept = default;

    /**
     * @brief Construct a set containing one modifier.
     * @param modifier Shift, Control, Alt, or Meta.
     */
    constexpr KeyModifiers(KeyModifier modifier) noexcept
        : bits_(static_cast<std::uint8_t>(modifier))
    {
    }

    /**
     * @brief Test whether the given modifier is set.
     * @param modifier Shift, Control, Alt, or Meta.
     * @return true when the modifier is present.
     */
    [[nodiscard]] constexpr bool contains(KeyModifier modifier) const noexcept
    {
        return (bits_ & static_cast<std::uint8_t>(modifier)) != 0;
    }

    /**
     * @brief Set or clear one modifier.
     * @param modifier Shift, Control, Alt, or Meta.
     * @param enabled `true` sets the modifier; `false` clears it.
     */
    constexpr void set(KeyModifier modifier, bool enabled = true) noexcept
    {
        const auto bit = static_cast<std::uint8_t>(modifier);
        bits_ = enabled ? static_cast<std::uint8_t>(bits_ | bit)
                        : static_cast<std::uint8_t>(bits_ & ~bit);
    }

    /**
     * @brief Check whether the set holds any modifier.
     * @return true when no modifier is set.
     */
    [[nodiscard]] constexpr bool empty() const noexcept { return bits_ == 0; }

    /**
     * @brief Read the raw bit field.
     * @return Bit field with one bit per enabled modifier.
     */
    [[nodiscard]] constexpr std::uint8_t bits() const noexcept { return bits_; }

private:
    std::uint8_t bits_{0};
};

[[nodiscard]] constexpr KeyModifiers operator|(
    KeyModifier left,
    KeyModifier right
) noexcept
{
    KeyModifiers result(left);
    result.set(right);
    return result;
}

}  // namespace sq::app
