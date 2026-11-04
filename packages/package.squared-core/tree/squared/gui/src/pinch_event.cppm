export module sq.gui:src.pinch_event;

import std;

export namespace sq::gui
{

/** @brief Where a two-finger pinch is: its start, a change, or its end. */
enum class PinchPhase : std::uint8_t { begin, change, end };

/**
 * @brief A two-finger pinch, offered to the widget under its middle
 * (`Widget::pinch`). The Stage recognises it; a widget that takes it at
 * `begin` gets its `change`s and its `end`, and the UI is not zoomed.
 */
struct PinchEvent
{
        /** @brief Which part of the pinch this is. */
        PinchPhase phase{ PinchPhase::begin };

        /**
     * @brief The fingers' distance over what it was when they started to
     * move apart or together: 1 at `begin`, 2 with them twice as far apart.
     * `end` carries the last change's.
     */
        float scale{ 1.0F };

        /** @brief The middle of the two fingers, in widget-local logical units. */
        float x{ 0.0F };

        /** @brief The middle of the two fingers, in widget-local logical units. */
        float y{ 0.0F };
};

} // namespace sq::gui
