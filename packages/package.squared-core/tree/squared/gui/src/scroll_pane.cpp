#include <squared/gui/scroll_pane.hpp>

#include "detail/gui_detail.hpp"
#include <squared/gui/painter.hpp>
#include <squared/gui/pointer_action.hpp>
#include <squared/gui/pointer_event.hpp>
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

Widget& ScrollPane::set_content(std::unique_ptr<Widget> content)
{
    if (!content) throw std::invalid_argument("GUI content must not be null");
    clear();
    content_ = content.get();
    static_cast<void>(add_actor(std::move(content)));
    invalidate_layout();
    return *content_;
}

void ScrollPane::set_scroll_y(float scroll_y) noexcept
{
    scroll_y_ = std::max(0.0F, scroll_y);
    clamp_scroll();
    if (content_) content_->set_position(0.0F, -scroll_y_);
}

Size ScrollPane::preferred_size(Painter& painter, const Skin& skin) const
{
    return content_ ? content_->preferred_size(painter, skin) : Size{};
}

void ScrollPane::layout(Painter& painter, const Skin& skin)
{
    if (!content_) return;
    const Size preferred = content_->preferred_size(painter, skin);
    content_->set_bounds(
        0.0F,
        -scroll_y_,
        std::max(width(), preferred.width),
        std::max(height(), preferred.height)
    );
    clamp_scroll();
    content_->set_position(0.0F, -scroll_y_);
    content_->invalidate_layout();
    content_->validate_layout(painter, skin);
}

bool ScrollPane::pointer_event(const PointerEvent& event)
{
    if (!enabled()) return false;
    if (event.action == PointerAction::down) {
        drag_pointer_ = event.pointer_id;
        last_pointer_y_ = event.y;
        return true;
    }
    if (!drag_pointer_ || *drag_pointer_ != event.pointer_id) return false;
    if (event.action == PointerAction::move) {
        set_scroll_y(scroll_y_ + last_pointer_y_ - event.y);
        last_pointer_y_ = event.y;
        return true;
    }
    drag_pointer_.reset();
    return true;
}

void ScrollPane::clamp_scroll() noexcept
{
    const float maximum = content_
        ? std::max(0.0F, content_->height() - height()) : 0.0F;
    scroll_y_ = std::clamp(scroll_y_, 0.0F, maximum);
}

} // namespace sq::gui
