#pragma once

#include <squared/gui/alignment.hpp>
#include <squared/gui/insets.hpp>

#include <cstddef>

namespace sq::gui {

class Table;
class Widget;

/** @brief Per-child constraints returned by Table::add for fluent configuration. */
class Cell {
public:
    /**
     * @brief Span multiple grid columns.
     * @param columns Number of grid columns covered by this cell; must be at
     * least one.
     * @return This cell for chaining.
     */
    Cell& column_span(std::size_t columns) noexcept;

    /**
     * @brief Stretch equally with other growing cells on both axes.
     * @return This cell for chaining.
     */
    Cell& grow() noexcept;

    /**
     * @brief Stretch horizontally with other growing cells.
     * @return This cell for chaining.
     */
    Cell& grow_x() noexcept;

    /**
     * @brief Stretch vertically with other growing cells.
     * @return This cell for chaining.
     */
    Cell& grow_y() noexcept;

    /**
     * @brief Expand to fill available cell space.
     * @return This cell for chaining.
     */
    Cell& fill() noexcept;

    /**
     * @brief Expand horizontally to fill available cell width.
     * @return This cell for chaining.
     */
    Cell& fill_x() noexcept;

    /**
     * @brief Expand vertically to fill available cell height.
     * @return This cell for chaining.
     */
    Cell& fill_y() noexcept;

    /**
     * @brief Set uniform padding on all four sides.
     * @param value Padding in logical units; non-negative.
     * @return This cell for chaining.
     */
    Cell& pad(float value) noexcept;

    /**
     * @brief Set per-side padding.
     * @param value Insets in logical units; non-negative.
     * @return This cell for chaining.
     */
    Cell& pad(Insets value) noexcept;

    /**
     * @brief Set horizontal and vertical alignment within the cell.
     * @param horizontal start, center, or end alignment.
     * @param vertical start, center, or end alignment.
     * @return This cell for chaining.
     */
    Cell& align(Alignment horizontal, Alignment vertical = Alignment::center) noexcept;

    /**
     * @brief Access the child widget placed in this cell.
     * @return Reference to the cell's child widget.
     */
    [[nodiscard]] Widget& widget() noexcept { return *widget_; }

private:
    friend class Table;
    void changed() noexcept;
    Table* owner_{nullptr};
    Widget* widget_{nullptr};
    std::size_t row_{0};
    std::size_t column_{0};
    std::size_t column_span_{1};
    float grow_x_{0.0F};
    float grow_y_{0.0F};
    bool fill_x_{false};
    bool fill_y_{false};
    Insets padding_{};
    Alignment horizontal_{Alignment::center};
    Alignment vertical_{Alignment::center};
};

} // namespace sq::gui
