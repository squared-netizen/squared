#pragma once

#include <squared/gui/key.hpp>
#include <squared/gui/key_modifiers.hpp>
#include <squared/gui/size.hpp>
#include <squared/gui/widget.hpp>

#include <cstddef>
#include <string>
#include <string_view>

namespace sq::gui {

class Painter;
class Skin;
struct PointerEvent;

/**
 * @brief Single-line editable text input with cursor and composition support.
 */
class TextField final : public Widget {
public:
    /**
     * @brief Construct a text field.
     * @param text Initial text content.
     */
    explicit TextField(std::string text = {});

    /**
     * @brief Replace the field text and reset the cursor.
     * @param text New text content.
     */
    void set_text(std::string text);

    /**
     * @brief Read the current text.
     * @return Reference to the stored text.
     */
    [[nodiscard]] const std::string& text() const noexcept { return text_; }

    /**
     * @brief Read the cursor position.
     * @return Byte offset into the UTF-8 text, or text length at the end.
     */
    [[nodiscard]] std::size_t cursor() const noexcept { return cursor_; }

    /**
     * @brief Change the applied text-field style.
     * @param style Named text-field style to apply.
     */
    void set_style(std::string style);

    /**
     * @brief Report the style-driven minimum size.
     * @note Parameters match Widget::minimum_size: an active painter and the
     * skin providing the text-field style.
     * @return Minimum extent in logical units.
     */
    [[nodiscard]] Size minimum_size(Painter&, const Skin&) const override;

    /**
     * @brief Report the text-plus-padding size.
     * @param painter Active painter used for measurement.
     * @param skin Skin providing the selected text-field style and font.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Paint the field background, text, cursor, and composition.
     * @note Parameters match the Widget::paint signature: the active painter,
     * the skin providing the text-field style, and the stage-space origin.
     */
    void paint(Painter&, const Skin&, float, float) const override;

    /**
     * @brief Place the cursor from a pointer event.
     * @note The parameter matches Widget::pointer_event: a pointer payload in
     * widget-local coordinates.
     * @return true when the field handled the event.
     */
    bool pointer_event(const PointerEvent&) override;

    /**
     * @brief Move the cursor and edit the text with keys.
     * @note Parameters match the Widget::key_down signature: a portable key
     * name and the modifier state at the time of the event.
     * @return true when the key was handled.
     */
    bool key_down(Key, KeyModifiers = {}) override;

    /**
     * @brief Insert committed UTF-8 text at the cursor.
     * @note The parameter matches Widget::text_input: committed text to insert.
     * @return true when the text was inserted.
     */
    bool text_input(std::string_view) override;

    /**
     * @brief Update the in-progress composition span.
     * @note Parameters match the Widget::text_editing signature: the current
     * composition text, its start offset, and its length.
     * @return true when the composition was handled.
     */
    bool text_editing(std::string_view, int, int) override;

    /**
     * @brief Track focus to drive the focused style and underline state.
     * @note The parameter matches Widget::focus_changed: true when the widget
     * gained focus.
     */
    void focus_changed(bool) override;

    /**
     * @brief Report focusability.
     * @return true; text fields take keyboard focus.
     */
    [[nodiscard]] bool focusable() const noexcept override;

    /**
     * @brief Request the platform text-input service while focused.
     * @return true; a focused text field always wants text input.
     */
    [[nodiscard]] bool wants_text_input() const noexcept override
    {
        return true;
    }

private:
    std::string text_;
    std::string style_{"default"};
    std::size_t cursor_{0};
    bool focused_{false};
    std::string composition_;
};

} // namespace sq::gui
