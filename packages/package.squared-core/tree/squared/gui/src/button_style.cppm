export module sq.gui:src.button_style;

export import :src.drawable_ptr;
export import :src.font_ptr;
import sq.graphics;

export namespace sq::gui
{

/** @brief Style data for a Button widget. */
struct ButtonStyle
{
        /** @brief Drawable shown in the normal state. */
        DrawablePtr normal;
        /** @brief Drawable shown while the pointer hovers. */
        DrawablePtr hovered;
        /** @brief Drawable shown while the button is pressed. */
        DrawablePtr pressed;
        /** @brief Drawable shown when the button is disabled. */
        DrawablePtr disabled;
        /** @brief Normal label color. */
        graphics::Color text{ graphics::Color::from_rgba8( 238, 241, 247 ) };
        /** @brief Label color when the button is disabled. */
        graphics::Color disabled_text{ graphics::Color::from_rgba8( 174, 181, 194 ) };
        /** @brief Minimum button height in logical units. */
        float minimum_height{ 44.0F };
        /** @brief Horizontal label padding in logical units. */
        float horizontal_padding{ 12.0F };
        /** @brief Label font, or empty to use the painter default. */
        FontPtr font;
        /** @brief Square drawable/glyph slot size in logical units. */
        float icon_size{ 20.0F };
        /** @brief Gap between an icon and non-empty label in logical units. */
        float icon_spacing{ 8.0F };
};

} // namespace sq::gui
