#include <squared/gui/progress_bar.hpp>

#include "detail/gui_detail.hpp"
#include <squared/gui/painter.hpp>
#include <squared/gui/rectangle.hpp>
#include <squared/gui/size.hpp>
#include <squared/gui/skin.hpp>

#include <algorithm>
#include <string>
#include <utility>

namespace sq::gui {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

ProgressBar::ProgressBar(float minimum, float maximum, float value)
{
    set_touchable(false);
    set_range(minimum, maximum);
    set_value(value);
}

void ProgressBar::set_range(float minimum, float maximum) noexcept
{
    minimum_ = std::min(minimum, maximum);
    maximum_ = std::max(minimum, maximum);
    value_ = std::clamp(value_, minimum_, maximum_);
}

void ProgressBar::set_value(float value) noexcept
{
    value_ = std::clamp(value, minimum_, maximum_);
}

float ProgressBar::progress() const noexcept
{
    return maximum_ > minimum_
        ? std::clamp((value_ - minimum_) / (maximum_ - minimum_), 0.0F, 1.0F)
        : 0.0F;
}

void ProgressBar::set_style(std::string style)
{
    style_ = std::move(style);
    invalidate_layout();
}

Size ProgressBar::minimum_size(Painter&, const Skin& skin) const
{
    const auto& style = skin.progress_bar_style(style_);
    return {
        std::max(0.0F, style.minimum_length),
        std::max(0.0F, style.thickness)
    };
}

Size ProgressBar::preferred_size(Painter& painter, const Skin& skin) const
{
    return minimum_size(painter, skin);
}

void ProgressBar::paint(
    Painter& painter,
    const Skin& skin,
    float x,
    float y
) const
{
    const auto& style = skin.progress_bar_style(style_);
    const float thickness = std::min(
        height(), std::max(0.0F, style.thickness)
    );
    const Rectangle track{
        x, y + std::max(0.0F, (height() - thickness) * 0.5F), width(), thickness
    };
    if (style.track) style.track->draw(painter, track);
    if (style.fill && progress() > 0.0F) {
        style.fill->draw(
            painter,
            {track.x, track.y, track.width * progress(), track.height}
        );
    }
}

} // namespace sq::gui
