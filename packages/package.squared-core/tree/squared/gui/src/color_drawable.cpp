#include <squared/gui/color_drawable.hpp>

#include "detail/gui_detail.hpp"
#include <squared/graphics/color.hpp>
#include <squared/gui/insets.hpp>
#include <squared/gui/painter.hpp>
#include <squared/gui/rectangle.hpp>
#include <squared/gui/size.hpp>

namespace sq::gui {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

ColorDrawable::ColorDrawable(
    graphics::Color color,
    Size minimum,
    Insets insets
) noexcept
    : color_(color), minimum_(minimum), insets_(insets)
{
}

Size ColorDrawable::minimum_size() const noexcept { return minimum_; }

Insets ColorDrawable::content_insets() const noexcept { return insets_; }

void ColorDrawable::draw(
    Painter& painter,
    const Rectangle& rectangle,
    graphics::Color tint
) const
{
    painter.fill_rectangle(rectangle, multiply(color_, tint));
}

} // namespace sq::gui
