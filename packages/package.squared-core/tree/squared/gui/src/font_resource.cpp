#include <squared/gui/font_resource.hpp>

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
    const bool valid_pages = std::all_of(
        pages_.begin(), pages_.end(), [this](const auto& page) {
            return page.width() >= font_->page_width() &&
                   page.height() >= font_->page_height();
        }
    );
    if (!valid_pages) {
        throw std::invalid_argument("FontResource page is smaller than BMFont metrics");
    }
}

graphics2d::TextureRegion FontResource::glyph_region(
    const graphics2d::GlyphPlacement& glyph
) const noexcept
{
    if (!resolved() || glyph.page >= pages_.size()) return {};
    return pages_[glyph.page].subregion(
        glyph.source_x,
        glyph.source_y,
        glyph.source_width,
        glyph.source_height
    );
}

} // namespace sq::gui
