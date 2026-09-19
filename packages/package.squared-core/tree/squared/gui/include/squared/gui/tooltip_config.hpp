#pragma once

namespace sq::gui {

/** @brief Timing and placement policy for transient Ui tooltips. */
struct TooltipConfig final {
    /** @brief Stationary pointer delay in seconds. */
    double hover_delay{0.5};
    /** @brief Primary-contact hold delay in seconds. */
    double long_press_delay{0.6};
    /** @brief Keyboard/controller focus delay in seconds. */
    double focus_delay{0.5};
    /** @brief Pointer travel cancelling a long press, in logical units. */
    float movement_tolerance{8.0F};
    /** @brief Minimum tooltip distance from viewport edges, in logical units. */
    float viewport_margin{8.0F};
    /** @brief Gap between focused owner and tooltip, in logical units. */
    float owner_gap{8.0F};
    /** @brief Offset from a hover/press pointer, in logical units. */
    float pointer_offset{14.0F};
};

} // namespace sq::gui
