#include <squared/gui/skin.hpp>

#include "detail/gui_detail.hpp"
#include <squared/graphics/color.hpp>
#include <squared/graphics2d/atlas_region.hpp>
#include <squared/graphics2d/texture_region.hpp>
#include <squared/gui/button_style.hpp>
#include <squared/gui/check_box_style.hpp>
#include <squared/gui/color_drawable.hpp>
#include <squared/gui/drawable_ptr.hpp>
#include <squared/gui/font_ptr.hpp>
#include <squared/gui/insets.hpp>
#include <squared/gui/label_style.hpp>
#include <squared/gui/nine_patch_drawable.hpp>
#include <squared/gui/nine_patch_splits.hpp>
#include <squared/gui/panel_style.hpp>
#include <squared/gui/progress_bar_style.hpp>
#include <squared/gui/region_drawable.hpp>
#include <squared/gui/size.hpp>
#include <squared/gui/slider_style.hpp>
#include <squared/gui/text_field_style.hpp>
#include <squared/gui/window_style.hpp>

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace sq::gui {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

Skin::Skin()
{
    const auto panel = std::make_shared<ColorDrawable>(surface);
    const auto tooltip = std::make_shared<ColorDrawable>(
        graphics::Color::from_rgba8(24, 27, 34, 245)
    );
    const auto normal = std::make_shared<ColorDrawable>(
        control, Size{0.0F, minimum_touch_size}, Insets{12, 8, 12, 8}
    );
    const auto hover = std::make_shared<ColorDrawable>(
        control_hover, Size{0.0F, minimum_touch_size}, Insets{12, 8, 12, 8}
    );
    const auto active = std::make_shared<ColorDrawable>(
        accent, Size{0.0F, minimum_touch_size}, Insets{12, 8, 12, 8}
    );
    const auto disabled = std::make_shared<ColorDrawable>(
        graphics::Color::from_rgba8(45, 48, 56),
        Size{0.0F, minimum_touch_size},
        Insets{12, 8, 12, 8}
    );

    add_drawable("panel", panel);
    add_drawable("tooltip", tooltip);
    add_drawable("control", normal);
    add_drawable("control.hover", hover);
    add_drawable("control.active", active);
    add_drawable("control.disabled", disabled);
    add_panel_style("default", {panel});
    add_panel_style("tooltip", {tooltip});
    add_label_style("default", {});
    add_button_style("default", {
        normal, hover, active, disabled, text, muted_text,
        minimum_touch_size, 12.0F, nullptr
    });
    add_text_field_style("default", {
        normal, active, text, accent, minimum_touch_size, 10.0F, nullptr
    });
    add_check_box_style("default", {
        normal, active, disabled, text, 8.0F, minimum_touch_size, nullptr
    });
    add_slider_style("default", {
        normal, active, active, 120.0F, minimum_touch_size
    });
    add_progress_bar_style("default", {
        normal, active, 120.0F, 12.0F
    });
    add_window_style("default", {
        panel, normal, normal, hover, active, text, text,
        graphics::Color::from_rgba8(0, 0, 0, 140),
        Insets{8.0F, 8.0F, 8.0F, 8.0F}, 36.0F, 28.0F, 8.0F, nullptr
    });
}

void Skin::add_drawable(std::string name, DrawablePtr drawable)
{
    if (name.empty() || !drawable) {
        throw std::invalid_argument("Skin drawable requires a name and value");
    }
    drawables_[std::move(name)] = std::move(drawable);
}

void Skin::add_region_drawable(
    std::string name,
    const graphics2d::TextureRegion& region,
    Insets insets
)
{
    add_drawable(
        std::move(name),
        std::make_shared<RegionDrawable>(region, insets)
    );
}

void Skin::add_nine_patch_drawable(
    std::string name,
    const graphics2d::TextureRegion& region,
    NinePatchSplits splits,
    std::optional<Insets> content_insets
)
{
    add_drawable(
        std::move(name),
        std::make_shared<NinePatchDrawable>(
            region, splits, content_insets
        )
    );
}

void Skin::add_nine_patch_drawable(
    std::string name,
    const graphics2d::AtlasRegion& region
)
{
    if (!region.splits()) {
        throw std::invalid_argument("Atlas region has no nine-patch splits");
    }
    const auto& split = *region.splits();
    std::optional<Insets> insets;
    if (region.pads()) {
        const auto& pad = *region.pads();
        insets = Insets{
            static_cast<float>(pad[0]), static_cast<float>(pad[2]),
            static_cast<float>(pad[1]), static_cast<float>(pad[3])
        };
    }
    add_nine_patch_drawable(
        std::move(name), region.region(),
        NinePatchSplits{split[0], split[2], split[1], split[3]},
        insets
    );
}

DrawablePtr Skin::drawable(std::string_view name) const noexcept
{
    const auto found = drawables_.find(std::string(name));
    return found == drawables_.end() ? DrawablePtr{} : found->second;
}

void Skin::add_font(std::string name, FontPtr font)
{
    if (name.empty() || !font) {
        throw std::invalid_argument("Skin font requires a name and value");
    }
    fonts_[std::move(name)] = std::move(font);
}

FontPtr Skin::font(std::string_view name) const noexcept
{
    const auto found = fonts_.find(std::string(name));
    return found == fonts_.end() ? FontPtr{} : found->second;
}

void Skin::add_panel_style(std::string name, PanelStyle style)
{
    panel_styles_[std::move(name)] = std::move(style);
}

void Skin::add_label_style(std::string name, LabelStyle style)
{
    label_styles_[std::move(name)] = std::move(style);
}

void Skin::add_button_style(std::string name, ButtonStyle style)
{
    button_styles_[std::move(name)] = std::move(style);
}

void Skin::add_text_field_style(std::string name, TextFieldStyle style)
{
    text_field_styles_[std::move(name)] = std::move(style);
}

void Skin::add_check_box_style(std::string name, CheckBoxStyle style)
{
    check_box_styles_[std::move(name)] = std::move(style);
}

void Skin::add_slider_style(std::string name, SliderStyle style)
{
    slider_styles_[std::move(name)] = std::move(style);
}

void Skin::add_progress_bar_style(std::string name, ProgressBarStyle style)
{
    progress_bar_styles_[std::move(name)] = std::move(style);
}

void Skin::add_window_style(std::string name, WindowStyle style)
{
    window_styles_[std::move(name)] = std::move(style);
}

const PanelStyle& Skin::panel_style(std::string_view name) const
{
    return style_or_default(panel_styles_, name);
}

const LabelStyle& Skin::label_style(std::string_view name) const
{
    return style_or_default(label_styles_, name);
}

const ButtonStyle& Skin::button_style(std::string_view name) const
{
    return style_or_default(button_styles_, name);
}

const TextFieldStyle& Skin::text_field_style(std::string_view name) const
{
    return style_or_default(text_field_styles_, name);
}

const CheckBoxStyle& Skin::check_box_style(std::string_view name) const
{
    return style_or_default(check_box_styles_, name);
}

const SliderStyle& Skin::slider_style(std::string_view name) const
{
    return style_or_default(slider_styles_, name);
}

const ProgressBarStyle& Skin::progress_bar_style(std::string_view name) const
{
    return style_or_default(progress_bar_styles_, name);
}

const WindowStyle& Skin::window_style(std::string_view name) const
{
    return style_or_default(window_styles_, name);
}

} // namespace sq::gui
