#include <squared/gui/image.hpp>

#include "detail/gui_detail.hpp"
#include <squared/gui/drawable_ptr.hpp>
#include <squared/gui/painter.hpp>
#include <squared/gui/size.hpp>
#include <squared/gui/skin.hpp>

#include <utility>

namespace sq::gui {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

Image::Image(DrawablePtr drawable) : drawable_(std::move(drawable))
{
    set_touchable(false);
}

void Image::set_drawable(DrawablePtr drawable)
{
    drawable_ = std::move(drawable);
    invalidate_layout();
}

Size Image::preferred_size(Painter&, const Skin&) const
{
    return drawable_ ? drawable_->minimum_size() : Size{};
}

void Image::paint(Painter& painter, const Skin&, float x, float y) const
{
    if (drawable_) drawable_->draw(painter, bounds_of(*this, x, y));
}

} // namespace sq::gui
