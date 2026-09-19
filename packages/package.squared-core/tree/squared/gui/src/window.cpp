#include <squared/gui/window.hpp>

#include "detail/gui_detail.hpp"
#include <squared/gui/drawable_ptr.hpp>
#include <squared/gui/painter.hpp>
#include <squared/gui/pointer_action.hpp>
#include <squared/gui/pointer_event.hpp>
#include <squared/gui/rectangle.hpp>
#include <squared/gui/size.hpp>
#include <squared/gui/skin.hpp>
#include <squared/gui/table.hpp>
#include <squared/gui/window_style.hpp>
#include <squared/scene2d/group.hpp>

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

namespace sq::gui {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

Window::Window(std::string title, std::string style)
    : title_(std::move(title)), style_(std::move(style))
{
    auto content = std::make_unique<Table>();
    content_ = content.get();
    static_cast<void>(add_actor(std::move(content)));
}

void Window::set_title(std::string title)
{
    title_ = std::move(title);
    invalidate_layout();
}

void Window::set_style(std::string style)
{
    style_ = std::move(style);
    invalidate_layout();
}

void Window::set_closable(bool closable) noexcept
{
    if (closable_ == closable) return;
    closable_ = closable;
    invalidate_layout();
}

void Window::set_minimum_window_size(Size size) noexcept
{
    requested_minimum_ = {
        std::max(0.0F, size.width), std::max(0.0F, size.height)
    };
    invalidate_layout();
}

Size Window::minimum_size(Painter& painter, const Skin& skin) const
{
    const WindowStyle& style = skin.window_style(style_);
    const Size table = content_->minimum_size(painter, skin);
    const Size title = painter.measure_text(title_, style.title_font.get());
    const Size background = style.background
        ? style.background->minimum_size() : Size{};
    const float title_controls = closable_
        ? std::max(0.0F, style.close_size) + style.content_insets.right
        : 0.0F;
    return {
        std::max({
            table.width + style.content_insets.left + style.content_insets.right,
            title.width + style.content_insets.left + title_controls +
                style.content_insets.right,
            background.width,
            requested_minimum_.width
        }),
        std::max({
            table.height + style.title_height + style.content_insets.top +
                style.content_insets.bottom,
            background.height,
            requested_minimum_.height
        })
    };
}

Size Window::preferred_size(Painter& painter, const Skin& skin) const
{
    const auto& style = skin.window_style(style_);
    const Size table = content_->preferred_size(painter, skin);
    const Size title = painter.measure_text(title_, style.title_font.get());
    const float title_controls = closable_
        ? style.close_size + style.content_insets.right : 0.0F;
    return {
        std::max({
            table.width + style.content_insets.left + style.content_insets.right,
            title.width + style.content_insets.left + title_controls,
            requested_minimum_.width
        }),
        std::max(table.height + style.title_height + style.content_insets.top +
                     style.content_insets.bottom,
                 requested_minimum_.height)
    };
}

void Window::layout(Painter& painter, const Skin& skin)
{
    const WindowStyle& style = skin.window_style(style_);
    title_height_ = std::max(
        style.title_height,
        painter.measure_text(title_, style.title_font.get()).height
    );
    close_size_ = std::min(title_height_, std::max(0.0F, style.close_size));
    resize_border_ = std::max(1.0F, style.resize_border);
    measured_minimum_ = minimum_size(painter, skin);
    if (width() < measured_minimum_.width || height() < measured_minimum_.height) {
        set_size(
            std::max(width(), measured_minimum_.width),
            std::max(height(), measured_minimum_.height)
        );
        constrain_to_parent();
    }
    content_->set_bounds(
        style.content_insets.left,
        title_height_ + style.content_insets.top,
        std::max(0.0F, width() - style.content_insets.left - style.content_insets.right),
        std::max(0.0F, height() - title_height_ - style.content_insets.top -
            style.content_insets.bottom)
    );
    content_->invalidate_layout();
    content_->validate_layout(painter, skin);
}

void Window::paint(Painter& painter, const Skin& skin, float x, float y) const
{
    const WindowStyle& style = skin.window_style(style_);
    if (style.background) style.background->draw(painter, bounds_of(*this, x, y));
    if (style.title_background) {
        style.title_background->draw(painter, {x, y, width(), title_height_});
    }
    const Size title_size = painter.measure_text(title_, style.title_font.get());
    painter.draw_text(
        title_, x + style.content_insets.left,
        y + std::max(0.0F, (title_height_ - title_size.height) * 0.5F),
        style.title_font.get(), style.title_text
    );
    if (closable_) {
        const Rectangle bounds = close_bounds();
        DrawablePtr background = close_pressed_ ? style.close_pressed
            : (close_hovered_ ? style.close_hovered : style.close_normal);
        if (background) {
            background->draw(
                painter,
                {x + bounds.x, y + bounds.y, bounds.width, bounds.height}
            );
        }
        const Size close_text = painter.measure_text("x", style.title_font.get());
        painter.draw_text(
            "x",
            x + bounds.x + std::max(0.0F, (bounds.width - close_text.width) * 0.5F),
            y + bounds.y + std::max(0.0F, (bounds.height - close_text.height) * 0.5F),
            style.title_font.get(), style.close_text
        );
    }
}

bool Window::pointer_event(const PointerEvent& event)
{
    if (!enabled()) return false;
    const bool inside = contains(event.x, event.y);
    if (event.action == PointerAction::down) {
        const Rectangle close = close_bounds();
        const bool over_close = closable_ &&
            event.x >= close.x && event.x <= close.x + close.width &&
            event.y >= close.y && event.y <= close.y + close.height;
        if (over_close) {
            interaction_pointer_ = event.pointer_id;
            close_pressed_ = true;
            close_hovered_ = true;
            return true;
        }
        const unsigned int edges = resizable_
            ? resize_edges(event.x, event.y) : resize_none;
        if (inside && edges != resize_none) {
            interaction_pointer_ = event.pointer_id;
            resize_edges_ = edges;
            resize_start_ = {x(), y(), width(), height()};
            pointer_start_x_ = x() + event.x;
            pointer_start_y_ = y() + event.y;
            return true;
        }
        if (movable_ && inside && event.y <= title_height_) {
            interaction_pointer_ = event.pointer_id;
            drag_offset_x_ = event.x;
            drag_offset_y_ = event.y;
            return true;
        }
        return inside || modal_;
    }
    if (interaction_pointer_ && *interaction_pointer_ == event.pointer_id) {
        const Rectangle close = close_bounds();
        const bool over_close = event.x >= close.x && event.x <= close.x + close.width &&
            event.y >= close.y && event.y <= close.y + close.height;
        if (close_pressed_) {
            close_hovered_ = over_close;
            if (event.action == PointerAction::up ||
                event.action == PointerAction::cancel) {
                const bool close_window = event.action == PointerAction::up && over_close;
                close_pressed_ = false;
                interaction_pointer_.reset();
                if (close_window) request_close();
            }
            return true;
        }
        if (resize_edges_ != resize_none) {
            if (event.action == PointerAction::move) {
                resize_from_pointer(x() + event.x, y() + event.y);
            } else {
                resize_edges_ = resize_none;
                interaction_pointer_.reset();
            }
            return true;
        }
        if (event.action == PointerAction::move) {
            set_position(
                x() + event.x - drag_offset_x_,
                y() + event.y - drag_offset_y_
            );
            constrain_to_parent();
        } else {
            interaction_pointer_.reset();
        }
        return true;
    }
    if (event.action == PointerAction::move && closable_) {
        const Rectangle close = close_bounds();
        close_hovered_ = event.x >= close.x && event.x <= close.x + close.width &&
            event.y >= close.y && event.y <= close.y + close.height;
    }
    return inside || modal_;
}

Rectangle Window::close_bounds() const noexcept
{
    const float size = std::min(title_height_, close_size_);
    return {
        std::max(0.0F, width() - size - 4.0F),
        std::max(0.0F, (title_height_ - size) * 0.5F),
        size,
        size
    };
}

unsigned int Window::resize_edges(float local_x, float local_y) const noexcept
{
    if (!contains(local_x, local_y)) return resize_none;
    unsigned int result = resize_none;
    if (local_x <= resize_border_) result |= resize_left;
    if (local_x >= width() - resize_border_) result |= resize_right;
    if (local_y <= resize_border_) result |= resize_top;
    if (local_y >= height() - resize_border_) result |= resize_bottom;
    return result;
}

void Window::constrain_to_parent() noexcept
{
    const scene2d::Group* owner = parent();
    if (!owner) return;
    const float minimum_width = std::max(
        requested_minimum_.width, measured_minimum_.width
    );
    const float minimum_height = std::max(
        requested_minimum_.height, measured_minimum_.height
    );
    const float constrained_width = owner->width() >= minimum_width
        ? std::min(width(), owner->width()) : width();
    const float constrained_height = owner->height() >= minimum_height
        ? std::min(height(), owner->height()) : height();
    if (constrained_width != width() || constrained_height != height()) {
        set_size(constrained_width, constrained_height);
        invalidate_layout();
    }
    set_position(
        std::clamp(x(), 0.0F, std::max(0.0F, owner->width() - width())),
        std::clamp(y(), 0.0F, std::max(0.0F, owner->height() - height()))
    );
}

void Window::resize_from_pointer(float stage_x, float stage_y) noexcept
{
    const float delta_x = stage_x - pointer_start_x_;
    const float delta_y = stage_y - pointer_start_y_;
    float left = resize_start_.x;
    float top = resize_start_.y;
    float right = resize_start_.x + resize_start_.width;
    float bottom = resize_start_.y + resize_start_.height;
    if ((resize_edges_ & resize_left) != 0U) left += delta_x;
    if ((resize_edges_ & resize_right) != 0U) right += delta_x;
    if ((resize_edges_ & resize_top) != 0U) top += delta_y;
    if ((resize_edges_ & resize_bottom) != 0U) bottom += delta_y;

    const float minimum_width = std::max(
        requested_minimum_.width, measured_minimum_.width
    );
    const float minimum_height = std::max(
        requested_minimum_.height, measured_minimum_.height
    );
    if (right - left < minimum_width) {
        if ((resize_edges_ & resize_left) != 0U) left = right - minimum_width;
        else right = left + minimum_width;
    }
    if (bottom - top < minimum_height) {
        if ((resize_edges_ & resize_top) != 0U) top = bottom - minimum_height;
        else bottom = top + minimum_height;
    }

    if (const scene2d::Group* owner = parent()) {
        if ((resize_edges_ & resize_left) != 0U && left < 0.0F) left = 0.0F;
        if ((resize_edges_ & resize_top) != 0U && top < 0.0F) top = 0.0F;
        if ((resize_edges_ & resize_right) != 0U && right > owner->width()) {
            right = owner->width();
        }
        if ((resize_edges_ & resize_bottom) != 0U && bottom > owner->height()) {
            bottom = owner->height();
        }
    }
    set_bounds(left, top, std::max(0.0F, right - left),
               std::max(0.0F, bottom - top));
    invalidate_layout();
}

} // namespace sq::gui
