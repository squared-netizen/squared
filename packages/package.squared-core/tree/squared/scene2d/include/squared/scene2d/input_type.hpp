#pragma once

namespace sq::scene2d {

/** @brief Event kind carried by one InputEvent. */
enum class InputType {
    pointer_move,
    pointer_down,
    pointer_up,
    pointer_cancel,
    key_down,
    key_up,
    navigation
};

} // namespace sq::scene2d
