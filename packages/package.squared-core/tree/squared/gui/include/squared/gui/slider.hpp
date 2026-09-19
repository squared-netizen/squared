#pragma once

#include <squared/gui/key.hpp>
#include <squared/gui/key_modifiers.hpp>
#include <squared/gui/size.hpp>
#include <squared/gui/widget.hpp>

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

namespace sq::gui {

class Painter;
class Skin;
struct PointerEvent;

/** @brief Draggable one-axis value selector with an optional gradient fill. */
class Slider final : public Widget {
public:
    /** @brief Action invoked with each new slider value. */
    using ChangeCallback = std::function<void(float)>;

    /**
     * @brief Construct a slider.
     * @param minimum Smallest selectable value.
     * @param maximum Largest selectable value.
     * @param value Initial value; clamped into the range.
     */
    Slider(float minimum = 0.0F, float maximum = 1.0F, float value = 0.0F);

    /**
     * @brief Set the selectable range.
     * @param minimum Smallest selectable value.
     * @param maximum Largest selectable value; with minimum must be ordered.
     */
    void set_range(float minimum, float maximum) noexcept;

    /**
     * @brief Set the current value.
     * @param value New value; clamped into the range and snapped to the step.
     */
    void set_value(float value);

    /**
     * @brief Read the current value.
     * @return Current value in the configured range.
     */
    [[nodiscard]] float value() const noexcept { return value_; }

    /**
     * @brief Set the value granularity.
     * @param step Snap increment in value units; zero disables snapping.
     */
    void set_step(float step) noexcept;

    /**
     * @brief Set the change callback.
     * @param callback Action copied into the slider; invoked with each new
     * value.
     */
    void set_on_change(ChangeCallback callback);

    /**
     * @brief Change the applied slider style.
     * @param style Named slider style to apply.
     */
    void set_style(std::string style);

    /**
     * @brief Report the style-driven minimum size.
     * @note Parameters match Widget::minimum_size: an active painter and the
     * skin providing the slider style.
     * @return Minimum extent in logical units.
     */
    [[nodiscard]] Size minimum_size(Painter&, const Skin&) const override;

    /**
     * @brief Report the track-driven preferred size.
     * @param painter Active painter used for measurement.
     * @param skin Active skin; unused by slider measurement.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Paint the track, gradient fill, and knob.
     * @note Parameters match the Widget::paint signature: the active painter,
     * the skin providing the slider style, and the stage-space origin.
     */
    void paint(Painter&, const Skin&, float, float) const override;

    /**
     * @brief Drag the knob to set the value.
     * @note The parameter matches Widget::pointer_event: a pointer payload in
     * widget-local coordinates.
     * @return true when the event was handled.
     */
    bool pointer_event(const PointerEvent&) override;

    /**
     * @brief Adjust the value with arrow keys.
     * @note Parameters match the Widget::key_down signature: a portable key
     * name and the modifier state at the time of the event.
     * @return true when the key adjusted the value.
     */
    bool key_down(Key, KeyModifiers = {}) override;

    /**
     * @brief Track the focused state.
     * @note The parameter matches Widget::focus_changed: true when the widget
     * gained focus.
     */
    void focus_changed(bool) override;

    /**
     * @brief Report focusability.
     * @return true; sliders take keyboard focus.
     */
    [[nodiscard]] bool focusable() const noexcept override;

private:
    void update_from_pointer(float x);
    float minimum_{0.0F};
    float maximum_{1.0F};
    float value_{0.0F};
    float step_{0.0F};
    std::string style_{"default"};
    ChangeCallback callback_;
    std::optional<std::int64_t> drag_pointer_;
    bool focused_{false};
};

} // namespace sq::gui
