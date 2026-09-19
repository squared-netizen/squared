#include <squared/gui/region_drawable.hpp>

#include "detail/gui_detail.hpp"
#include <squared/graphics/color.hpp>
#include <squared/graphics2d/texture_region.hpp>
#include <squared/gui/insets.hpp>
#include <squared/gui/painter.hpp>
#include <squared/gui/rectangle.hpp>
#include <squared/gui/size.hpp>

namespace sq::gui {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

RegionDrawable::RegionDrawable(
    const graphics2d::TextureRegion& region,
    Insets insets
) noexcept
    : region_(&region), insets_(insets)
{
}

Size RegionDrawable::minimum_size() const noexcept
{
    return {
        static_cast<float>(region_->width()),
        static_cast<float>(region_->height())
    };
}

Insets RegionDrawable::content_insets() const noexcept { return insets_; }

void RegionDrawable::draw(
    Painter& painter,
    const Rectangle& rectangle,
    graphics::Color tint
) const
{
    painter.draw_region(*region_, rectangle, tint);
}

} // namespace sq::gui
