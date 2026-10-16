// What BitmapFont::load and GlyphLayout::set_text return when they fail.
//
// The error is returned, true when something failed, and owns nothing: its
// texts are fixed strings, and the values that used to be glued into them (a
// field name, a byte offset) are fields of their own. This checks each kind of
// failure carries the right category, line, field and byte, that a failure
// leaves the font or layout as it was, and that a failed layout of warm storage
// allocates nothing, which an owned message could not promise.

#include <squared/dev/frame_allocations.hpp>
#include <squared/graphics/bitmap_font.hpp>
#include <squared/graphics/glyph_layout.hpp>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>

using sq::graphics::BitmapFont;
using sq::graphics::BitmapFontError;
using sq::graphics::BitmapFontErrorCode;
using sq::graphics::GlyphLayout;

namespace
{

// Ten lines; the kerning record is line 10.
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

/// The font with the first occurrence of `from` replaced by `to`.
std::string with( std::string_view from, std::string_view to )
{
    std::string text{ k_font };
    const auto at = text.find( from );
    assert( at != std::string::npos );
    text.replace( at, from.size(), to );
    return text;
}

bool same( const char *left, const char *right )
{
    return std::strcmp( left, right ) == 0;
}

} // namespace

int main()
{
    static_assert( std::is_trivially_copyable_v<BitmapFontError> );
    static_assert( sizeof( BitmapFontError ) == ( sizeof( void * ) == 8 ? 32 : 20 ) );

    // An empty error is success, with printable empty texts.
    constexpr BitmapFontError none;
    assert( !none && none.code == BitmapFontErrorCode::none );
    assert( same( none.message, "" ) && same( none.field, "" ) );
    assert( none.line == 0 && none.byte == 0 );

    BitmapFont font;
    assert( !font.load( k_font ) );
    assert( font.valid() );
    const std::size_t glyphs = font.glyphs().size();

    // A missing field names the field and its line.
    {
        const BitmapFontError error = font.load( with( "lineHeight=16 ", "" ) );
        assert( error && error.code == BitmapFontErrorCode::missing_required );
        assert( same( error.message, "missing required field" ) );
        assert( same( error.field, "lineHeight" ) && error.line == 2 && error.byte == 0 );
    }

    // So does a field that is not a number.
    {
        const BitmapFontError error = font.load( with( "size=16", "size=big" ) );
        assert( error.code == BitmapFontErrorCode::malformed );
        assert( same( error.message, "invalid integer field" ) );
        assert( same( error.field, "size" ) && error.line == 1 );
    }

    // An unknown record is found by its line; no field is involved.
    {
        const std::string text = std::string{ k_font } + "bogus a=1\n";
        const BitmapFontError error = font.load( text );
        assert( error.code == BitmapFontErrorCode::unsupported );
        assert( same( error.message, "unsupported BMFont record" ) );
        assert( same( error.field, "" ) && error.line == 11 );
    }

    // Limits that cannot be met are the caller's mistake, about no line.
    {
        sq::graphics::BitmapFontParseLimits limits;
        limits.maximum_glyphs = 0;
        const BitmapFontError error = font.load( k_font, limits );
        assert( error.code == BitmapFontErrorCode::invalid_argument && error.line == 0 );
    }

    // Every failure above left the font as it was.
    assert( font.valid() && font.glyphs().size() == glyphs );

    // Every kind of descriptor failure says something.
    for( const std::string &text :
         { with( "pages=1", "pages=2" ), with( "page id=0", "page id=-1" ),
           with( "chars count=4", "chars count=0" ), with( "char id=66", "char id=65" ),
           with( "info ", "common " ), std::string{ "info face=\"t\"\n" } } ) {
        BitmapFont scratch;
        const BitmapFontError error = scratch.load( text );
        assert( error && error.message[0] != '\0' );
        assert( !scratch.valid() );
    }

    // Layout: a bad byte is reported by its offset, and the first one wins.
    GlyphLayout layout;
    sq::graphics::GlyphLayoutOptions strict;
    strict.reject_invalid_utf8 = true;
    assert( !layout.set_text( font, "AB", strict ) );
    {
        const BitmapFontError error = layout.set_text( font,
                                                       "AB\xff"
                                                       "A\xfe",
                                                       strict );
        assert( error.code == BitmapFontErrorCode::invalid_utf8 );
        assert( same( error.message, "invalid UTF-8" ) && error.byte == 2 && error.line == 0 );
    }
    assert( layout.glyphs().size() == 2 && layout.width() > 0.0F );

    // Failing allocates nothing. The old error built "invalid UTF-8 at byte 2"
    // into an owned string on every call, too long to stay inside it.
    sq::dev::frame_begin();
    const BitmapFontError failed = layout.set_text( font, "AB\xff", strict );
    const sq::dev::FrameAllocations used = sq::dev::frame_end();
    assert( failed && used.count == 0 );

    // Strict is the default. Without it the bad byte becomes the replacement
    // glyph, and nothing fails.
    assert( layout.set_text( font, "A\xff" ).code == BitmapFontErrorCode::invalid_utf8 );
    sq::graphics::GlyphLayoutOptions lenient;
    lenient.reject_invalid_utf8 = false;
    assert( !layout.set_text( font, "A\xff", lenient ) && layout.glyphs().size() == 2 );

    std::printf( "bitmap font errors: all assertions passed  sizeof(BitmapFontError)=%zu\n",
                 sizeof( BitmapFontError ) );
    return 0;
}
