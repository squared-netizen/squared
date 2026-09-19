#include <squared/gui/button.hpp>

#include "detail/gui_detail.hpp"
#include <squared/graphics/color.hpp>
#include <squared/gui/drawable_ptr.hpp>
#include <squared/gui/font_ptr.hpp>
#include <squared/gui/font_resource.hpp>
#include <squared/gui/key.hpp>
#include <squared/gui/key_modifiers.hpp>
#include <squared/gui/painter.hpp>
#include <squared/gui/pointer_action.hpp>
#include <squared/gui/pointer_event.hpp>
#include <squared/gui/size.hpp>
#include <squared/gui/skin.hpp>

#include <algorithm>
#include <string>
#include <utility>

namespace sq::gui {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

Button::Button(std::string text, Callback callback)
    : text_(std::move(text)), callback_(std::move(callback))
{
}

void Button::set_text(std::string text)
{
    text_ = std::move(text);
    invalidate_layout();
}

void Button::set_on_click(Callback callback)
{
    callback_ = std::move(callback);
}

void Button::set_style(std::string style)
{
    style_ = std::move(style);
    invalidate_layout();
}

void Button::set_icon(DrawablePtr drawable)
{
    icon_drawable_ = std::move(drawable);
    glyph_.clear();
    glyph_font_.reset();
    invalidate_layout();
}

void Button::set_glyph(std::string glyph, FontPtr font)
{
    glyph_ = std::move(glyph);
    glyph_font_ = std::move(font);
    icon_drawable_.reset();
    invalidate_layout();
}

void Button::clear_icon()
{
    icon_drawable_.reset();
    glyph_.clear();
    glyph_font_.reset();
    invalidate_layout();
}

Size Button::minimum_size(Painter& painter, const Skin& skin) const
{
    const auto& style = skin.button_style(style_);
    const Size text = painter.measure_text(text_, style.font.get());
    const bool has_icon = icon_drawable_ || !glyph_.empty();
    const float icon = has_icon ? std::max(0.0F, style.icon_size) : 0.0F;
    const float spacing = has_icon && !text_.empty()
        ? std::max(0.0F, style.icon_spacing) : 0.0F;
    return {
        std::max(
            style.minimum_height,
            text.width + icon + spacing + 2.0F * style.horizontal_padding
        ),
        std::max({skin.minimum_touch_size, style.minimum_height, icon})
    };
}

Size Button::preferred_size(Painter& painter, const Skin& skin) const
{
    const auto& style = skin.button_style(style_);
    const Size text = painter.measure_text(text_, style.font.get());
    const bool has_icon = icon_drawable_ || !glyph_.empty();
    const float icon = has_icon ? std::max(0.0F, style.icon_size) : 0.0F;
    const float spacing = has_icon && !text_.empty()
        ? std::max(0.0F, style.icon_spacing) : 0.0F;
    return {
        std::max(
            style.minimum_height,
            text.width + icon + spacing + 2.0F * style.horizontal_padding
        ),
        std::max({skin.minimum_touch_size, style.minimum_height, icon})
    };
}

void Button::paint(Painter& painter, const Skin& skin, float x, float y) const
{
    const auto& style = skin.button_style(style_);
    DrawablePtr background;
    if (!enabled()) background = style.disabled;
    else if (pressed_ || selected()) background = style.pressed;
    else if (hovered_) background = style.hovered;
    else background = style.normal;
    if (background) background->draw(painter, bounds_of(*this, x, y));
    if (focused_) {
        painter.stroke_rectangle(bounds_of(*this, x, y), skin.accent, 2.0F);
    }
    const FontResource* glyph_font = glyph_font_ ? glyph_font_.get() : style.font.get();
    const Size text_size = painter.measure_text(text_, style.font.get());
    const bool has_icon = icon_drawable_ || !glyph_.empty();
    const float icon_size = has_icon ? std::max(0.0F, style.icon_size) : 0.0F;
    const float spacing = has_icon && !text_.empty()
        ? std::max(0.0F, style.icon_spacing) : 0.0F;
    const float content_width = icon_size + spacing + text_size.width;
    const float content_x = x + std::max(0.0F, (width() - content_width) * 0.5F);
    const graphics::Color content_color = enabled()
        ? style.text : style.disabled_text;
    if (icon_drawable_) {
        icon_drawable_->draw(
            painter,
            {content_x, y + std::max(0.0F, (height() - icon_size) * 0.5F),
             icon_size, icon_size},
            content_color
        );
    } else if (!glyph_.empty()) {
        const Size glyph_size = painter.measure_text(glyph_, glyph_font);
        painter.draw_text(
            glyph_,
            content_x + std::max(0.0F, (icon_size - glyph_size.width) * 0.5F),
            y + std::max(0.0F, (height() - glyph_size.height) * 0.5F),
            glyph_font, content_color
        );
    }
    painter.draw_text(
        text_,
        content_x + icon_size + spacing,
        y + std::max(0.0F, (height() - text_size.height) * 0.5F),
        style.font.get(), content_color
    );
}

bool Button::pointer_event(const PointerEvent& event)
{
    if (!enabled()) return false;
    const bool inside = contains(event.x, event.y);
    if (event.action == PointerAction::move) {
        hovered_ = inside;
        return pressed_;
    }
    if (event.action == PointerAction::down) {
        pressed_ = inside;
        hovered_ = inside;
        return pressed_;
    }
    if (event.action == PointerAction::up) {
        const bool clicked = pressed_ && inside;
        pressed_ = false;
        hovered_ = inside;
        if (clicked) activate();
        return clicked;
    }
    pressed_ = false;
    hovered_ = false;
    return true;
}

bool Button::key_down(Key key, KeyModifiers)
{
    if (!enabled() || (key != Key::enter && key != Key::space)) return false;
    activate();
    return true;
}

bool Button::focusable() const noexcept { return enabled(); }

void Button::focus_changed(bool focused) { focused_ = focused; }

void Button::activate() { if (callback_) callback_(); }

bool Button::selected() const noexcept { return false; }

} // namespace sq::gui
