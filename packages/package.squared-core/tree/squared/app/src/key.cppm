export module sq.app:src.key;

import std;

export namespace sq::app
{

/**
 * @brief Portable key names for key events.
 *
 * The one key vocabulary in squared: the platform layer produces it in an
 * Event, and the GUI routes it to the focused widget unchanged. A key squared
 * does not name arrives as `unknown`, and nothing routes it.
 *
 * Letters and digits are not keys here: what a key types arrives as text
 * (`Event::Type::TextInput`), with Ctrl and Alt in the event's modifiers.
 * From `page_up` on (12.3d): the keys a terminal needs that a form does not.
 */
enum class Key : std::uint8_t {
    unknown,
    left,
    right,
    up,
    down,
    home,
    end,
    backspace,
    delete_key,
    enter,
    space,
    tab,
    escape,
    page_up,
    page_down,
    insert,
    f1,
    f2,
    f3,
    f4,
    f5,
    f6,
    f7,
    f8,
    f9,
    f10,
    f11,
    f12
};

} // namespace sq::app
