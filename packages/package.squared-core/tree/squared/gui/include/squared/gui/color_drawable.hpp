#pragma once

#include <squared/graphics/color.hpp>
#include <squared/gui/drawable.hpp>
#include <squared/gui/insets.hpp>
#include <squared/gui/size.hpp>

namespace sq::gui {

class Painter;
struct Rectangle;

/**
 * @brief Drawable that paints a flat, optionally inset color.
 */
class ColorDrawable final : public Drawable {
public:
    /**
     * @brief Construct a flat color drawable.
     * @param color Normalized fill color.
     * @param minimum Smallest acceptable size in logical units.
     * @param insets Content insets in logical units.
     */
    explicit ColorDrawable(
        graphics::Color color,
        Size minimum = {},
        Insets insets = {}
    ) noexcept;

    /**
     * @brief Report the smallest acceptable size.
     * @return Minimum extent in logical units.
     */
    [[nodiscard]] Size minimum_size() const noexcept override;

    /**
     * @brief Report the content insets.
     * @return Insets between the outer box and the flat color.
     */
    [[nodiscard]] Insets content_insets() const noexcept override;

    /**
     * @brief Paint the flat color into the rectangle.
     * @param painter Destination painter; valid for the call.
     * @param rectangle Target rectangle in logical units.
     * @param tint Color multiplied with the stored fill color.
     */
    void draw(
        Painter& painter,
        const Rectangle& rectangle,
        graphics::Color tint = graphics::Color::white()
    ) const override;

private:
    graphics::Color color_;
    Size minimum_;
    Insets insets_;
};

} // namespace sq::gui
