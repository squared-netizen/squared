#pragma once

#include <squared/gui/drawable_ptr.hpp>
#include <squared/gui/size.hpp>
#include <squared/gui/widget.hpp>

namespace sq::gui {

class Painter;
class Skin;

/** @brief Widget rendering one shared drawable. */
class Image final : public Widget {
public:
    /**
     * @brief Construct an image widget.
     * @param drawable Drawable to display, or empty for a blank image.
     */
    explicit Image(DrawablePtr drawable = {});

    /**
     * @brief Replace the displayed drawable.
     * @param drawable Drawable to display; shared ownership.
     */
    void set_drawable(DrawablePtr drawable);

    /**
     * @brief Report the drawable size.
     * @param painter Active painter used for measurement.
     * @param skin Active skin; unused by image measurement.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Paint the drawable.
     * @note Parameters match the Widget::paint signature: the active painter,
     * the skin providing style resources, and the stage-space origin.
     */
    void paint(Painter&, const Skin&, float, float) const override;

private:
    DrawablePtr drawable_;
};

} // namespace sq::gui
