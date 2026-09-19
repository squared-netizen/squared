#include <squared/gui/check_box.hpp>

#include "detail/gui_detail.hpp"
#include <squared/gui/drawable_ptr.hpp>
#include <squared/gui/painter.hpp>
#include <squared/gui/size.hpp>
#include <squared/gui/skin.hpp>
#include <squared/gui/toggle_button.hpp>

#include <algorithm>
#include <string>
#include <utility>

namespace sq::gui {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

CheckBox::CheckBox(std::string text, bool checked)
    : ToggleButton(std::move(text), checked)
{
}

void CheckBox::set_check_style(std::string style)
{
    check_style_ = std::move(style);
    invalidate_layout();
}

Size CheckBox::minimum_size(Painter& painter, const Skin& skin) const
{
    const auto& style = skin.check_box_style(check_style_);
    const Size text_size = painter.measure_text(text());
    return {
        style.minimum_touch_size + style.spacing + text_size.width,
        std::max(style.minimum_touch_size, text_size.height)
    };
}

Size CheckBox::preferred_size(Painter& painter, const Skin& skin) const
{
    const auto& style = skin.check_box_style(check_style_);
    const Size text_size = painter.measure_text(text(), style.font.get());
    return {
        style.minimum_touch_size + style.spacing + text_size.width,
        std::max(skin.minimum_touch_size, style.minimum_touch_size)
    };
}

void CheckBox::paint(Painter& painter, const Skin& skin, float x, float y) const
{
    const auto& style = skin.check_box_style(check_style_);
    DrawablePtr mark = !enabled()
        ? style.disabled : (checked() ? style.checked : style.unchecked);
    const float size = std::min(height(), style.minimum_touch_size);
    if (mark) {
        mark->draw(painter, {x, y + (height() - size) * 0.5F, size, size});
    }
    const Size text_size = painter.measure_text(text(), style.font.get());
    painter.draw_text(
        text(),
        x + size + style.spacing,
        y + std::max(0.0F, (height() - text_size.height) * 0.5F),
        style.font.get(), style.text
    );
    if (focused()) {
        painter.stroke_rectangle(bounds_of(*this, x, y), skin.accent, 2.0F);
    }
}

} // namespace sq::gui
