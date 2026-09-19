#pragma once

#include <squared/gui/cell.hpp>
#include <squared/gui/size.hpp>
#include <squared/gui/widget.hpp>

#include <cstddef>
#include <deque>
#include <memory>

namespace sq::gui {

class Painter;
class Skin;

/**
 * @brief Grid layout with libGDX-style rows and chainable cell constraints.
 */
class Table final : public Widget {
public:
    /**
     * @brief Add a child widget to the current cell.
     * @param child Widget to adopt; ownership transfers to the table.
     * @return Configurable Cell describing the child's placement.
     */
    Cell& add(std::unique_ptr<Widget> child);

    /**
     * @brief Advance to the next grid row.
     * @return Reference to this table for chaining.
     */
    Table& row() noexcept;

    /**
     * @brief Set table padding.
     * @param padding Padding in logical units; non-negative.
     */
    void set_padding(float padding) noexcept;

    /**
     * @brief Set spacing between rows and columns.
     * @param spacing Gap in logical units; non-negative.
     */
    void set_spacing(float spacing) noexcept;

    /**
     * @brief Report the smallest size fitting every cell.
     * @note Parameters match Widget::minimum_size: an active painter and the
     * skin providing style values.
     * @return Minimum extent in logical units.
     */
    [[nodiscard]] Size minimum_size(Painter&, const Skin&) const override;

    /**
     * @brief Report the preferred size from cell content.
     * @param painter Active painter used for measurement.
     * @param skin Skin forwarded to every cell widget.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Compute cell rectangles and position every child.
     * @note Parameters match Widget::layout: an active painter and the skin
     * providing style values.
     */
    void layout(Painter&, const Skin&) override;

private:
    struct GridMetrics;
    [[nodiscard]] GridMetrics measure(Painter&, const Skin*, bool minimum) const;
    float padding_{-1.0F};
    float spacing_{-1.0F};
    std::size_t current_row_{0};
    std::size_t current_column_{0};
    std::deque<Cell> cells_;
};

} // namespace sq::gui
