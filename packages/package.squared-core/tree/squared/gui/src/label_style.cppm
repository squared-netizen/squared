export module sq.gui:src.label_style;

export import :src.font_ptr;
import sq.graphics;
import std;

export namespace sq::gui
{

/** @brief Style data for a Label widget. */
struct LabelStyle
{
        /** @brief Font resource, or empty to use the painter default. */
        FontPtr font;
        /** @brief Primary label color. */
        std::optional<graphics::Color> text;
        /** @brief Muted label color. */
        std::optional<graphics::Color> muted_text;
};

} // namespace sq::gui
