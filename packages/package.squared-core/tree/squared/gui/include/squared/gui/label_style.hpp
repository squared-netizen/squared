#pragma once

#include <squared/graphics/color.hpp>
#include <squared/gui/font_ptr.hpp>

#include <optional>

namespace sq::gui {

/** @brief Style data for a Label widget. */
struct LabelStyle {
    /** @brief Font resource, or empty to use the painter default. */
    FontPtr font;
    /** @brief Primary label color. */
    std::optional<graphics::Color> text;
    /** @brief Muted label color. */
    std::optional<graphics::Color> muted_text;
};

} // namespace sq::gui
