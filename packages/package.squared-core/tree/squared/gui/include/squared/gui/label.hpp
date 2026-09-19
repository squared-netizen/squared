#pragma once

#include <squared/gui/size.hpp>
#include <squared/gui/widget.hpp>

#include <string>

namespace sq::gui {

class Painter;
class Skin;

/** @brief Read-only text widget. */
class Label final : public Widget {
public:
    /**
     * @brief Construct a label.
     * @param text Initial text content.
     */
    explicit Label(std::string text = {});

    /**
     * @brief Replace the label text.
     * @param text New text content.
     */
    void set_text(std::string text);

    /**
     * @brief Read the current text.
     * @return Reference to the stored text.
     */
    [[nodiscard]] const std::string& text() const noexcept { return text_; }

    /**
     * @brief Select a named LabelStyle from the active Skin.
     * @param style Style name; an unknown name falls back to `default`.
     */
    void set_style(std::string style);

    /** @brief Return the selected label-style name. */
    [[nodiscard]] const std::string& style() const noexcept { return style_; }

    /**
     * @brief Control the muted visual style.
     * @param muted true renders with the muted text color.
     */
    void set_muted(bool muted) noexcept { muted_ = muted; }

    /**
     * @brief Report the measured text size.
     * @param painter Active painter used for measurement.
     * @param skin Skin providing the selected label style and font.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Paint the label text.
     * @note Parameters match the Widget::paint signature: the active painter,
     * the skin providing text colors, and the stage-space origin.
     */
    void paint(Painter&, const Skin&, float, float) const override;

private:
    std::string text_;
    std::string style_{"default"};
    bool muted_{false};
};

} // namespace sq::gui
