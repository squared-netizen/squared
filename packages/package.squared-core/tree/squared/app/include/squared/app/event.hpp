#pragma once

#include <squared/app/key_modifiers.hpp>

#include <cstdint>
#include <string>

namespace sq::app {

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

}  // namespace sq::app
