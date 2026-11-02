module sq.gui;

import sq.graphics;
import std;

namespace sq::gui
{

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

Size Painter::measure_text( std::string_view text, const FontResource *font )
{
    if( font == nullptr || !font->resolved() ) {
        return measure_text( text );
    }
    graphics::GlyphLayout layout;
    graphics::GlyphLayoutOptions options;
    options.scale = font->scale();
    if( layout.set_text( *font->bitmap_font(), text, options ) ) {
        return {};
    }
    return { layout.width(), layout.height() };
}

void Painter::draw_text( std::string_view text, float x, float y, const FontResource *,
                         graphics::Color color )
{
    draw_text( text, x, y, color );
}

} // namespace sq::gui
