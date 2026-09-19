#pragma once

#include <squared/graphics/color.hpp>
#include <squared/gui/drawable_ptr.hpp>
#include <squared/gui/font_ptr.hpp>
#include <squared/gui/insets.hpp>

namespace sq::gui {

/** @brief Style data for a Window widget. */
struct WindowStyle {
    /** @brief Drawable for the window body. */
    DrawablePtr background;
    /** @brief Drawable for the title bar. */
    DrawablePtr title_background;
    /** @brief Drawable for the close button in the normal state. */
    DrawablePtr close_normal;
    /** @brief Drawable for the close button while hovered. */
    DrawablePtr close_hovered;
    /** @brief Drawable for the close button while pressed. */
    DrawablePtr close_pressed;
    /** @brief Title text color. */
    graphics::Color title_text{graphics::Color::from_rgba8(238, 241, 247)};
    /** @brief Close-button glyph color. */
    graphics::Color close_text{graphics::Color::from_rgba8(238, 241, 247)};
    /** @brief Overlay color used for modal windows. */
    graphics::Color modal_overlay{graphics::Color::from_rgba8(0, 0, 0, 140)};
    /** @brief Insets between the window border and its content. */
    Insets content_insets{8.0F, 8.0F, 8.0F, 8.0F};
    /** @brief Title bar height in logical units. */
    float title_height{36.0F};
    /** @brief Close-button size in logical units. */
    float close_size{28.0F};
    /** @brief Resize grab border width in logical units. */
    float resize_border{8.0F};
    /** @brief Title and close-glyph font, or empty for the painter default. */
    FontPtr title_font;
};

} // namespace sq::gui
