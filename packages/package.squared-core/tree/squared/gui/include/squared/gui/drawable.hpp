#pragma once

#include <squared/graphics/color.hpp>
#include <squared/gui/insets.hpp>
#include <squared/gui/size.hpp>

namespace sq::gui {

class Painter;
struct Rectangle;

/**
 * @brief Skin image abstraction; it never exposes SDL or another backend type.
 *
 * Drawables are immutable and shared through DrawablePtr. Any referenced
 * Texture or TextureAtlas must outlive every use of the drawable.
 */
class Drawable {
public:
    virtual ~Drawable() = default;

    /**
     * @brief Report the smallest acceptable size.
     * @return Minimum extent in logical units.
     */
    [[nodiscard]] virtual Size minimum_size() const noexcept = 0;

    /**
     * @brief Report the region reserved for content.
     * @return Insets between the outer box and drawable content.
     */
    [[nodiscard]] virtual Insets content_insets() const noexcept = 0;

    /**
     * @brief Draw the image into a rectangle.
     * @param painter Destination painter; valid for the call.
     * @param rectangle Target rectangle in logical units.
     * @param tint Color multiplied with the drawable's own coloring.
     */
    virtual void draw(
        Painter& painter,
        const Rectangle& rectangle,
        graphics::Color tint = graphics::Color::white()
    ) const = 0;
};

} // namespace sq::gui
