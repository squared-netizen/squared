#pragma once

namespace sq::gui {

/** @brief Insets from the edges of a rectangle, in logical units. */
struct Insets {
    /** @brief Left inset from the left edge. */
    float left{0.0F};
    /** @brief Top inset from the top edge. */
    float top{0.0F};
    /** @brief Right inset from the right edge. */
    float right{0.0F};
    /** @brief Bottom inset from the bottom edge. */
    float bottom{0.0F};
};

} // namespace sq::gui
