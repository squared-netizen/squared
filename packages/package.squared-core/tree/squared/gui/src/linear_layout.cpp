#include <squared/gui/linear_layout.hpp>

#include "detail/gui_detail.hpp"
#include <squared/gui/direction.hpp>
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

LinearLayout::LinearLayout(Direction direction) noexcept
    : direction_(direction)
{
}

Widget& LinearLayout::add(std::unique_ptr<Widget> child, float grow)
{
    if (!child) throw std::invalid_argument("GUI widget must not be null");
    Widget* reference = child.get();
    static_cast<void>(add_actor(std::move(child)));
    try {
        slots_.push_back({reference, std::max(0.0F, grow)});
    } catch (...) {
        [[maybe_unused]] auto removed = remove_actor(*reference);
        throw;
    }
    invalidate_layout();
    return *reference;
}

void LinearLayout::set_padding(float padding) noexcept
{
    padding_ = std::max(0.0F, padding);
    invalidate_layout();
}

void LinearLayout::set_spacing(float spacing) noexcept
{
    spacing_ = std::max(0.0F, spacing);
    invalidate_layout();
}

Size LinearLayout::measured_size(
    Painter& painter,
    const Skin* skin,
    bool minimum
) const
{
    const float padding = padding_ < 0.0F
        ? (skin ? skin->padding : default_padding) : padding_;
    const float spacing = spacing_ < 0.0F
        ? (skin ? skin->spacing : default_spacing) : spacing_;
    Size result{padding * 2.0F, padding * 2.0F};
    for (std::size_t index = 0; index < slots_.size(); ++index) {
        const Size child = minimum
            ? slots_[index].widget->minimum_size(painter, *skin)
            : slots_[index].widget->preferred_size(painter, *skin);
        if (direction_ == Direction::horizontal) {
            result.width += child.width;
            result.height = std::max(result.height, child.height + padding * 2.0F);
        } else {
            result.width = std::max(result.width, child.width + padding * 2.0F);
            result.height += child.height;
        }
        if (index != 0) {
            if (direction_ == Direction::horizontal) result.width += spacing;
            else result.height += spacing;
        }
    }
    return result;
}

Size LinearLayout::minimum_size(Painter& painter, const Skin& skin) const
{
    return measured_size(painter, &skin, true);
}

Size LinearLayout::preferred_size(Painter& painter, const Skin& skin) const
{
    return measured_size(painter, &skin, false);
}

void LinearLayout::layout(Painter& painter, const Skin& skin)
{
    const float padding = padding_ < 0.0F ? skin.padding : padding_;
    const float spacing = spacing_ < 0.0F ? skin.spacing : spacing_;
    const float main_extent = direction_ == Direction::horizontal
        ? width() : height();
    const float cross_extent = direction_ == Direction::horizontal
        ? height() : width();
    const float gaps = slots_.empty()
        ? 0.0F : spacing * static_cast<float>(slots_.size() - 1);
    const float available = std::max(0.0F, main_extent - 2.0F * padding - gaps);

    std::vector<SizeHints> hints;
    std::vector<float> main_sizes;
    hints.reserve(slots_.size());
    main_sizes.reserve(slots_.size());
    float used = 0.0F;
    float total_grow = 0.0F;
    for (const Slot& slot : slots_) {
        hints.push_back(slot.widget->size_hints(painter, skin));
        const Size& preferred = hints.back().preferred;
        const float value = direction_ == Direction::horizontal
            ? preferred.width : preferred.height;
        main_sizes.push_back(value);
        used += value;
        total_grow += slot.grow;
    }

    if (used > available && used > 0.0F) {
        const float shortage = used - available;
        float shrink_room = 0.0F;
        for (std::size_t index = 0; index < slots_.size(); ++index) {
            const float minimum = direction_ == Direction::horizontal
                ? hints[index].minimum.width : hints[index].minimum.height;
            shrink_room += std::max(0.0F, main_sizes[index] - minimum);
        }
        for (std::size_t index = 0; index < slots_.size(); ++index) {
            const float minimum = direction_ == Direction::horizontal
                ? hints[index].minimum.width : hints[index].minimum.height;
            const float room = std::max(0.0F, main_sizes[index] - minimum);
            const float reduction = shrink_room > 0.0F
                ? std::min(room, shortage * room / shrink_room)
                : 0.0F;
            main_sizes[index] -= reduction;
        }
    } else if (available > used && total_grow > 0.0F) {
        const float extra = available - used;
        for (std::size_t index = 0; index < slots_.size(); ++index) {
            const float maximum = direction_ == Direction::horizontal
                ? hints[index].maximum.width : hints[index].maximum.height;
            main_sizes[index] = std::min(
                maximum,
                main_sizes[index] + extra * slots_[index].grow / total_grow
            );
        }
    }

    float cursor = padding;
    for (std::size_t index = 0; index < slots_.size(); ++index) {
        Widget& child = *slots_[index].widget;
        const float cross_minimum = direction_ == Direction::horizontal
            ? hints[index].minimum.height : hints[index].minimum.width;
        const float cross_maximum = direction_ == Direction::horizontal
            ? hints[index].maximum.height : hints[index].maximum.width;
        const float cross_size = clamp_dimension(
            std::max(0.0F, cross_extent - 2.0F * padding),
            cross_minimum,
            cross_maximum
        );
        if (direction_ == Direction::horizontal) {
            child.set_bounds(cursor, padding, main_sizes[index], cross_size);
        } else {
            child.set_bounds(padding, cursor, cross_size, main_sizes[index]);
        }
        child.invalidate_layout();
        child.validate_layout(painter, skin);
        cursor += main_sizes[index] + spacing;
    }
}

} // namespace sq::gui
