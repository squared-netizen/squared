#pragma once

#include <squared/gui/direction.hpp>
#include <squared/gui/size.hpp>
#include <squared/gui/widget.hpp>

namespace sq::gui {

class Painter;
class Skin;

/** @brief Thin vertical or horizontal divider line. */
class Separator final : public Widget {
public:
    /**
     * @brief Construct a separator.
     * @param direction Orientation of the divider.
     */
    explicit Separator(Direction direction = Direction::horizontal) noexcept;

    /**
     * @brief Report the line-driven preferred size.
     * @param painter Active painter used for measurement.
     * @param skin Active skin; unused by separator measurement.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Paint the divider line.
     * @note Parameters match the Widget::paint signature: the active painter,
     * the skin providing the border color, and the stage-space origin.
     */
    void paint(Painter&, const Skin&, float, float) const override;

private:
    Direction direction_;
};

} // namespace sq::gui
