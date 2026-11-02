export module sq.gui:src.panel_style;

export import :src.drawable_ptr;

export namespace sq::gui
{

/** @brief Style data for a Panel widget. */
struct PanelStyle
{
        /** @brief Background drawable; may be empty for no background. */
        DrawablePtr background;
};

} // namespace sq::gui
