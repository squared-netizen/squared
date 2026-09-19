#include <squared/gui/text_field.hpp>

#include "detail/gui_detail.hpp"
#include <squared/gui/drawable_ptr.hpp>
#include <squared/gui/key.hpp>
#include <squared/gui/key_modifiers.hpp>
#include <squared/gui/painter.hpp>
#include <squared/gui/pointer_action.hpp>
#include <squared/gui/pointer_event.hpp>
#include <squared/gui/size.hpp>
#include <squared/gui/skin.hpp>

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace sq::gui {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

TextField::TextField(std::string text) : text_(std::move(text))
{
    cursor_ = text_.size();
}

void TextField::set_text(std::string text)
{
    text_ = std::move(text);
    cursor_ = text_.size();
    invalidate_layout();
}

void TextField::set_style(std::string style)
{
    style_ = std::move(style);
    invalidate_layout();
}

Size TextField::minimum_size(Painter&, const Skin& skin) const
{
    const auto& style = skin.text_field_style(style_);
    return {80.0F, std::max(skin.minimum_touch_size, style.minimum_height)};
}

Size TextField::preferred_size(Painter& painter, const Skin& skin) const
{
    const auto& style = skin.text_field_style(style_);
    const Size text_size = painter.measure_text(
        text_.empty() ? std::string_view{"M"} : std::string_view{text_},
        style.font.get()
    );
    return {
        std::max(120.0F, text_size.width + 2.0F * style.horizontal_padding),
        std::max(skin.minimum_touch_size, style.minimum_height)
    };
}

void TextField::paint(Painter& painter, const Skin& skin, float x, float y) const
{
    const auto& style = skin.text_field_style(style_);
    const DrawablePtr background = focused_ ? style.focused : style.normal;
    if (background) background->draw(painter, bounds_of(*this, x, y));
    const float text_y = y + std::max(
        0.0F,
        (height() - painter.measure_text("M", style.font.get()).height) * 0.5F
    );
    painter.draw_text(
        text_, x + style.horizontal_padding, text_y, style.font.get(), style.text
    );
    if (focused_) {
        const Size prefix = painter.measure_text(
            std::string_view(text_).substr(0, cursor_), style.font.get()
        );
        if (!composition_.empty()) {
            const float composition_x = x + style.horizontal_padding + prefix.width;
            painter.draw_text(
                composition_, composition_x, text_y, style.font.get(), style.cursor
            );
            const Size composition_size = painter.measure_text(
                composition_, style.font.get()
            );
            painter.fill_rectangle(
                {composition_x, text_y + composition_size.height - 1.0F,
                 composition_size.width, 1.0F},
                style.cursor
            );
        }
        painter.fill_rectangle(
            {x + style.horizontal_padding + prefix.width,
             text_y, 1.0F, prefix.height},
            style.cursor
        );
    }
}

bool TextField::pointer_event(const PointerEvent& event)
{
    if (!enabled() || event.action != PointerAction::down) return false;
    cursor_ = text_.size();
    return contains(event.x, event.y);
}

bool TextField::key_down(Key key, KeyModifiers)
{
    if (!enabled()) return false;
    switch (key) {
    case Key::left: cursor_ = previous_codepoint(text_, cursor_); return true;
    case Key::right: cursor_ = next_codepoint(text_, cursor_); return true;
    case Key::home: cursor_ = 0; return true;
    case Key::end: cursor_ = text_.size(); return true;
    case Key::backspace:
        if (cursor_ > 0) {
            const std::size_t previous = previous_codepoint(text_, cursor_);
            text_.erase(previous, cursor_ - previous);
            cursor_ = previous;
        }
        invalidate_layout();
        return true;
    case Key::delete_key:
        if (cursor_ < text_.size()) {
            text_.erase(cursor_, next_codepoint(text_, cursor_) - cursor_);
        }
        invalidate_layout();
        return true;
    default: return false;
    }
}

bool TextField::text_input(std::string_view text)
{
    if (!enabled() || text.empty()) return false;
    composition_.clear();
    text_.insert(cursor_, text);
    cursor_ += text.size();
    invalidate_layout();
    return true;
}

bool TextField::text_editing(std::string_view text, int, int)
{
    if (!enabled()) return false;
    composition_.assign(text);
    return true;
}

void TextField::focus_changed(bool focused)
{
    focused_ = focused;
    if (!focused) composition_.clear();
}

bool TextField::focusable() const noexcept { return enabled(); }

} // namespace sq::gui
