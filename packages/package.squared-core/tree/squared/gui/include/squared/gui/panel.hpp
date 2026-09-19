#pragma once

#include <squared/gui/widget.hpp>

#include <string>

namespace sq::gui {

class Painter;
class Skin;

/** @brief Container painting a single style background. */
class Panel final : public Widget {
public:
    /**
     * @brief Construct a panel.
     * @param style Named panel style to apply.
     */
    explicit Panel(std::string style = "default");

    /**
     * @brief Change the applied panel style.
     * @param style Named panel style to apply.
     */
    void set_style(std::string style);

    /**
     * @brief Paint the panel background.
     * @note Parameters match the Widget::paint signature: the active painter,
     * the skin providing the panel style, and the stage-space origin.
     */
    void paint(Painter&, const Skin&, float, float) const override;

private:
    std::string style_;
};

} // namespace sq::gui
