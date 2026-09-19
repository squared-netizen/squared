#pragma once

#include <squared/gui/drawable_ptr.hpp>

namespace sq::gui {

/** @brief Style data for a Slider widget. */
struct SliderStyle {
    /** @brief Drawable for the unfilled track portion. */
    DrawablePtr track;
    /** @brief Drawable for the filled track portion. */
    DrawablePtr filled_track;
    /** @brief Drawable for the slider knob. */
    DrawablePtr knob;
    /** @brief Minimum track length in logical units. */
    float minimum_length{120.0F};
    /** @brief Minimum touch target size in logical units. */
    float minimum_touch_size{44.0F};
};

} // namespace sq::gui
