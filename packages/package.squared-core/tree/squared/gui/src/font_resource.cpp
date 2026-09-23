#include <squared/gui/font_resource.hpp>

#include <squared/graphics2d/bitmap_glyph.hpp>

#include "detail/gui_detail.hpp"
#include <squared/graphics2d/bitmap_font.hpp>
#include <squared/graphics2d/glyph_placement.hpp>
#include <squared/graphics2d/texture_region.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sq::gui {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

FontResource::FontResource(std::string descriptor_path)
    : descriptor_path_(std::move(descriptor_path))
{
    if (!valid_font_path(descriptor_path_)) {
        throw std::invalid_argument("FontResource requires a descriptor path");
    }
}

FontResource::FontResource(
    std::string descriptor_path,
    graphics2d::BitmapFont font,
    std::vector<graphics2d::TextureRegion> pages,
    float scale
) : descriptor_path_(std::move(descriptor_path)), font_(std::move(font)),
    pages_(std::move(pages)), scale_(scale)
{
    if (!valid_font_path(descriptor_path_) || !font_->valid() ||
        !std::isfinite(scale_) || scale_ <= 0.0F ||
        pages_.size() != font_->pages().size()) {
        throw std::invalid_argument("FontResource metrics or pages are invalid");
    }
    // Deliberately no check that glyphs fit their page region.
    //
    // The obvious one - region at least as large as the descriptor's
    // scaleW/scaleH - rejects the normal case: a font page packed into an
    // atlas has its empty margin trimmed, so the region is smaller than the
    // nominal page. The stock libGDX skin is a 256x128 page packed as 254x77.
    //
    // The careful one - every glyph inside the region - rejects that same skin
    // too: its glyphs reach 255 pixels across a 254-pixel region, one pixel of
    // overhang that libGDX neither checks nor notices. Refusing to construct a
    // font over one pixel would make squared unable to load the skin its own
    // widget set is designed against.
    //
    // glyph_region() clamps instead, so an overhanging glyph loses a pixel
    // rather than sampling whatever the atlas packed next to it.
}

graphics2d::TextureRegion FontResource::glyph_region(
    const graphics2d::GlyphPlacement& glyph
) const noexcept
{
    if (!resolved() || glyph.page >= pages_.size()) return {};
    const graphics2d::TextureRegion& page = pages_[glyph.page];

    // Clamp to the page: a glyph may overhang its region by a pixel when the
    // page was packed into an atlas, and sampling past the edge would pull in
    // whatever the packer placed beside it.
    const int source_x = std::clamp(glyph.source_x, 0, page.width());
    const int source_y = std::clamp(glyph.source_y, 0, page.height());
    return page.subregion(
        source_x,
        source_y,
        std::min(glyph.source_width, page.width() - source_x),
        std::min(glyph.source_height, page.height() - source_y)
    );
}

} // namespace sq::gui
