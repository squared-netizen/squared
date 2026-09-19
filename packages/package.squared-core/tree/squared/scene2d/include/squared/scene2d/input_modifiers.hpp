#pragma once

namespace sq::scene2d {

/** @brief Backend-neutral modifier state copied at the input boundary. */
struct InputModifiers final {
    /** @brief Shift key pressed. */
    bool shift{false};
    /** @brief Control key pressed. */
    bool control{false};
    /** @brief Alt key pressed. */
    bool alt{false};
    /** @brief Meta key pressed. */
    bool meta{false};
};

} // namespace sq::scene2d
