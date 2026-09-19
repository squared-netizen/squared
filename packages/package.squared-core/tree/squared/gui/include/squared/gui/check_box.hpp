#pragma once

#include <squared/gui/size.hpp>
#include <squared/gui/toggle_button.hpp>

#include <string>

namespace sq::gui {

class Painter;
class Skin;

/** @brief Toggle button rendering a checkbox glyph and label. */
class CheckBox : public ToggleButton {
public:
    /**
     * @brief Construct a checkbox.
     * @param text Optional label text.
     * @param checked Initial checked state.
     */
    explicit CheckBox(std::string text = {}, bool checked = false);

    /**
     * @brief Change the applied check-box style.
     * @param style Named check-box style to apply.
     */
    void set_check_style(std::string style);

    /**
     * @brief Report the touch-target-driven minimum size.
     * @note Parameters match Widget::minimum_size: an active painter and the
     * skin providing the check-box style.
     * @return Minimum extent in logical units.
     */
    [[nodiscard]] Size minimum_size(Painter&, const Skin&) const override;

    /**
     * @brief Report the box-plus-label size.
     * @param painter Active painter used for measurement.
     * @param skin Skin providing the selected check-box style and font.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Paint the checkbox glyph and label.
     * @note Parameters match the Widget::paint signature: the active painter,
     * the skin providing the check-box style, and the stage-space origin.
     */
    void paint(Painter&, const Skin&, float, float) const override;

private:
    std::string check_style_{"default"};
};

} // namespace sq::gui
