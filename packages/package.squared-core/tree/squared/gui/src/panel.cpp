#include <squared/gui/panel.hpp>

#include "detail/gui_detail.hpp"
#include <squared/gui/painter.hpp>
#include <squared/gui/skin.hpp>

#include <string>
#include <utility>

namespace sq::gui {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

Panel::Panel(std::string style) : style_(std::move(style)) {}

void Panel::set_style(std::string style)
{
    style_ = std::move(style);
    invalidate_layout();
}

void Panel::paint(Painter& painter, const Skin& skin, float x, float y) const
{
    const auto& style = skin.panel_style(style_);
    if (style.background) {
        style.background->draw(painter, bounds_of(*this, x, y));
    }
}

} // namespace sq::gui
