#pragma once

#include <cstdint>
#include <string>

namespace squared::application {

/** @brief Platform-neutral modifier keys carried by keyboard events. */
enum class KeyModifier : std::uint8_t {
    shift = 1U << 0U,
    control = 1U << 1U,
    alt = 1U << 2U,
    meta = 1U << 3U
};

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

/**
 * @brief Platform-neutral application event.
 *
 * The SDL platform adapter converts supported SDL events into this compact
 * representation before developer application code receives them.
 */
struct Event final {
    /** @brief Portable hardware-independent key names for key events. */
    enum class Key {
        unknown, left, right, up, down, home, end, backspace, delete_key,
        enter, space, tab, escape
    };
    /** @brief Semantic focus/navigation action from any supported device. */
    enum class Navigation {
        unknown, left, right, up, down, next, previous, activate, cancel
    };
    /** @brief Supported event categories for the Phase 5 boundary. */
    enum class Type {
        QuitRequested,
        PointerDown,
        PointerMove,
        PointerUp,
        BackRequested,
        Pause,
        Resume,
        Resize,
        KeyDown,
        KeyUp,
        NavigationInput,
        TextInput,
        TextEditing,
        TextInputShown,
        TextInputHidden
    };

    /** @brief Event category for the platform-neutral boundary. */
    Type type{Type::QuitRequested};

    /**
     * @brief Stable identifier for one pointer.
     *
     * The same value is carried by PointerDown, PointerMove, and PointerUp
     * events for one physical or logical touch. Values are platform-stable
     * for the lifetime of the contact and are never reused during it.
     */
    std::int64_t pointer_id{0};

    /** @brief Pointer horizontal position in logical pixels. */
    float x{0.0F};
    /** @brief Pointer vertical position in logical pixels. */
    float y{0.0F};
    /** @brief Drawable width in logical pixels for Resize events. */
    int width{0};
    /** @brief Drawable height in logical pixels for Resize events. */
    int height{0};
    /** @brief Portable key for KeyDown and KeyUp events. */
    Key key{Key::unknown};
    /** @brief Modifier state captured with a key or navigation event. */
    KeyModifiers modifiers;
    /** @brief Semantic action for NavigationInput events. */
    Navigation navigation{Navigation::unknown};
    /** @brief Stable source-device identifier for navigation events. */
    std::int32_t input_device_id{0};
    /** @brief Whether a KeyDown event is an OS auto-repeat. */
    bool repeat{false};
    /** @brief UTF-8 text for TextInput or TextEditing events. */
    std::string text;
    /** @brief Start offset of the pre-edit span in UTF-16 code units. */
    int editing_start{0};
    /** @brief Length of the pre-edit span in UTF-16 code units. */
    int editing_length{0};
};

}  // namespace squared::application
