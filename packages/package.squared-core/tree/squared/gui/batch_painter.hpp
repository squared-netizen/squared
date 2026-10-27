#pragma once

#include <squared/graphics/color.hpp>
#include <squared/graphics/glyph_layout.hpp>
#include <squared/graphics/texture.hpp>
#include <squared/graphics/texture_region.hpp>
#include <squared/gui/src/painter.hpp>
#include <squared/gui/src/rectangle.hpp>
#include <squared/gui/src/size.hpp>

#include <array>
#include <cstddef>
#include <string_view>

namespace sq::graphics
{
class SpriteBatch;
} // namespace sq::graphics

namespace sq::gui
{

class FontResource;

/**
 * @brief A Painter that draws through a SpriteBatch.
 *
 * The concrete end of the widget set: everything a widget draws becomes a quad
 * in a batch. Owns neither the batch nor the font; both must outlive it.
 *
 * Call set_viewport() before the first frame, and again on resize. Clipping
 * needs both sizes: the GUI works in logical units with a top-left origin, and
 * a scissor rectangle is framebuffer pixels measured from the bottom.
 */
class BatchPainter final : public Painter
{
    public:
        /**
     * @brief Bind to a batch and a default font.
     * @param batch Batch to draw into; must outlive this painter.
     * @param default_font Font used by the text calls that take none; may be
     * null, in which case those calls draw nothing and measure to zero.
     */
        BatchPainter( graphics::SpriteBatch &batch, const FontResource *default_font ) noexcept;

        ~BatchPainter() override;

        BatchPainter( const BatchPainter & ) = delete;
        BatchPainter &operator=( const BatchPainter & ) = delete;

        /**
     * @brief Supply the texel that solid fills are drawn from.
     * @param white A fully white, fully opaque region, or null.
     * @return true when a fill source is available.
     *
     * @note Pass the skin's `white` region. Fills then come from the same
     * atlas page as every other widget graphic, so a whole interface can be
     * one draw call. A painter with its own texture instead forces a texture
     * switch - and therefore a draw call - at every fill.
     *
     * @note With null, a 1x1 white texture is created and owned here. That
     * works, and costs a draw call per fill. It is the fallback, not the
     * intent.
     *
     * @note The region is referenced, not copied, so it follows the atlas
     * through a restore. A copy goes stale the moment the atlas reloads a
     * page - every fill is then silently skipped - and the caller would have
     * to know to set it again. Keeping the pointer removes that rule; the
     * region must simply outlive the painter, which an atlas region does.
     */
        [[nodiscard]] bool set_fill_source( const graphics::TextureRegion *white ) noexcept;

        /**
     * @brief Tell the painter how logical units map to pixels.
     * @param logical_width Width the GUI lays out in.
     * @param logical_height Height the GUI lays out in.
     * @param pixel_width Drawable width, from the graphics context.
     * @param pixel_height Drawable height.
     *
     * @note Only clipping needs this. Everything else is in logical units and
     * the camera handles it.
     */
        void set_viewport( float logical_width, float logical_height, int pixel_width,
                           int pixel_height ) noexcept;

        [[nodiscard]] Size measure_text( std::string_view text ) override;

        [[nodiscard]] Size measure_text( std::string_view text, const FontResource *font ) override;

        void fill_rectangle( const Rectangle &rectangle, graphics::Color color ) override;

        void stroke_rectangle( const Rectangle &rectangle, graphics::Color color,
                               float thickness ) override;

        void draw_region( const graphics::TextureRegion &region, const Rectangle &rectangle,
                          graphics::Color color = graphics::Color::white() ) override;

        void draw_text( std::string_view text, float x, float y, graphics::Color color ) override;

        void draw_text( std::string_view text, float x, float y, const FontResource *font,
                        graphics::Color color ) override;

        void push_clip( const Rectangle &rectangle ) override;

        void pop_clip() override;

        /** @brief Report how deeply clips are currently nested. */
        [[nodiscard]] std::size_t clip_depth() const noexcept { return depth_; }

    private:
        /**
     * @brief Deepest clip nesting that is tracked exactly.
     *
     * A fixed array rather than a vector: push_clip and pop_clip run per frame,
     * and a container that could allocate in the frame loop is the thing the
     * memory rules exist to prevent. Sixteen is far past any real widget tree;
     * beyond it the parent's clip stays in force, which overdraws rather than
     * clipping wrongly.
     */
        static constexpr std::size_t k_maximum_clip_depth = 16;

        /// Room the scratch layout is made with: a text of up to this many
        /// glyphs, on up to this many lines, lays out with no allocation.
        static constexpr std::size_t k_reserved_glyphs = 64;
        static constexpr std::size_t k_reserved_lines = 4;

        void apply_current_clip() noexcept;

        /// The single texel a solid fill is stretched from. See fill_rectangle.
        [[nodiscard]] graphics::TextureRegion centre_texel() const noexcept;

        graphics::SpriteBatch *batch_;
        const FontResource *default_font_;
        /**
     * @brief Scratch layout shared by measure_text and draw_text.
     *
     * Kept between calls so that laying text out reuses its storage: it grows
     * to the longest text drawn and then stops allocating, where a layout
     * made per call allocated twice for every label, every frame. Made with
     * room for k_reserved_glyphs, so a value that grows as it comes (a
     * counter's 9 to 10, a row's "number 10") grows nothing in a frame.
     */
        graphics::GlyphLayout text_layout_;
        const graphics::TextureRegion *fill_{ nullptr };
        graphics::Texture owned_white_;
        graphics::TextureRegion owned_fill_;
        std::array<Rectangle, k_maximum_clip_depth> clips_{};
        std::size_t depth_{ 0 };
        float logical_width_{ 0.0F };
        float logical_height_{ 0.0F };
        int pixel_width_{ 0 };
        int pixel_height_{ 0 };
};

} // namespace sq::gui
