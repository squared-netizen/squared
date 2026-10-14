#pragma once

#include <squared/gui/src/drawable_ptr.hpp>

namespace sq::gui
{

/** @brief Style data for a Panel widget. */
struct PanelStyle
{
        /** @brief Background drawable; may be empty for no background. */
        DrawablePtr background;
};

} // namespace sq::gui
