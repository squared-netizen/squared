#include <squared/gui/separator.hpp>

#include "detail/gui_detail.hpp"
#include <squared/gui/direction.hpp>
#include <squared/gui/painter.hpp>
#include <squared/gui/size.hpp>
#include <squared/gui/skin.hpp>

namespace sq::gui {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

Separator::Separator(Direction direction) noexcept : direction_(direction)
{
    set_touchable(false);
}

Size Separator::preferred_size(Painter&, const Skin&) const
{
    return direction_ == Direction::horizontal
        ? Size{0.0F, 1.0F} : Size{1.0F, 0.0F};
}

void Separator::paint(Painter& painter, const Skin& skin, float x, float y) const
{
    painter.fill_rectangle(bounds_of(*this, x, y), skin.border);
}

} // namespace sq::gui
