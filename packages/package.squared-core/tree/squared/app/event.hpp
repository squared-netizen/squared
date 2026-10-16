#pragma once

#include <squared/app/src/key.hpp>
#include <squared/app/src/key_modifiers.hpp>
#include <squared/app/src/navigation.hpp>

#include <cstdint>
#include <string>

namespace sq::app
{

/**
 * @brief Platform-neutral application event.
 *
 * The platform layer converts what the system reports into this compact
 * representation before application code receives it. Keys, modifiers and
 * navigation use the same types the GUI routes, so nothing converts them
 * again on the way to a widget.
 */
struct Event final
{
        /** @brief Supported event categories for the Phase 5 boundary. */
        enum class Type : std::uint8_t {
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
        Type type{ Type::QuitRequested };

        /**
     * @brief Stable identifier for one pointer.
     *
     * The same value is carried by PointerDown, PointerMove, and PointerUp
     * events for one physical or logical touch. Values are platform-stable
     * for the lifetime of the contact and are never reused during it.
     */
        std::int64_t pointer_id{ 0 };

        /** @brief Pointer horizontal position in logical pixels. */
        float x{ 0.0F };
        /** @brief Pointer vertical position in logical pixels. */
        float y{ 0.0F };
        /** @brief Drawable width in logical pixels for Resize events. */
        int width{ 0 };
        /** @brief Drawable height in logical pixels for Resize events. */
        int height{ 0 };
        /** @brief Portable key for KeyDown and KeyUp events. */
        Key key{ Key::unknown };
        /** @brief Modifier state captured with a key or navigation event. */
        KeyModifiers modifiers;
        /** @brief Semantic action for NavigationInput events. */
        Navigation navigation{ Navigation::unknown };
        /** @brief Stable source-device identifier for navigation events. */
        std::int32_t input_device_id{ 0 };
        /** @brief Whether a KeyDown event is an OS auto-repeat. */
        bool repeat{ false };
        /** @brief UTF-8 text for TextInput or TextEditing events. */
        std::string text;
        /** @brief Start offset of the pre-edit span in UTF-16 code units. */
        int editing_start{ 0 };
        /** @brief Length of the pre-edit span in UTF-16 code units. */
        int editing_length{ 0 };
};

} // namespace sq::app
