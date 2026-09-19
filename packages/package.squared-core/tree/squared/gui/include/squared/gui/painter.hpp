#pragma once

#include <squared/graphics/color.hpp>
#include <squared/gui/size.hpp>

#include <string_view>

namespace sq::graphics2d {
class TextureRegion;
} // namespace sq::graphics2d

namespace sq::gui {

class FontResource;
struct Rectangle;

/**
 * @brief Portable drawing boundary implemented over Squared Graphics2D.
 *
 * All coordinate systems are logical pixels in the same space used for widget
 * placement. The painter is owned by the UI framework and is only valid within
 * the measure/layout/paint calls that pass it.
 */
class Painter {
public:
    virtual ~Painter() = default;

    /**
     * @brief Measure rendered text.
     * @param text UTF-8 text to measure.
     * @return Extent the text occupies when drawn with the current font.
     */
    [[nodiscard]] virtual Size measure_text(std::string_view text) = 0;

    /**
     * @brief Measure text using a style-selected font when it is resolved.
     * @param text UTF-8 text to measure.
     * @param font Borrowed font resource, or null for the painter default.
     * @return Extent in logical units. The base implementation uses
     * GlyphLayout for resolved resources and otherwise calls measure_text(text).
     */
    [[nodiscard]] virtual Size measure_text(
        std::string_view text,
        const FontResource* font
    );

    /**
     * @brief Fill a rectangle with a solid color.
     * @param rectangle Area to fill in logical units.
     * @param color Normalized fill color.
     */
    virtual void fill_rectangle(
        const Rectangle& rectangle,
        graphics::Color color
    ) = 0;

    /**
     * @brief Stroke a rectangle border.
     * @param rectangle Centerline of the border in logical units.
     * @param color Normalized stroke color.
     * @param thickness Border width in logical units; non-negative.
     */
    virtual void stroke_rectangle(
        const Rectangle& rectangle,
        graphics::Color color,
        float thickness
    ) = 0;

    /**
     * @brief Draw a texture region scaled to a rectangle.
     * @param region Region to sample; must outlive the call.
     * @param rectangle Target rectangle in logical units.
     * @param color Tint multiplied with the sampled texture.
     */
    virtual void draw_region(
        const graphics2d::TextureRegion& region,
        const Rectangle& rectangle,
        graphics::Color color = graphics::Color::white()
    ) = 0;

    /**
     * @brief Draw single-line text.
     * @param text UTF-8 text to render.
     * @param x Left baseline position in logical units.
     * @param y Top baseline position in logical units.
     * @param color Normalized text color.
     */
    virtual void draw_text(
        std::string_view text,
        float x,
        float y,
        graphics::Color color
    ) = 0;

    /**
     * @brief Draw single-line text with a style-selected font.
     * @param text UTF-8 text to render.
     * @param x Left position in logical units.
     * @param y Top position in logical units.
     * @param font Borrowed font resource, or null for the painter default.
     * @param color Normalized text color.
     * @note The base implementation delegates to the font-agnostic overload;
     * renderers supporting bitmap pages override this method.
     */
    virtual void draw_text(
        std::string_view text,
        float x,
        float y,
        const FontResource* font,
        graphics::Color color
    );

    /**
     * @brief Push a clipping rectangle.
     * @param rectangle Clip region in logical units; nested clips intersect.
     */
    virtual void push_clip(const Rectangle& rectangle) = 0;

    /** @brief Pop the most recently pushed clipping rectangle. */
    virtual void pop_clip() = 0;
};

} // namespace sq::gui
