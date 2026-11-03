// sq::font (12.3a, 12.3b) on the test font, JetBrains Mono Regular
// (test/data/, OFL; read from the module's directory, where tests run).
// Face: opening from a file and from memory, the metrics at two sizes,
// glyphs rendered (coverage, bearings, advance), bold and italic made from
// the regular face, a character it lacks, a .ttc index it lacks, errors,
// moves. FontSet: the primary and the system's fallbacks (CJK drawn by Noto
// CJK where the machine has it: Android's /system/fonts, Debian's
// fonts-noto-cjk), bold, sizes, a full set. GlyphAtlas: a glyph found the
// second time, its coverage as rendered, the dirty rectangle, a glyph no
// face has, starting again when full or at capacity.

#include <stdio.h>

import sq.font;

import std;

using namespace sq::font;

namespace
{

int failures = 0;

#define CHECK( condition )                                                                         \
    do {                                                                                           \
        if( !( condition ) ) {                                                                     \
            std::fprintf( stderr, "font_test.cpp:%d: %s\n", __LINE__, #condition );                \
            ++failures;                                                                            \
        }                                                                                          \
    } while( false )

constexpr std::string_view font_path = "test/data/JetBrainsMono-Regular.ttf";

std::vector<std::byte> read_file( std::string_view path )
{
    std::ifstream in{ std::string{ path }, std::ios::binary };
    std::vector<std::byte> bytes;
    char c = 0;
    while( in.get( c ) ) {
        bytes.push_back( static_cast<std::byte>( c ) );
    }
    return bytes;
}

// How much of the bitmap is covered, and where: the ink's bounding rows.
struct Ink
{
        std::uint64_t total = 0;
        int first_row = -1;
        int last_row = -1;
};

Ink ink_of( const GlyphBitmap &glyph )
{
    Ink ink;
    for( std::uint16_t y = 0; y < glyph.height; ++y ) {
        std::uint64_t row = 0;
        for( const std::uint8_t value : glyph.row( y ) ) {
            row += value;
        }
        if( row > 0 ) {
            if( ink.first_row < 0 ) {
                ink.first_row = y;
            }
            ink.last_row = y;
        }
        ink.total += row;
    }
    return ink;
}

void opening_and_metrics()
{
    Face face;
    CHECK( !face.is_open() );
    CHECK( face.set_pixel_size( 20 ).code == FaceErrorCode::not_open );
    CHECK( !face.open_file( font_path ) );
    CHECK( face.is_open() );
    CHECK( face.family() == "JetBrains Mono" );
    CHECK( !face.bold() && !face.italic() );
    CHECK( !face.render( U'A' ) ); // no size yet
    CHECK( face.set_pixel_size( 0 ).code == FaceErrorCode::size );

    CHECK( !face.set_pixel_size( 20 ) );
    const FontMetrics small = face.metrics();
    // JetBrains Mono: 600 units wide on a 1000-unit em, ascender 1020,
    // descender -300: at 20 px a 12 px cell, about 27 high.
    CHECK( small.cell_width == 12 );
    CHECK( small.ascender >= 20 && small.ascender <= 22 );
    CHECK( small.descender <= -5 && small.descender >= -7 );
    CHECK( small.cell_height >= small.ascender - small.descender );
    CHECK( small.cell_height <= 28 );
    CHECK( small.underline_thickness >= 1 );

    CHECK( !face.set_pixel_size( 40 ) );
    const FontMetrics large = face.metrics();
    CHECK( large.cell_width == 24 );
    CHECK( large.cell_height >= 2 * small.cell_height - 2 &&
           large.cell_height <= 2 * small.cell_height + 2 );
}

void glyphs()
{
    Face face;
    CHECK( !face.open_file( font_path ) );
    CHECK( !face.set_pixel_size( 32 ) );
    const FontMetrics metrics = face.metrics();

    const auto a = face.render( U'A' );
    CHECK( a.has_value() );
    if( a ) {
        CHECK( a->width > 0 && a->width <= metrics.cell_width + 2 );
        CHECK( a->height > 0 && a->top > 0 && a->top <= metrics.ascender );
        CHECK( a->pitch >= a->width );
        CHECK( a->advance == metrics.cell_width );
        const Ink ink = ink_of( *a );
        CHECK( ink.total > 0 );
        CHECK( ink.first_row == 0 && ink.last_row == a->height - 1 );
    }
    // A capital sits on the baseline; a 'g' reaches below it.
    const auto g = face.render( U'g' );
    CHECK( g.has_value() && g->height > g->top );
    // A space: an advance, no ink.
    const auto space = face.render( U' ' );
    CHECK( space.has_value() && space->advance == metrics.cell_width );
    CHECK( !space || ink_of( *space ).total == 0 );
    // Beyond ASCII: Cyrillic and box drawing are in JetBrains Mono.
    CHECK( face.has_glyph( U'Ж' ) && face.render( U'Ж' ).has_value() );
    CHECK( face.has_glyph( U'─' ) && face.render( U'─' ).has_value() );
    // CJK is not: a fallback face draws it (12.3b).
    CHECK( !face.has_glyph( U'中' ) );
    CHECK( !face.render( U'中' ) );
}

void bold_and_italic()
{
    Face face;
    CHECK( !face.open_file( font_path ) );
    CHECK( !face.set_pixel_size( 32 ) );
    const auto plain = face.render( U'H' );
    CHECK( plain.has_value() );
    const Ink plain_ink = plain ? ink_of( *plain ) : Ink{};
    const std::uint16_t plain_width = plain ? plain->width : 0;

    // Emboldened: more ink, at least as wide.
    const auto bold = face.render( U'H', { .bold = true } );
    CHECK( bold.has_value() );
    if( bold ) {
        CHECK( ink_of( *bold ).total > plain_ink.total + plain_ink.total / 10 );
        CHECK( bold->width >= plain_width );
    }
    // Slanted: wider (the top leans right), about as much ink.
    const auto italic = face.render( U'H', { .italic = true } );
    CHECK( italic.has_value() );
    if( italic ) {
        CHECK( italic->width > plain_width );
        const auto total = static_cast<double>( ink_of( *italic ).total );
        CHECK( total > 0.8 * static_cast<double>( plain_ink.total ) &&
               total < 1.2 * static_cast<double>( plain_ink.total ) );
    }
}

void from_memory_and_errors()
{
    const std::vector<std::byte> file = read_file( font_path );
    CHECK( file.size() > 100000 );
    Face face;
    CHECK( !face.open_memory( file ) );
    CHECK( !face.set_pixel_size( 16 ) );
    CHECK( face.render( U'x' ).has_value() );

    // A second face in a one-face file; not a font; not there.
    Face second;
    CHECK( second.open_memory( file, 1 ).code == FaceErrorCode::no_such_face );
    CHECK( !second.is_open() );
    const std::array<std::byte, 64> junk{};
    Face bad;
    CHECK( bad.open_memory( junk ).code == FaceErrorCode::open );
    CHECK( bad.open_file( "test/data/no-such-font.ttf" ).code == FaceErrorCode::open );
    CHECK(
        ( FaceError{ FaceErrorCode::size, 0 }.what() == "the face cannot be set to that size" ) );

    // Moved: the new one works, the old one is closed.
    Face moved = std::move( face );
    CHECK( moved.is_open() && moved.render( U'y' ).has_value() );
    CHECK( !face.is_open() ); // NOLINT(bugprone-use-after-move)
    Face assigned;
    assigned = std::move( moved );
    CHECK( assigned.render( U'z' ).has_value() );
    // Reopened: the old face goes first.
    CHECK( !assigned.open_file( font_path ) );
    CHECK( assigned.metrics() == FontMetrics{} ); // no size until set
}

Face open_test_face()
{
    Face face;
    CHECK( !face.open_file( font_path ) );
    return face;
}

void font_set()
{
    FontSet fonts;
    CHECK( fonts.set_pixel_size( 20 ).code == FaceErrorCode::not_open );
    CHECK( fonts.add( Face{} ).code == FaceErrorCode::not_open );
    CHECK( !fonts.add( open_test_face() ) );
    CHECK( fonts.size() == 1 && !fonts.has_bold() );
    CHECK( !fonts.set_pixel_size( 20 ) );
    CHECK( fonts.metrics().cell_width == 12 );

    // ASCII from the primary; CJK from nothing yet.
    const auto a = fonts.choose( U'A', false );
    CHECK( a && a->face == 0 && !a->bold_face );
    CHECK( !fonts.choose( U'\u4E2D', false ) );
    // Bold without a bold face: the primary, emboldened.
    const auto bold = fonts.choose( U'A', true );
    CHECK( bold && bold->face == 0 && !bold->bold_face );
    FontSet::Choice chosen{ .face = 9 };
    CHECK( fonts.render( U'A', { .bold = true }, &chosen ).has_value() && chosen.face == 0 );

    // A bold face (the regular one standing in): chosen for bold.
    CHECK( !fonts.set_bold( open_test_face() ) );
    const auto real_bold = fonts.choose( U'A', true );
    CHECK( real_bold && real_bold->bold_face );

    // The system's fallbacks, where this machine has them.
    const std::size_t added = fonts.add_system_fallbacks();
    CHECK( fonts.size() == 1 + added );
    bool has_cjk = false;
    for( const std::string_view path : FontSet::system_fallback_paths() ) {
        std::error_code error;
        if( path.find( "CJK" ) != std::string_view::npos &&
            std::filesystem::is_regular_file( std::filesystem::path{ path }, error ) ) {
            has_cjk = true;
        }
    }
    if( has_cjk ) {
        const auto cjk = fonts.choose( U'\u4E2D', false );
        CHECK( cjk && cjk->face >= 1 );
        // Added after the size was set: at that size too.
        const auto glyph = fonts.render( U'\u4E2D', {} );
        CHECK( glyph && glyph->width > fonts.metrics().cell_width );
        // A CJK glyph is an em wide (20 px here), less than its two cells
        // (24): the drawing centres it in them.
        CHECK( glyph && glyph->advance > fonts.metrics().cell_width );
        CHECK( glyph && glyph->width <= 2 * fonts.metrics().cell_width + 2 );
    } else {
        std::printf( "font_test: no Noto CJK on this machine; the CJK fallback is not checked\n" );
    }

    // Full.
    FontSet many;
    for( std::size_t i = 0; i < FontSet::max_faces; ++i ) {
        CHECK( !many.add( open_test_face() ) );
    }
    CHECK( many.add( open_test_face() ).code == FaceErrorCode::set_full );
}

std::uint64_t sum_of( std::span<const std::uint8_t> pixels, std::uint16_t width,
                      const AtlasGlyph &glyph )
{
    std::uint64_t sum = 0;
    for( std::uint16_t y = 0; y < glyph.height; ++y ) {
        for( std::uint16_t x = 0; x < glyph.width; ++x ) {
            sum += pixels[( std::size_t{ glyph.y } + y ) * width + glyph.x + x];
        }
    }
    return sum;
}

void atlas()
{
    FontSet fonts;
    CHECK( !fonts.add( open_test_face() ) );
    CHECK( !fonts.set_pixel_size( 24 ) );

    GlyphAtlas atlas{ 256, 128, 64 };
    CHECK( atlas.width() == 256 && atlas.height() == 128 );
    CHECK( atlas.dirty() == ( GlyphAtlas::Rect{ 0, 0, 256, 128 } ) ); // all of it, to upload once
    atlas.clear_dirty();
    CHECK( atlas.dirty().empty() );

    const auto a = atlas.glyph( fonts, U'A', {} );
    CHECK( a && a->width > 0 && a->height > 0 && a->face == 0 );
    CHECK( a && a->advance == fonts.metrics().cell_width );
    // Its coverage, as the face renders it.
    Face face = open_test_face();
    CHECK( !face.set_pixel_size( 24 ) );
    const auto rendered = face.render( U'A' );
    CHECK( rendered && a &&
           sum_of( atlas.pixels(), atlas.width(), *a ) == ink_of( *rendered ).total );
    CHECK( a && atlas.dirty() == ( GlyphAtlas::Rect{ a->x, a->y, a->width, a->height } ) );
    atlas.clear_dirty();

    // The second time: found, nothing rendered or changed.
    const std::size_t held = atlas.size();
    CHECK( atlas.glyph( fonts, U'A', {} ) == a );
    CHECK( atlas.size() == held && atlas.dirty().empty() );
    // Bold is another glyph; so is italic.
    const auto bold = atlas.glyph( fonts, U'A', { .bold = true } );
    CHECK( bold && a && ( bold->x != a->x || bold->y != a->y ) );
    CHECK( atlas.glyph( fonts, U'A', { .italic = true } ).has_value() );
    // A space: placed, no pixels.
    const auto space = atlas.glyph( fonts, U' ', {} );
    CHECK( space && space->width == 0 && space->advance == fonts.metrics().cell_width );
    // No face has it: remembered as such.
    CHECK( !atlas.glyph( fonts, U'\uE000', {} ) );
    const std::size_t with_missing = atlas.size();
    CHECK( !atlas.glyph( fonts, U'\uE000', {} ) );
    CHECK( atlas.size() == with_missing );

    // Glyphs never overlap.
    std::vector<AtlasGlyph> placed;
    const std::uint32_t generation = atlas.generation();
    for( char32_t c = U'a'; c <= U'z' && atlas.generation() == generation; ++c ) {
        if( const auto g = atlas.glyph( fonts, c, {} ) ) {
            placed.push_back( *g );
        }
    }
    for( std::size_t i = 0; i < placed.size(); ++i ) {
        for( std::size_t j = i + 1; j < placed.size(); ++j ) {
            const AtlasGlyph &p = placed[i];
            const AtlasGlyph &q = placed[j];
            const bool apart = p.x + p.width <= q.x || q.x + q.width <= p.x ||
                               p.y + p.height <= q.y || q.y + q.height <= p.y;
            CHECK( apart || p.width == 0 || q.width == 0 );
        }
    }

    // Full: it starts again, and the glyph asked for is in it.
    GlyphAtlas small{ 64, 64, 1000 };
    std::uint32_t seen = small.generation();
    bool restarted = false;
    for( char32_t c = U'!'; c <= U'~'; ++c ) {
        const auto g = small.glyph( fonts, c, {} );
        CHECK( g.has_value() );
        if( small.generation() != seen ) {
            restarted = true;
            seen = small.generation();
            CHECK( small.size() == 1 );
        }
    }
    CHECK( restarted );
    // At capacity: the same.
    GlyphAtlas few{ 512, 512, 4 };
    const std::uint32_t before = few.generation();
    for( char32_t c = U'a'; c < U'a' + 5; ++c ) {
        CHECK( few.glyph( fonts, c, {} ).has_value() );
    }
    CHECK( few.generation() == before + 1 && few.size() == 1 );
    // A glyph larger than the atlas: none.
    GlyphAtlas tiny{ 4, 4, 8 };
    CHECK( !tiny.glyph( fonts, U'W', {} ) );
    // Cleared by hand (a new size): empty, a new generation, all dirty.
    few.clear_dirty();
    few.clear();
    CHECK( few.size() == 0 && few.dirty() == ( GlyphAtlas::Rect{ 0, 0, 512, 512 } ) );
}

} // namespace

int main()
{
    opening_and_metrics();
    glyphs();
    bold_and_italic();
    from_memory_and_errors();
    font_set();
    atlas();
    if( failures > 0 ) {
        std::fprintf( stderr, "font_test: %d failed\n", failures );
        return 1;
    }
    std::printf( "font_test: ok\n" );
    return 0;
}
