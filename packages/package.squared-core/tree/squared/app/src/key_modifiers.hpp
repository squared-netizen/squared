#pragma once

namespace sq::app
{

/**
 * @brief The modifier keys held during a key or navigation event.
 *
 * One byte: four one-bit fields. Read them as fields (`modifiers.shift`) and
 * build with designated initializers (`KeyModifiers{ .shift = true }`).
 */
struct KeyModifiers final
{
        /** @brief Either Shift key. */
        bool shift : 1 = false;
        /** @brief Either Control key. */
        bool control : 1 = false;
        /** @brief Either Alt key. */
        bool alt : 1 = false;
        /** @brief Either Meta key (the Windows or Command key). */
        bool meta : 1 = false;

        /**
     * @brief Report whether any modifier is held.
     * @return true when at least one field is set.
     */
        [[nodiscard]] constexpr bool any() const noexcept
        {
            return shift || control || alt || meta;
        }

        /** @brief Equal when the same modifiers are held. */
        friend constexpr bool operator==( KeyModifiers, KeyModifiers ) noexcept = default;
};

static_assert( sizeof( KeyModifiers ) == 1, "KeyModifiers is one byte; see docs/developer/app.md" );

} // namespace sq::app
