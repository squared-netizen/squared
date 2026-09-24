#include <squared/gui/batch_painter.hpp>

#include <squared/graphics/color.hpp>
#include <squared/graphics2d/bitmap_font.hpp>
#include <squared/graphics2d/bitmap_font_error.hpp>
#include <squared/graphics2d/glyph_layout.hpp>
#include <squared/graphics2d/glyph_placement.hpp>
#include <squared/graphics2d/sprite_batch.hpp>
#include <squared/graphics2d/texture_recovery_options.hpp>
#include <squared/graphics2d/texture_region.hpp>
#include <squared/gui/font_resource.hpp>
#include <squared/gui/rectangle.hpp>
#include <squared/gui/size.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sq::gui {

namespace {

/**
 * @brief Lay text out with a font, or report nothing.
 *
 * One place, because measuring and drawing must agree: a widget that measures
 * with one layout and draws with another is laid out wrong by exactly the
 * difference.
 */
bool layout_text(
    const FontResource* font,
    std::string_view text,
    graphics2d::GlyphLayout& layout
) noexcept
{
    layout.clear();
    if (font == nullptr || !font->resolved()) return false;

    const graphics2d::BitmapFont* bitmap = font->bitmap_font();
    if (bitmap == nullptr || !bitmap->valid()) return false;

    graphics2d::BitmapFontError error;
    return layout.set_text(*bitmap, text, error);
}

}  // namespace

BatchPainter::BatchPainter(
    graphics2d::SpriteBatch& batch,
    const FontResource* default_font
) noexcept
    : batch_(&batch)
    , default_font_(default_font)
{
}

BatchPainter::~BatchPainter() = default;

graphics2d::TextureRegion BatchPainter::centre_texel() const noexcept
{
    if (fill_ == nullptr) return {};
    if (fill_->width() <= 1 || fill_->height() <= 1) return *fill_;
    return fill_->subregion(fill_->width() / 2, fill_->height() / 2, 1, 1);
}

bool BatchPainter::set_fill_source(
    const graphics2d::TextureRegion* white
) noexcept
{
    if (white != nullptr && white->valid()) {
        fill_ = white;
        owned_white_.destroy();
        return true;
    }

    // The fallback: a texture of this painter's own. Correct, and a texture
    // switch at every fill.
    const std::uint8_t texel[4] = {255, 255, 255, 255};
    if (!owned_white_.create_rgba(
            1, 1, texel,
            graphics2d::TextureRecoveryOptions::retain_pixels())) {
        return false;
    }
    owned_fill_ = graphics2d::TextureRegion{owned_white_};
    fill_ = &owned_fill_;
    return true;
}

void BatchPainter::set_viewport(
    float logical_width,
    float logical_height,
    int pixel_width,
    int pixel_height
) noexcept
{
    logical_width_ = logical_width;
    logical_height_ = logical_height;
    pixel_width_ = pixel_width;
    pixel_height_ = pixel_height;
}

Size BatchPainter::measure_text(std::string_view text)
{
    return measure_text(text, default_font_);
}

Size BatchPainter::measure_text(
    std::string_view text,
    const FontResource* font
)
{
    graphics2d::GlyphLayout layout;
    if (!layout_text(font, text, layout)) return Size{0.0F, 0.0F};
    return Size{layout.width(), layout.height()};
}

void BatchPainter::fill_rectangle(
    const Rectangle& rectangle,
    graphics::Color color
)
{
    if (fill_ == nullptr) return;
    if (rectangle.width <= 0.0F || rectangle.height <= 0.0F) return;

    // One texel from the middle, not the whole region.
    //
    // A skin's white patch is a few pixels square with linear filtering, so
    // stretching all of it across a widget makes the edge texels blend with
    // whatever the packer placed beside them in the atlas - a solid fill comes
    // out blurred at its borders. Sampling the centre alone cannot reach a
    // neighbour, and for a 1x1 fallback it is the same texel either way.
    //
    // Computed here rather than stored, so it stays a view of the current
    // region: a cached copy would go stale the next time the atlas reloads.
    batch_->draw(centre_texel(), rectangle.x, rectangle.y,
                 rectangle.width, rectangle.height, color);
}

void BatchPainter::stroke_rectangle(
    const Rectangle& rectangle,
    graphics::Color color,
    float thickness
)
{
    if (fill_ == nullptr || thickness <= 0.0F) return;
    if (rectangle.width <= 0.0F || rectangle.height <= 0.0F) return;

    // Four fills rather than a shader: the corners overlap, which only shows
    // with a translucent colour, and a border thick enough for that to matter
    // is not what this is for.
    const float edge = std::min(
        thickness, std::min(rectangle.width, rectangle.height) * 0.5F);

    fill_rectangle({rectangle.x, rectangle.y, rectangle.width, edge}, color);
    fill_rectangle({rectangle.x, rectangle.y + rectangle.height - edge,
                    rectangle.width, edge}, color);
    fill_rectangle({rectangle.x, rectangle.y + edge,
                    edge, rectangle.height - (edge * 2.0F)}, color);
    fill_rectangle({rectangle.x + rectangle.width - edge, rectangle.y + edge,
                    edge, rectangle.height - (edge * 2.0F)}, color);
}

void BatchPainter::draw_region(
    const graphics2d::TextureRegion& region,
    const Rectangle& rectangle,
    graphics::Color color
)
{
    batch_->draw(region, rectangle.x, rectangle.y,
                 rectangle.width, rectangle.height, color);
}

void BatchPainter::draw_text(
    std::string_view text,
    float x,
    float y,
    graphics::Color color
)
{
    draw_text(text, x, y, default_font_, color);
}

void BatchPainter::draw_text(
    std::string_view text,
    float x,
    float y,
    const FontResource* font,
    graphics::Color color
)
{
    graphics2d::GlyphLayout layout;
    if (!layout_text(font, text, layout)) return;

    for (const graphics2d::GlyphPlacement& glyph : layout.glyphs()) {
        const graphics2d::TextureRegion region = font->glyph_region(glyph);
        if (!region.valid()) continue;
        batch_->draw(region, x + glyph.x, y + glyph.y,
                     glyph.width, glyph.height, color);
    }
}

void BatchPainter::push_clip(const Rectangle& rectangle)
{
    if (depth_ < k_maximum_clip_depth) {
        // Nested clips intersect: a child may only narrow what its parent
        // allowed, never widen it.
        Rectangle clip = rectangle;
        if (depth_ > 0) {
            const Rectangle& parent = clips_[depth_ - 1];
            const float left = std::max(clip.x, parent.x);
            const float top = std::max(clip.y, parent.y);
            const float right =
                std::min(clip.x + clip.width, parent.x + parent.width);
            const float bottom =
                std::min(clip.y + clip.height, parent.y + parent.height);
            clip = Rectangle{left, top,
                             std::max(0.0F, right - left),
                             std::max(0.0F, bottom - top)};
        }
        clips_[depth_] = clip;
    }
    ++depth_;
    apply_current_clip();
}

void BatchPainter::pop_clip()
{
    if (depth_ == 0) return;
    --depth_;
    apply_current_clip();
}

void BatchPainter::apply_current_clip() noexcept
{
    if (depth_ == 0) {
        batch_->clear_clip();
        return;
    }
    // Past the tracked depth the parent's clip stays in force: overdrawing is
    // wrong in a way you can see, clipping to the wrong rectangle is wrong in
    // a way you cannot.
    const Rectangle& clip =
        clips_[std::min(depth_, k_maximum_clip_depth) - 1];

    if (logical_width_ <= 0.0F || logical_height_ <= 0.0F
        || pixel_width_ <= 0 || pixel_height_ <= 0) {
        return;
    }

    const float scale_x = static_cast<float>(pixel_width_) / logical_width_;
    const float scale_y = static_cast<float>(pixel_height_) / logical_height_;

    // The GUI measures y downwards from the top; a scissor rectangle measures
    // upwards from the bottom. This is the whole conversion, and getting it
    // wrong clips the mirror image of what was asked for.
    const int left = static_cast<int>(std::lround(clip.x * scale_x));
    const int width = static_cast<int>(std::lround(clip.width * scale_x));
    const int height = static_cast<int>(std::lround(clip.height * scale_y));
    const int top = static_cast<int>(std::lround(clip.y * scale_y));
    const int bottom = pixel_height_ - top - height;

    batch_->set_clip(left, bottom, std::max(0, width), std::max(0, height));
}

}  // namespace sq::gui
