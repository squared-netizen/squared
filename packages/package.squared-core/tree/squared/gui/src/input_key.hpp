#pragma once

#include <cstdint>

namespace sq::gui
{

/** @brief Portable hardware-independent key names for key events. */
enum class InputKey : std::uint8_t {
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

} // namespace sq::gui
