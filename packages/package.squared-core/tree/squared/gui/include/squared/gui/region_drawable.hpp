#pragma once

#include <squared/graphics/color.hpp>
#include <squared/gui/drawable.hpp>
#include <squared/gui/insets.hpp>
#include <squared/gui/size.hpp>

namespace sq::graphics2d {
class TextureRegion;
} // namespace sq::graphics2d

namespace sq::gui {

class Painter;
struct Rectangle;

/**
 * @brief Drawable that samples one texture region.
 *
 * The referenced region must outlive the drawable.
 */
class RegionDrawable final : public Drawable {
public:
    /**
     * @brief Construct a texture-region drawable.
     * @param region Region to sample; must outlive the drawable.
     * @param insets Content insets in logical units.
     */
    explicit RegionDrawable(
        const graphics2d::TextureRegion& region,
        Insets insets = {}
    ) noexcept;

    /**
     * @brief Report the smallest acceptable size.
     * @return Minimum extent set from the region's pixel size.
     */
    [[nodiscard]] Size minimum_size() const noexcept override;

    /**
     * @brief Report the content insets.
     * @return Insets between the outer box and the region content.
     */
    [[nodiscard]] Insets content_insets() const noexcept override;

    /**
     * @brief Draw the region stretched to the rectangle.
     * @param painter Destination painter; valid for the call.
     * @param rectangle Target rectangle in logical units.
     * @param tint Color multiplied with the sampled texture.
     */
    void draw(
        Painter& painter,
        const Rectangle& rectangle,
        graphics::Color tint = graphics::Color::white()
    ) const override;

private:
    const graphics2d::TextureRegion* region_;
    Insets insets_;
};

} // namespace sq::gui
