#include <squared/gui/src/painter.hpp>

#include "detail/gui_detail.hpp"
#include <squared/graphics/color.hpp>
#include <squared/graphics/src/bitmap_font_error.hpp>
#include <squared/graphics/glyph_layout.hpp>
#include <squared/graphics/src/glyph_layout_options.hpp>
#include <squared/graphics/texture_region.hpp>
#include <squared/gui/src/font_resource.hpp>
#include <squared/gui/src/rectangle.hpp>
#include <squared/gui/src/size.hpp>

#include <string_view>

namespace sq::gui {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

Size Painter::measure_text(
    std::string_view text,
    const FontResource* font
)
{
    if (font == nullptr || !font->resolved()) return measure_text(text);
    graphics::GlyphLayout layout;
    graphics::GlyphLayoutOptions options;
    options.scale = font->scale();
    graphics::BitmapFontError error;
    if (!layout.set_text(*font->bitmap_font(), text, options, error)) return {};
    return {layout.width(), layout.height()};
}

void Painter::draw_text(
    std::string_view text,
    float x,
    float y,
    const FontResource*,
    graphics::Color color
)
{
    draw_text(text, x, y, color);
}

} // namespace sq::gui
