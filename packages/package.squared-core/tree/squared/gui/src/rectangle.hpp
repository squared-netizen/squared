#pragma once

namespace sq::gui {

/** @brief Axis-aligned rectangle in logical units. */
struct Rectangle {
    /** @brief Left edge in logical units. */
    float x{0.0F};
    /** @brief Top edge in logical units. */
    float y{0.0F};
    /** @brief Width in logical units; non-negative. */
    float width{0.0F};
    /** @brief Height in logical units; non-negative. */
    float height{0.0F};
};

} // namespace sq::gui
