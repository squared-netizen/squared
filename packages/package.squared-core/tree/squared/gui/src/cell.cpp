#include <squared/gui/cell.hpp>

#include "detail/gui_detail.hpp"
#include <squared/gui/alignment.hpp>
#include <squared/gui/insets.hpp>
#include <squared/gui/table.hpp>
#include <squared/gui/widget.hpp>

#include <algorithm>
#include <cstddef>

namespace sq::gui {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

void Cell::changed() noexcept
{
    if (owner_) owner_->invalidate_layout();
}

Cell& Cell::column_span(std::size_t columns) noexcept
{
    column_span_ = std::max<std::size_t>(1, columns);
    changed();
    return *this;
}

Cell& Cell::grow() noexcept { return grow_x().grow_y(); }

Cell& Cell::grow_x() noexcept { grow_x_ = 1.0F; changed(); return *this; }

Cell& Cell::grow_y() noexcept { grow_y_ = 1.0F; changed(); return *this; }

Cell& Cell::fill() noexcept { return fill_x().fill_y(); }

Cell& Cell::fill_x() noexcept { fill_x_ = true; changed(); return *this; }

Cell& Cell::fill_y() noexcept { fill_y_ = true; changed(); return *this; }

Cell& Cell::pad(float value) noexcept
{
    return pad(Insets{value, value, value, value});
}

Cell& Cell::pad(Insets value) noexcept
{
    padding_ = {
        std::max(0.0F, value.left), std::max(0.0F, value.top),
        std::max(0.0F, value.right), std::max(0.0F, value.bottom)
    };
    changed();
    return *this;
}

Cell& Cell::align(Alignment horizontal, Alignment vertical) noexcept
{
    horizontal_ = horizontal;
    vertical_ = vertical;
    changed();
    return *this;
}

} // namespace sq::gui
