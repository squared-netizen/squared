#include <squared/gui/table.hpp>

#include "detail/gui_detail.hpp"
#include <squared/gui/alignment.hpp>
#include <squared/gui/cell.hpp>
#include <squared/gui/painter.hpp>
#include <squared/gui/size.hpp>
#include <squared/gui/size_hints.hpp>
#include <squared/gui/skin.hpp>
#include <squared/gui/widget.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sq::gui {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

struct Table::GridMetrics {
    std::vector<float> columns;
    std::vector<float> rows;
    float width{0.0F};
    float height{0.0F};
};

Cell& Table::add(std::unique_ptr<Widget> child)
{
    if (!child) throw std::invalid_argument("GUI widget must not be null");
    for (const Cell& existing : cells_) {
        if (existing.row_ == current_row_) {
            current_column_ = std::max(
                current_column_, existing.column_ + existing.column_span_
            );
        }
    }
    Widget* reference = child.get();
    static_cast<void>(add_actor(std::move(child)));
    try {
        cells_.push_back({});
    } catch (...) {
        [[maybe_unused]] auto removed = remove_actor(*reference);
        throw;
    }
    Cell& cell = cells_.back();
    cell.owner_ = this;
    cell.widget_ = reference;
    cell.row_ = current_row_;
    cell.column_ = current_column_++;
    invalidate_layout();
    return cell;
}

Table& Table::row() noexcept
{
    if (current_column_ != 0 || !cells_.empty()) ++current_row_;
    current_column_ = 0;
    invalidate_layout();
    return *this;
}

void Table::set_padding(float padding) noexcept
{
    padding_ = std::max(0.0F, padding);
    invalidate_layout();
}

void Table::set_spacing(float spacing) noexcept
{
    spacing_ = std::max(0.0F, spacing);
    invalidate_layout();
}

Table::GridMetrics Table::measure(
    Painter& painter,
    const Skin* skin,
    bool minimum
) const
{
    const float outer = padding_ < 0.0F
        ? (skin ? skin->padding : default_padding) : padding_;
    const float gap = spacing_ < 0.0F
        ? (skin ? skin->spacing : default_spacing) : spacing_;
    std::size_t column_count = 0;
    std::size_t row_count = 0;
    for (const Cell& cell : cells_) {
        column_count = std::max(column_count, cell.column_ + cell.column_span_);
        row_count = std::max(row_count, cell.row_ + 1);
    }
    GridMetrics result;
    result.columns.assign(column_count, 0.0F);
    result.rows.assign(row_count, 0.0F);

    for (const Cell& cell : cells_) {
        const Size size = minimum
            ? cell.widget_->minimum_size(painter, *skin)
            : cell.widget_->preferred_size(painter, *skin);
        const float padded_width = size.width + cell.padding_.left + cell.padding_.right;
        const float padded_height = size.height + cell.padding_.top + cell.padding_.bottom;
        result.rows[cell.row_] = std::max(result.rows[cell.row_], padded_height);
        if (cell.column_span_ == 1) {
            result.columns[cell.column_] = std::max(
                result.columns[cell.column_], padded_width
            );
        }
    }
    for (const Cell& cell : cells_) {
        if (cell.column_span_ == 1) continue;
        const Size size = minimum
            ? cell.widget_->minimum_size(painter, *skin)
            : cell.widget_->preferred_size(painter, *skin);
        const float needed = size.width + cell.padding_.left + cell.padding_.right;
        float present = gap * static_cast<float>(cell.column_span_ - 1);
        for (std::size_t column = cell.column_;
             column < cell.column_ + cell.column_span_; ++column) {
            present += result.columns[column];
        }
        const float extra = std::max(0.0F, needed - present) /
            static_cast<float>(cell.column_span_);
        if (extra > 0.0F) {
            for (std::size_t column = cell.column_;
                 column < cell.column_ + cell.column_span_; ++column) {
                result.columns[column] += extra;
            }
        }
    }
    result.width = 2.0F * outer;
    result.height = 2.0F * outer;
    for (float value : result.columns) result.width += value;
    for (float value : result.rows) result.height += value;
    if (!result.columns.empty()) {
        result.width += gap * static_cast<float>(result.columns.size() - 1);
    }
    if (!result.rows.empty()) {
        result.height += gap * static_cast<float>(result.rows.size() - 1);
    }
    return result;
}

Size Table::minimum_size(Painter& painter, const Skin& skin) const
{
    const GridMetrics result = measure(painter, &skin, true);
    return {result.width, result.height};
}

Size Table::preferred_size(Painter& painter, const Skin& skin) const
{
    const GridMetrics result = measure(painter, &skin, false);
    return {result.width, result.height};
}

void Table::layout(Painter& painter, const Skin& skin)
{
    GridMetrics metrics = measure(painter, &skin, false);
    const GridMetrics minimum = measure(painter, &skin, true);
    const float outer = padding_ < 0.0F ? skin.padding : padding_;
    const float gap = spacing_ < 0.0F ? skin.spacing : spacing_;
    std::vector<float> column_grow(metrics.columns.size(), 0.0F);
    std::vector<float> row_grow(metrics.rows.size(), 0.0F);
    for (const Cell& cell : cells_) {
        const float share = cell.grow_x_ / static_cast<float>(cell.column_span_);
        for (std::size_t column = cell.column_;
             column < cell.column_ + cell.column_span_; ++column) {
            column_grow[column] = std::max(column_grow[column], share);
        }
        row_grow[cell.row_] = std::max(row_grow[cell.row_], cell.grow_y_);
    }
    const auto fit = [](std::vector<float>& sizes,
                        const std::vector<float>& minimums,
                        const std::vector<float>& weights,
                        float target) {
        float used = 0.0F;
        for (float size : sizes) used += size;
        if (target < used) {
            const float shortage = used - target;
            float room = 0.0F;
            for (std::size_t index = 0; index < sizes.size(); ++index) {
                room += std::max(0.0F, sizes[index] - minimums[index]);
            }
            if (room <= 0.0F) return;
            for (std::size_t index = 0; index < sizes.size(); ++index) {
                const float available = std::max(
                    0.0F, sizes[index] - minimums[index]
                );
                sizes[index] -= std::min(
                    available, shortage * available / room
                );
            }
            return;
        }
        float total = 0.0F;
        for (float weight : weights) total += weight;
        const float extra = target - used;
        if (extra <= 0.0F || total <= 0.0F) return;
        for (std::size_t index = 0; index < sizes.size(); ++index) {
            sizes[index] += extra * weights[index] / total;
        }
    };
    const float column_gaps = metrics.columns.empty() ? 0.0F
        : gap * static_cast<float>(metrics.columns.size() - 1);
    const float row_gaps = metrics.rows.empty() ? 0.0F
        : gap * static_cast<float>(metrics.rows.size() - 1);
    fit(metrics.columns, minimum.columns, column_grow,
        std::max(0.0F, width() - 2.0F * outer - column_gaps));
    fit(metrics.rows, minimum.rows, row_grow,
        std::max(0.0F, height() - 2.0F * outer - row_gaps));

    std::vector<float> column_x(metrics.columns.size(), outer);
    std::vector<float> row_y(metrics.rows.size(), outer);
    for (std::size_t index = 1; index < column_x.size(); ++index) {
        column_x[index] = column_x[index - 1] + metrics.columns[index - 1] + gap;
    }
    for (std::size_t index = 1; index < row_y.size(); ++index) {
        row_y[index] = row_y[index - 1] + metrics.rows[index - 1] + gap;
    }
    for (Cell& cell : cells_) {
        float cell_width = 0.0F;
        for (std::size_t column = cell.column_;
             column < cell.column_ + cell.column_span_; ++column) {
            cell_width += metrics.columns[column];
        }
        cell_width += gap * static_cast<float>(cell.column_span_ - 1);
        const float available_width = std::max(
            0.0F, cell_width - cell.padding_.left - cell.padding_.right
        );
        const float available_height = std::max(
            0.0F, metrics.rows[cell.row_] - cell.padding_.top - cell.padding_.bottom
        );
        const SizeHints hints = cell.widget_->size_hints(painter, skin);
        const float child_width = cell.fill_x_ || cell.grow_x_ > 0.0F
            ? clamp_dimension(available_width, hints.minimum.width, hints.maximum.width)
            : std::min(available_width, hints.preferred.width);
        const float child_height = cell.fill_y_ || cell.grow_y_ > 0.0F
            ? clamp_dimension(available_height, hints.minimum.height, hints.maximum.height)
            : std::min(available_height, hints.preferred.height);
        const auto offset = [](float room, float size, Alignment alignment) {
            if (alignment == Alignment::end) return room - size;
            if (alignment == Alignment::center) return (room - size) * 0.5F;
            return 0.0F;
        };
        cell.widget_->set_bounds(
            column_x[cell.column_] + cell.padding_.left +
                offset(available_width, child_width, cell.horizontal_),
            row_y[cell.row_] + cell.padding_.top +
                offset(available_height, child_height, cell.vertical_),
            child_width,
            child_height
        );
        cell.widget_->invalidate_layout();
        cell.widget_->validate_layout(painter, skin);
    }
}

} // namespace sq::gui
