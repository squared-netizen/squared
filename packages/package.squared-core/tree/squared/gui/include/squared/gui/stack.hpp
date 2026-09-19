#pragma once

#include <squared/gui/size.hpp>
#include <squared/gui/widget.hpp>

#include <memory>

namespace sq::gui {

class Painter;
class Skin;

/** @brief Container overlaying children into the same box, later on top. */
class Stack final : public Widget {
public:
    /**
     * @brief Add a child widget to the stack.
     * @param child Widget to adopt; ownership transfers to the stack.
     * @return Reference to the added child.
     */
    Widget& add(std::unique_ptr<Widget> child);

    /**
     * @brief Report the largest child as preferred.
     * @param painter Active painter used for measurement.
     * @param skin Skin forwarded to every child widget.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Expand every child to the stack box.
     * @note Parameters match Widget::layout: an active painter and the skin
     * providing style values.
     */
    void layout(Painter&, const Skin&) override;
};

} // namespace sq::gui
