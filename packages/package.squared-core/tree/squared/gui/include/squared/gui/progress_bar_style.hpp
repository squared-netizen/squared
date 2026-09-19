#pragma once

#include <squared/gui/drawable_ptr.hpp>

namespace sq::gui {

/** @brief Style data for a non-interactive ProgressBar widget. */
struct ProgressBarStyle {
    /** @brief Drawable for the complete progress track. */
    DrawablePtr track;
    /** @brief Drawable for the completed portion of the track. */
    DrawablePtr fill;
    /** @brief Preferred horizontal length in logical units. */
    float minimum_length{120.0F};
    /** @brief Preferred track thickness in logical units. */
    float thickness{12.0F};
};

} // namespace sq::gui
