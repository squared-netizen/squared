#pragma once

#include <cstdint>

namespace sq::gui
{

/** @brief Event kind carried by one InputEvent. */
enum class InputType : std::uint8_t {
    pointer_move,
    pointer_down,
    pointer_up,
    pointer_cancel,
    key_down,
    key_up,
    navigation
};

} // namespace sq::gui
