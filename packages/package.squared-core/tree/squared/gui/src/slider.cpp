#include <squared/gui/slider.hpp>

#include "detail/gui_detail.hpp"
#include <squared/gui/key.hpp>
#include <squared/gui/key_modifiers.hpp>
#include <squared/gui/painter.hpp>
#include <squared/gui/pointer_action.hpp>
#include <squared/gui/pointer_event.hpp>
#include <squared/gui/rectangle.hpp>
#include <squared/gui/size.hpp>
#include <squared/gui/skin.hpp>

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

namespace sq::gui {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

Slider::Slider(float minimum, float maximum, float value)
{
    set_range(minimum, maximum);
    value_ = std::clamp(value, minimum_, maximum_);
}

void Slider::set_range(float minimum, float maximum) noexcept
{
    minimum_ = std::min(minimum, maximum);
    maximum_ = std::max(minimum, maximum);
    value_ = std::clamp(value_, minimum_, maximum_);
}

void Slider::set_value(float value)
{
    float adjusted = std::clamp(value, minimum_, maximum_);
    if (step_ > 0.0F) {
        adjusted = minimum_ + std::round((adjusted - minimum_) / step_) * step_;
        adjusted = std::clamp(adjusted, minimum_, maximum_);
    }
    if (adjusted == value_) return;
    value_ = adjusted;
    if (callback_) callback_(value_);
}

void Slider::set_step(float step) noexcept { step_ = std::max(0.0F, step); }

void Slider::set_on_change(ChangeCallback callback) { callback_ = std::move(callback); }

void Slider::set_style(std::string style) { style_ = std::move(style); }

Size Slider::minimum_size(Painter&, const Skin& skin) const
{
    const auto& style = skin.slider_style(style_);
    return {style.minimum_length, std::max(style.minimum_touch_size, skin.minimum_touch_size)};
}

Size Slider::preferred_size(Painter&, const Skin&) const
{
    return {120.0F, default_touch_size};
}

void Slider::paint(Painter& painter, const Skin& skin, float x, float y) const
{
    const auto& style = skin.slider_style(style_);
    const float ratio = maximum_ > minimum_
        ? (value_ - minimum_) / (maximum_ - minimum_) : 0.0F;
    const float track_height = std::min(8.0F, height());
    const Rectangle track{x, y + (height() - track_height) * 0.5F, width(), track_height};
    if (style.track) style.track->draw(painter, track);
    if (style.filled_track) {
        style.filled_track->draw(painter, {track.x, track.y, track.width * ratio, track.height});
    }
    const float knob_size = std::min(height(), style.minimum_touch_size * 0.6F);
    if (style.knob) {
        style.knob->draw(
            painter,
            {x + ratio * std::max(0.0F, width() - knob_size),
             y + (height() - knob_size) * 0.5F,
             knob_size,
             knob_size}
        );
    }
    if (focused_) {
        painter.stroke_rectangle(bounds_of(*this, x, y), skin.accent, 2.0F);
    }
}

bool Slider::pointer_event(const PointerEvent& event)
{
    if (!enabled()) return false;
    if (event.action == PointerAction::down && contains(event.x, event.y)) {
        drag_pointer_ = event.pointer_id;
        update_from_pointer(event.x);
        return true;
    }
    if (!drag_pointer_ || *drag_pointer_ != event.pointer_id) return false;
    if (event.action == PointerAction::move) {
        update_from_pointer(event.x);
        return true;
    }
    drag_pointer_.reset();
    if (event.action == PointerAction::up) update_from_pointer(event.x);
    return true;
}

bool Slider::key_down(Key key, KeyModifiers)
{
    if (!enabled() || (key != Key::left && key != Key::right)) return false;
    const float amount = step_ > 0.0F
        ? step_
        : std::max(0.01F, (maximum_ - minimum_) * 0.05F);
    set_value(value_ + (key == Key::left ? -amount : amount));
    return true;
}

bool Slider::focusable() const noexcept { return enabled(); }

void Slider::focus_changed(bool focused) { focused_ = focused; }

void Slider::update_from_pointer(float x)
{
    const float ratio = width() > 0.0F ? std::clamp(x / width(), 0.0F, 1.0F) : 0.0F;
    set_value(minimum_ + ratio * (maximum_ - minimum_));
}

} // namespace sq::gui
