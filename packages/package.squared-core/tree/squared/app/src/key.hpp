#pragma once

#include <cstdint>

namespace sq::app
{

/**
 * @brief Portable key names for key events.
 *
 * The one key vocabulary in squared: the platform layer produces it in an
 * Event, and the GUI routes it to the focused widget unchanged. A key squared
 * does not name arrives as `unknown`, and nothing routes it.
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
    escape
};

} // namespace sq::app
