#include <squared/dev/frame_allocations.hpp>
#include <squared/graphics/bitmap_font.hpp>
#include <squared/graphics/glyph_layout.hpp>
#include <squared/graphics/src/glyph_layout_options.hpp>

#include <cassert>
#include <cstdio>
#include <string_view>

namespace
{

// A two-glyph font with a space and a replacement glyph; enough to lay out
// lines, tabs and kerning.
constexpr std::string_view k_font =
    "info face=\"t\" size=16 bold=0 italic=0 charset=\"\" unicode=1 stretchH=100 smooth=1 "
    "aa=1 padding=0,0,0,0 spacing=1,1\n"
    "common lineHeight=16 base=12 scaleW=64 scaleH=64 pages=1 packed=0\n"
    "page id=0 file=\"t.png\"\n"
    "chars count=4\n"
    "char id=65 x=0 y=0 width=8 height=12 xoffset=0 yoffset=2 xadvance=9 page=0 chnl=15\n"
    "char id=66 x=8 y=0 width=7 height=12 xoffset=1 yoffset=2 xadvance=8 page=0 chnl=15\n"
    "char id=32 x=0 y=0 width=0 height=0 xoffset=0 yoffset=0 xadvance=4 page=0 chnl=15\n"
    "char id=63 x=16 y=0 width=6 height=12 xoffset=0 yoffset=2 xadvance=7 page=0 chnl=15\n"
    "kernings count=1\n"
    "kerning first=65 second=66 amount=-1\n";

/// Lay out text inside a counted frame and return how many allocations it made.
std::uint64_t allocations_for( sq::graphics::GlyphLayout &layout,
                               const sq::graphics::BitmapFont &font, std::string_view text )
{
    sq::dev::frame_begin();
    const bool laid_out = !layout.set_text( font, text );
    const sq::dev::FrameAllocations used = sq::dev::frame_end();
    assert( laid_out );
    static_cast<void>( laid_out );
    return used.count;
}

} // namespace

int main()
{
    sq::graphics::BitmapFont font;
    const bool loaded = !font.load( k_font );
    assert( loaded );
    static_cast<void>( loaded );

    sq::graphics::GlyphLayout layout;

    // The first layout grows the storage; the same text again allocates
    // nothing. This is the per-frame case: a label laid out every frame.
    assert( allocations_for( layout, font, "AB AB" ) > 0 );
    const auto *glyphs = layout.glyphs().data();
    assert( allocations_for( layout, font, "AB AB" ) == 0 );
    assert( allocations_for( layout, font, "BA" ) == 0 );
    assert( layout.glyphs().data() == glyphs );
    assert( layout.glyphs().size() == 2 );

    // clear() keeps the storage too.
    layout.clear();
    assert( layout.glyphs().empty() && layout.lines().empty() && layout.width() == 0.0F );
    assert( allocations_for( layout, font, "ABBA" ) == 0 );

    // Longer text, or more lines, grows it once; after that, nothing.
    assert( allocations_for( layout, font, "AB\nAB\nAB\tAB AB" ) > 0 );
    assert( allocations_for( layout, font, "AB\nAB\nAB\tAB AB" ) == 0 );
    assert( layout.lines().size() == 3 );
    assert( layout.glyphs().size() == 2 + 2 + 2 + 4 + 2 + 1 + 2 );

    // A failed call leaves the previous layout as it was.
    sq::graphics::GlyphLayoutOptions strict;
    strict.reject_invalid_utf8 = true;
    const float width = layout.width();
    const std::size_t count = layout.glyphs().size();
    const sq::graphics::BitmapFontError error = layout.set_text( font, "A\xff", strict );
    assert( error.code == sq::graphics::BitmapFontErrorCode::invalid_utf8 );
    assert( layout.width() == width && layout.glyphs().size() == count );

    // Kerning between A and B still applies: B starts one unit closer.
    assert( !layout.set_text( font, "AB" ) );
    assert( layout.glyphs()[1].x == 9.0F - 1.0F + 1.0F );

    std::printf( "glyph layout: all assertions passed  sizeof(GlyphLayout)=%zu\n",
                 sizeof( sq::graphics::GlyphLayout ) );
    return 0;
}
