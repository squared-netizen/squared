#pragma once

#include <squared/gui/size.hpp>
#include <squared/gui/widget.hpp>

#include <string>

namespace sq::gui {

class Painter;
class Skin;

/** @brief Read-only determinate progress indicator. */
class ProgressBar final : public Widget {
public:
    /**
     * @brief Construct a progress bar.
     * @param minimum Value represented by an empty bar.
     * @param maximum Value represented by a full bar.
     * @param value Initial value; clamped into the ordered range.
     */
    ProgressBar(float minimum = 0.0F, float maximum = 1.0F, float value = 0.0F);

    /**
     * @brief Replace the value range.
     * @param minimum First range endpoint.
     * @param maximum Second range endpoint.
     * @post Endpoints are ordered and the current value is clamped.
     */
    void set_range(float minimum, float maximum) noexcept;

    /** @brief Set and clamp the represented value. */
    void set_value(float value) noexcept;

    /** @brief Return the represented value. */
    [[nodiscard]] float value() const noexcept { return value_; }

    /** @brief Return completion normalized to the closed range `[0, 1]`. */
    [[nodiscard]] float progress() const noexcept;

    /** @brief Select a named ProgressBarStyle from the active Skin. */
    void set_style(std::string style);

    /** @brief Report the selected style's minimum horizontal extent. */
    [[nodiscard]] Size minimum_size(Painter&, const Skin&) const override;

    /** @brief Report the selected style's preferred extent. */
    [[nodiscard]] Size preferred_size(Painter&, const Skin&) const override;

    /** @brief Paint the complete track and completed portion. */
    void paint(Painter&, const Skin&, float, float) const override;

private:
    float minimum_{0.0F};
    float maximum_{1.0F};
    float value_{0.0F};
    std::string style_{"default"};
};

} // namespace sq::gui
