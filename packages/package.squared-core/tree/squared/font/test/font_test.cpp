// sq::font::Face (12.3a) on the test font, JetBrains Mono Regular
// (test/data/, OFL; read from the module's directory, where tests run):
// opening from a file and from memory, the metrics at two sizes, glyphs
// rendered (coverage, bearings, advance), bold and italic made from the
// regular face, a character it lacks, a .ttc index it lacks, errors, moves.

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

} // namespace

int main()
{
    opening_and_metrics();
    glyphs();
    bold_and_italic();
    from_memory_and_errors();
    if( failures > 0 ) {
        std::fprintf( stderr, "font_test: %d failed\n", failures );
        return 1;
    }
    std::printf( "font_test: ok\n" );
    return 0;
}
