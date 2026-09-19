#pragma once

#include <squared/graphics/color.hpp>
#include <squared/gui/button_style.hpp>
#include <squared/gui/check_box_style.hpp>
#include <squared/gui/drawable_ptr.hpp>
#include <squared/gui/font_ptr.hpp>
#include <squared/gui/insets.hpp>
#include <squared/gui/label_style.hpp>
#include <squared/gui/nine_patch_splits.hpp>
#include <squared/gui/panel_style.hpp>
#include <squared/gui/progress_bar_style.hpp>
#include <squared/gui/slider_style.hpp>
#include <squared/gui/text_field_style.hpp>
#include <squared/gui/window_style.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace sq::graphics2d {
class AtlasRegion;
class TextureRegion;
} // namespace sq::graphics2d

namespace sq::gui {

/**
 * @brief Named, reusable skin resources and widget styles.
 *
 * The skin owns named drawables and style values. Styles are plain value
 * types; drawables are shared via DrawablePtr. A skin is not thread-safe.
 */
class Skin {
public:
    /** @brief Construct a skin with default palette values and default styles. */
    Skin();

    /** @brief Base surface background color. */
    graphics::Color surface{graphics::Color::from_rgba8(28, 31, 38)};
    /** @brief Control background color. */
    graphics::Color control{graphics::Color::from_rgba8(52, 57, 68)};
    /** @brief Control color while hovered. */
    graphics::Color control_hover{graphics::Color::from_rgba8(66, 73, 87)};
    /** @brief Interactive accent/focus color. */
    graphics::Color accent{graphics::Color::from_rgba8(79, 137, 255)};
    /** @brief Primary text color. */
    graphics::Color text{graphics::Color::from_rgba8(238, 241, 247)};
    /** @brief Secondary/muted text color. */
    graphics::Color muted_text{graphics::Color::from_rgba8(174, 181, 194)};
    /** @brief Border color. */
    graphics::Color border{graphics::Color::from_rgba8(91, 99, 116)};
    /** @brief Default padding inside containers, in logical units. */
    float padding{8.0F};
    /** @brief Default spacing between children, in logical units. */
    float spacing{6.0F};
    /** @brief Default border thickness in logical units. */
    float border_width{1.0F};
    /** @brief Minimum touch target size in logical units. */
    float minimum_touch_size{44.0F};

    /**
     * @brief Register a named drawable.
     * @param name Lookup name; replaces any existing drawable with the name.
     * @param drawable Drawable to store; shared ownership.
     */
    void add_drawable(std::string name, DrawablePtr drawable);

    /**
     * @brief Register a named region drawable.
     * @param name Lookup name.
     * @param region Region referenced by the new drawable; must outlive the
     * skin.
     * @param insets Content insets in logical units.
     */
    void add_region_drawable(
        std::string name,
        const graphics2d::TextureRegion& region,
        Insets insets = {}
    );

    /**
     * @brief Register a named nine-patch drawable.
     * @param name Lookup name.
     * @param region Region referenced by the new drawable; must outlive the
     * skin.
     * @param splits Corner scissor amounts in source pixels.
     * @param content_insets Content insets; derived from splits when empty.
     */
    void add_nine_patch_drawable(
        std::string name,
        const graphics2d::TextureRegion& region,
        NinePatchSplits splits,
        std::optional<Insets> content_insets = std::nullopt
    );

    /**
     * @brief Register a named nine-patch drawable from an atlas region.
     * @param name Lookup name.
     * @param region Atlas region supplying texture and splits; must outlive
     * the skin.
     */
    void add_nine_patch_drawable(
        std::string name,
        const graphics2d::AtlasRegion& region
    );

    /**
     * @brief Look up a named drawable.
     * @param name Drawable name to look up.
     * @return Shared drawable, or an empty pointer when the name is absent.
     */
    [[nodiscard]] DrawablePtr drawable(std::string_view name) const noexcept;

    /**
     * @brief Register or replace a named immutable font resource.
     * @param name Lookup name; must be non-empty.
     * @param font Shared font resource; must be non-empty.
     * @throws std::invalid_argument when name or font is empty.
     */
    void add_font(std::string name, FontPtr font);

    /**
     * @brief Look up a named font resource.
     * @param name Font name to look up.
     * @return Shared font, or an empty pointer when absent.
     */
    [[nodiscard]] FontPtr font(std::string_view name) const noexcept;

    /**
     * @brief Register or replace a named panel style.
     * @param name Style name.
     * @param style Style value to store.
     */
    void add_panel_style(std::string name, PanelStyle style);

    /**
     * @brief Register or replace a named label style.
     * @param name Style name.
     * @param style Style value to store.
     */
    void add_label_style(std::string name, LabelStyle style);

    /**
     * @brief Register or replace a named button style.
     * @param name Style name.
     * @param style Style value to store.
     */
    void add_button_style(std::string name, ButtonStyle style);

    /**
     * @brief Register or replace a named text-field style.
     * @param name Style name.
     * @param style Style value to store.
     */
    void add_text_field_style(std::string name, TextFieldStyle style);

    /**
     * @brief Register or replace a named check-box style.
     * @param name Style name.
     * @param style Style value to store.
     */
    void add_check_box_style(std::string name, CheckBoxStyle style);

    /**
     * @brief Register or replace a named slider style.
     * @param name Style name.
     * @param style Style value to store.
     */
    void add_slider_style(std::string name, SliderStyle style);

    /**
     * @brief Register or replace a named progress-bar style.
     * @param name Style name.
     * @param style Style value to store.
     */
    void add_progress_bar_style(std::string name, ProgressBarStyle style);

    /**
     * @brief Register or replace a named window style.
     * @param name Style name.
     * @param style Style value to store.
     */
    void add_window_style(std::string name, WindowStyle style);

    /**
     * @brief Look up a panel style.
     * @param name Style name to look up.
     * @return Named style, or `default` when the name is absent.
     * @throws std::out_of_range only when `default` is also absent.
     */
    [[nodiscard]] const PanelStyle& panel_style(std::string_view name) const;

    /**
     * @brief Look up a label style.
     * @param name Style name to look up.
     * @return Named style, or `default` when the name is absent.
     * @throws std::out_of_range only when `default` is also absent.
     */
    [[nodiscard]] const LabelStyle& label_style(std::string_view name) const;

    /**
     * @brief Look up a button style.
     * @param name Style name to look up.
     * @return Named style, or `default` when the name is absent.
     * @throws std::out_of_range only when `default` is also absent.
     */
    [[nodiscard]] const ButtonStyle& button_style(std::string_view name) const;

    /**
     * @brief Look up a text-field style.
     * @param name Style name to look up.
     * @return Named style, or `default` when the name is absent.
     * @throws std::out_of_range only when `default` is also absent.
     */
    [[nodiscard]] const TextFieldStyle& text_field_style(std::string_view name) const;

    /**
     * @brief Look up a check-box style.
     * @param name Style name to look up.
     * @return Named style, or `default` when the name is absent.
     * @throws std::out_of_range only when `default` is also absent.
     */
    [[nodiscard]] const CheckBoxStyle& check_box_style(std::string_view name) const;

    /**
     * @brief Look up a slider style.
     * @param name Style name to look up.
     * @return Named style, or `default` when the name is absent.
     * @throws std::out_of_range only when `default` is also absent.
     */
    [[nodiscard]] const SliderStyle& slider_style(std::string_view name) const;

    /**
     * @brief Look up a progress-bar style.
     * @param name Style name to look up.
     * @return Named style, or `default` when the name is absent.
     * @throws std::out_of_range only when `default` is also absent.
     */
    [[nodiscard]] const ProgressBarStyle& progress_bar_style(
        std::string_view name
    ) const;

    /**
     * @brief Look up a window style.
     * @param name Style name to look up.
     * @return Named style, or `default` when the name is absent.
     * @throws std::out_of_range only when `default` is also absent.
     */
    [[nodiscard]] const WindowStyle& window_style(std::string_view name) const;

private:
    std::unordered_map<std::string, DrawablePtr> drawables_;
    std::unordered_map<std::string, FontPtr> fonts_;
    std::unordered_map<std::string, PanelStyle> panel_styles_;
    std::unordered_map<std::string, LabelStyle> label_styles_;
    std::unordered_map<std::string, ButtonStyle> button_styles_;
    std::unordered_map<std::string, TextFieldStyle> text_field_styles_;
    std::unordered_map<std::string, CheckBoxStyle> check_box_styles_;
    std::unordered_map<std::string, SliderStyle> slider_styles_;
    std::unordered_map<std::string, ProgressBarStyle> progress_bar_styles_;
    std::unordered_map<std::string, WindowStyle> window_styles_;
};

} // namespace sq::gui
