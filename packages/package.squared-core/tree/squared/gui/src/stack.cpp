#include <squared/gui/stack.hpp>

#include "detail/gui_detail.hpp"
#include <squared/gui/painter.hpp>
#include <squared/gui/size.hpp>
#include <squared/gui/skin.hpp>
#include <squared/gui/widget.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>

namespace sq::gui {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

Widget& Stack::add(std::unique_ptr<Widget> child)
{
    if (!child) throw std::invalid_argument("GUI widget must not be null");
    Widget& result = static_cast<Widget&>(add_actor(std::move(child)));
    invalidate_layout();
    return result;
}

Size Stack::preferred_size(Painter& painter, const Skin& skin) const
{
    Size result{};
    for (std::size_t index = 0; index < child_count(); ++index) {
        if (const auto* child =
                scene2d::actor_cast<Widget>(child_at(index))) {
            const Size size = child->preferred_size(painter, skin);
            result.width = std::max(result.width, size.width);
            result.height = std::max(result.height, size.height);
        }
    }
    return result;
}

void Stack::layout(Painter& painter, const Skin& skin)
{
    for (std::size_t index = 0; index < child_count(); ++index) {
        if (auto* child = scene2d::actor_cast<Widget>(child_at(index))) {
            child->set_bounds(0.0F, 0.0F, width(), height());
            child->invalidate_layout();
            child->validate_layout(painter, skin);
        }
    }
}

} // namespace sq::gui
