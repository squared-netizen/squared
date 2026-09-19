#pragma once

#include <squared/graphics/color.hpp>
#include <squared/graphics2d/texture_region.hpp>
#include <squared/gui/drawable.hpp>
#include <squared/gui/insets.hpp>
#include <squared/gui/nine_patch_splits.hpp>
#include <squared/gui/size.hpp>

#include <array>
#include <optional>

namespace sq::gui {

class Painter;
struct Rectangle;

/**
 * @brief Scalable drawable that preserves corners and stretches edges and center.
 */
class NinePatchDrawable final : public Drawable {
public:
    /**
     * @brief Construct a nine-patch drawable.
     * @param region Region to sample; must outlive the drawable.
     * @param splits Corner scissor amounts in source pixels.
     * @param content_insets Content insets; defaults to a nine-patch inset
     * derived from the splits.
     */
    NinePatchDrawable(
        const graphics2d::TextureRegion& region,
        NinePatchSplits splits,
        std::optional<Insets> content_insets = std::nullopt
    );

    /**
     * @brief Report the smallest acceptable size.
     * @return Minimum extent that preserves the corner pieces.
     */
    [[nodiscard]] Size minimum_size() const noexcept override;

    /**
     * @brief Report the content insets.
     * @return Effective insets bounding the stretchable content region.
     */
    [[nodiscard]] Insets content_insets() const noexcept override;

    /**
     * @brief Draw the nine scalable pieces into the rectangle.
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
    std::array<graphics2d::TextureRegion, 9> regions_{};
    NinePatchSplits splits_{};
    Insets insets_{};
};

} // namespace sq::gui
