#pragma once

#include <squared/gui/direction.hpp>
#include <squared/gui/size.hpp>
#include <squared/gui/widget.hpp>

#include <memory>
#include <vector>

namespace sq::gui {

class Painter;
class Skin;

/**
 * @brief Container stacking children along one axis with optional growth.
 */
class LinearLayout final : public Widget {
public:
    /**
     * @brief Construct a linear layout.
     * @param direction Stacking axis.
     */
    explicit LinearLayout(Direction direction = Direction::vertical) noexcept;

    /**
     * @brief Add a child widget.
     * @param child Widget to adopt; ownership transfers to the layout.
     * @param grow Extra stretch share for the child, in logical units.
     * @return Reference to the added child.
     */
    Widget& add(std::unique_ptr<Widget> child, float grow = 0.0F);

    /**
     * @brief Set layout padding.
     * @param padding Padding in logical units; non-negative.
     */
    void set_padding(float padding) noexcept;

    /**
     * @brief Set spacing between children.
     * @param spacing Gap in logical units; non-negative.
     */
    void set_spacing(float spacing) noexcept;

    /**
     * @brief Report the smallest size fitting all children.
     * @note Parameters match Widget::minimum_size: an active painter and the
     * skin providing style values.
     * @return Minimum extent in logical units.
     */
    [[nodiscard]] Size minimum_size(Painter&, const Skin&) const override;

    /**
     * @brief Report the preferred size from stacked content.
     * @param painter Active painter used for measurement.
     * @param skin Skin forwarded to every child widget.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Position children along the layout axis.
     * @note Parameters match Widget::layout: an active painter and the skin
     * providing style values.
     */
    void layout(Painter&, const Skin&) override;

private:
    struct Slot { Widget* widget{nullptr}; float grow{0.0F}; };
    [[nodiscard]] Size measured_size(Painter&, const Skin*, bool minimum) const;
    Direction direction_;
    float padding_{-1.0F};
    float spacing_{-1.0F};
    std::vector<Slot> slots_;
};

} // namespace sq::gui
