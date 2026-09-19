#pragma once

#include <squared/graphics/color.hpp>
#include <squared/gui/drawable_ptr.hpp>
#include <squared/gui/font_ptr.hpp>

namespace sq::gui {

/** @brief Style data for a TextField widget. */
struct TextFieldStyle {
    /** @brief Drawable shown when unfocused. */
    DrawablePtr normal;
    /** @brief Drawable shown while focused. */
    DrawablePtr focused;
    /** @brief Text color. */
    graphics::Color text{graphics::Color::from_rgba8(238, 241, 247)};
    /** @brief Cursor color. */
    graphics::Color cursor{graphics::Color::from_rgba8(79, 137, 255)};
    /** @brief Minimum field height in logical units. */
    float minimum_height{44.0F};
    /** @brief Horizontal text padding in logical units. */
    float horizontal_padding{10.0F};
    /** @brief Text font, or empty to use the painter default. */
    FontPtr font;
};

} // namespace sq::gui
