#pragma once

#include <squared/graphics/color.hpp>
#include <squared/gui/drawable_ptr.hpp>
#include <squared/gui/font_ptr.hpp>

namespace sq::gui {

/** @brief Style data for a CheckBox widget. */
struct CheckBoxStyle {
    /** @brief Drawable for the unchecked box. */
    DrawablePtr unchecked;
    /** @brief Drawable for the checked box. */
    DrawablePtr checked;
    /** @brief Drawable shown when the box is disabled. */
    DrawablePtr disabled;
    /** @brief Label color. */
    graphics::Color text{graphics::Color::from_rgba8(238, 241, 247)};
    /** @brief Gap between the box and its label in logical units. */
    float spacing{8.0F};
    /** @brief Minimum touch target size in logical units. */
    float minimum_touch_size{44.0F};
    /** @brief Label font, or empty to use the painter default. */
    FontPtr font;
};

} // namespace sq::gui
