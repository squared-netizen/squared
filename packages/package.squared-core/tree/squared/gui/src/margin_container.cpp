#include <squared/gui/margin_container.hpp>

#include "detail/gui_detail.hpp"
#include <squared/gui/insets.hpp>
#include <squared/gui/painter.hpp>
#include <squared/gui/size.hpp>
#include <squared/gui/skin.hpp>
#include <squared/gui/widget.hpp>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <utility>

namespace sq::gui {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

MarginContainer::MarginContainer(Insets margin) noexcept : margin_(margin) {}

Widget& MarginContainer::set_content(std::unique_ptr<Widget> content)
{
    if (!content) throw std::invalid_argument("GUI content must not be null");
    clear();
    content_ = content.get();
    static_cast<void>(add_actor(std::move(content)));
    invalidate_layout();
    return *content_;
}

Size MarginContainer::preferred_size(Painter& painter, const Skin& skin) const
{
    const Size child = content_
        ? content_->preferred_size(painter, skin) : Size{};
    return {
        child.width + margin_.left + margin_.right,
        child.height + margin_.top + margin_.bottom
    };
}

void MarginContainer::layout(Painter& painter, const Skin& skin)
{
    if (!content_) return;
    content_->set_bounds(
        margin_.left,
        margin_.top,
        std::max(0.0F, width() - margin_.left - margin_.right),
        std::max(0.0F, height() - margin_.top - margin_.bottom)
    );
    content_->invalidate_layout();
    content_->validate_layout(painter, skin);
}

} // namespace sq::gui
