#pragma once

#include <squared/gui/drawable_ptr.hpp>
#include <squared/gui/font_ptr.hpp>
#include <squared/gui/key.hpp>
#include <squared/gui/key_modifiers.hpp>
#include <squared/gui/size.hpp>
#include <squared/gui/widget.hpp>

#include <functional>
#include <string>

namespace sq::gui {

class Painter;
class Skin;
struct PointerEvent;

/** @brief Clickable, focusable control with an optional label and icon. */
class Button : public Widget {
public:
    /** @brief Click action invoked when the button is activated. */
    using Callback = std::function<void()>;

    /**
     * @brief Construct a button.
     * @param text Optional label text.
     * @param callback Click action; copied into the button.
     */
    explicit Button(std::string text = {}, Callback callback = {});

    /**
     * @brief Replace the label text.
     * @param text New label text.
     */
    void set_text(std::string text);

    /**
     * @brief Read the label text.
     * @return Reference to the stored label.
     */
    [[nodiscard]] const std::string& text() const noexcept { return text_; }

    /**
     * @brief Set the click action.
     * @param callback Action copied into the button; may be empty.
     */
    void set_on_click(Callback callback);

    /**
     * @brief Change the applied button style.
     * @param style Named button style to apply.
     */
    void set_style(std::string style);

    /**
     * @brief Display a drawable before the label.
     * @param drawable Shared immutable drawable; empty clears the icon.
     * @post Any previously configured glyph is cleared.
     */
    void set_icon(DrawablePtr drawable);

    /**
     * @brief Display one UTF-8 glyph before the label.
     * @param glyph UTF-8 glyph text; empty clears the icon.
     * @param font Optional immutable font, or empty for the button style font.
     * @post Any previously configured drawable icon is cleared.
     */
    void set_glyph(std::string glyph, FontPtr font = {});

    /** @brief Remove either kind of icon while preserving the label. */
    void clear_icon();

    /**
     * @brief Report the style-driven minimum size.
     * @note Parameters match Widget::minimum_size: an active painter and the
     * skin providing the button style.
     * @return Minimum extent in logical units.
     */
    [[nodiscard]] Size minimum_size(Painter&, const Skin&) const override;

    /**
     * @brief Report the label-plus-padding size.
     * @param painter Active painter used for measurement.
     * @param skin Skin providing the selected button style and font.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Paint the styled button background and label.
     * @note Parameters match the Widget::paint signature: the active painter,
     * the skin providing the button style, and the stage-space origin.
     */
    void paint(Painter&, const Skin&, float, float) const override;

    /**
     * @brief Track press, hover, and release onto the button.
     * @note The parameter matches Widget::pointer_event: a pointer payload in
     * widget-local coordinates.
     * @return true when the event was handled.
     */
    bool pointer_event(const PointerEvent&) override;

    /**
     * @brief Activate on Enter or Space.
     * @note Parameters match the Widget::key_down signature: a portable key
     * name and the modifier state at the time of the event.
     * @return true when the key activated the button.
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
     * @return true; buttons take keyboard focus.
     */
    [[nodiscard]] bool focusable() const noexcept override;

protected:
    /**
     * @brief Perform the button's activation action.
     *
     * Base invokes the click callback. Subclasses override to toggle state.
     */
    virtual void activate();

    /**
     * @brief Report the selected state used for styling.
     * @return true when the button is rendered as selected.
     */
    [[nodiscard]] virtual bool selected() const noexcept;

    /**
     * @brief Read the pressed state.
     * @return true while the pointer presses the button.
     */
    [[nodiscard]] bool pressed() const noexcept { return pressed_; }

    /**
     * @brief Read the hovered state.
     * @return true while the pointer hovers the button.
     */
    [[nodiscard]] bool hovered() const noexcept { return hovered_; }

    /**
     * @brief Read the focused state.
     * @return true while the button owns keyboard focus.
     */
    [[nodiscard]] bool focused() const noexcept { return focused_; }

private:
    std::string text_;
    std::string style_{"default"};
    Callback callback_;
    DrawablePtr icon_drawable_;
    std::string glyph_;
    FontPtr glyph_font_;
    bool pressed_{false};
    bool hovered_{false};
    bool focused_{false};
};

} // namespace sq::gui
