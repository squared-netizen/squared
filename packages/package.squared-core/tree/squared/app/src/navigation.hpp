#pragma once

#include <cstdint>

namespace sq::app
{

/**
 * @brief Semantic focus and navigation action, from any supported device.
 *
 * A D-pad, a gamepad and a keyboard's arrow and Tab keys all produce these.
 * The GUI routes one to the focused widget first, then moves focus with it.
 */
enum class Navigation : std::uint8_t {
    unknown,
    left,
    right,
    up,
    down,
    next,
    previous,
    activate,
    cancel
};

} // namespace sq::app
