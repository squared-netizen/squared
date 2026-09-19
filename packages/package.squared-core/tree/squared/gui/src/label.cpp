#include <squared/gui/label.hpp>

#include "detail/gui_detail.hpp"
#include <squared/gui/painter.hpp>
#include <squared/gui/size.hpp>
#include <squared/gui/skin.hpp>

#include <string>
#include <utility>

namespace sq::gui {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

Label::Label(std::string text) : text_(std::move(text))
{
    set_touchable(false);
}

void Label::set_text(std::string text)
{
    text_ = std::move(text);
    invalidate_layout();
}

void Label::set_style(std::string style)
{
    style_ = std::move(style);
    invalidate_layout();
}

Size Label::preferred_size(Painter& painter, const Skin& skin) const
{
    const auto& style = skin.label_style(style_);
    return painter.measure_text(text_, style.font.get());
}

void Label::paint(
    Painter& painter,
    const Skin& skin,
    float x,
    float y
) const
{
    const auto& style = skin.label_style(style_);
    painter.draw_text(
        text_, x, y, style.font.get(),
        muted_ ? style.muted_text.value_or(skin.muted_text)
               : style.text.value_or(skin.text)
    );
}

} // namespace sq::gui
