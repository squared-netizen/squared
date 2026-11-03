module;

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SYNTHESIS_H

module sq.font;

import std;

namespace sq::font
{

namespace
{

FT_Library library_of( void *library ) noexcept
{
    return static_cast<FT_Library>( library );
}

FT_Face face_of( void *face ) noexcept
{
    return static_cast<FT_Face>( face );
}

// 26.6 fixed point to whole pixels.
std::int16_t ceil_pixels( FT_Pos value ) noexcept
{
    return static_cast<std::int16_t>( ( value + 63 ) >> 6 );
}
std::int16_t floor_pixels( FT_Pos value ) noexcept
{
    return static_cast<std::int16_t>( value >> 6 );
}
std::int16_t round_pixels( FT_Pos value ) noexcept
{
    return static_cast<std::int16_t>( ( value + 32 ) >> 6 );
}

// Light hinting: the auto-hinter, vertically only.
constexpr FT_Int32 load_flags = FT_LOAD_DEFAULT | FT_LOAD_TARGET_LIGHT;

} // namespace

Face::Face( Face &&other ) noexcept
    : library_( std::exchange( other.library_, nullptr ) )
    , face_( std::exchange( other.face_, nullptr ) )
    , metrics_( other.metrics_ )
    , sized_( std::exchange( other.sized_, false ) )
{}

Face &Face::operator=( Face &&other ) noexcept
{
    if( this != &other ) {
        close();
        library_ = std::exchange( other.library_, nullptr );
        face_ = std::exchange( other.face_, nullptr );
        metrics_ = other.metrics_;
        sized_ = std::exchange( other.sized_, false );
    }
    return *this;
}

Face::~Face()
{
    close();
}

void Face::close() noexcept
{
    if( face_ != nullptr ) {
        FT_Done_Face( face_of( face_ ) );
        face_ = nullptr;
    }
    if( library_ != nullptr ) {
        FT_Done_FreeType( library_of( library_ ) );
        library_ = nullptr;
    }
    metrics_ = {};
    sized_ = false;
}

FaceError Face::opened( int error, int index ) noexcept
{
    if( error == 0 ) {
        return {};
    }
    close();
    if( index > 0 && error == FT_Err_Invalid_Argument ) {
        return { FaceErrorCode::no_such_face, error };
    }
    return { FaceErrorCode::open, error };
}

FaceError Face::open_file( std::string_view path, int index )
{
    close();
    FT_Library library = nullptr;
    if( const FT_Error error = FT_Init_FreeType( &library ); error != 0 ) {
        return { FaceErrorCode::library, error };
    }
    library_ = library;
    const std::string terminated{ path };
    FT_Face face = nullptr;
    const FT_Error error = FT_New_Face( library, terminated.c_str(), index, &face );
    face_ = error == 0 ? face : nullptr;
    return opened( error, index );
}

FaceError Face::open_memory( std::span<const std::byte> file, int index )
{
    close();
    FT_Library library = nullptr;
    if( const FT_Error error = FT_Init_FreeType( &library ); error != 0 ) {
        return { FaceErrorCode::library, error };
    }
    library_ = library;
    FT_Face face = nullptr;
    const FT_Error error =
        FT_New_Memory_Face( library, reinterpret_cast<const FT_Byte *>( file.data() ),
                            static_cast<FT_Long>( file.size() ), index, &face );
    face_ = error == 0 ? face : nullptr;
    return opened( error, index );
}

FaceError Face::set_pixel_size( std::uint16_t pixels )
{
    if( face_ == nullptr ) {
        return { FaceErrorCode::not_open, 0 };
    }
    if( pixels == 0 ) {
        return { FaceErrorCode::size, 0 };
    }
    FT_Face face = face_of( face_ );
    if( const FT_Error error = FT_Set_Pixel_Sizes( face, 0, pixels ); error != 0 ) {
        return { FaceErrorCode::size, error };
    }
    const FT_Size_Metrics &size = face->size->metrics;
    FontMetrics metrics;
    metrics.ascender = ceil_pixels( size.ascender );
    metrics.descender = floor_pixels( size.descender );
    const int extent = metrics.ascender - metrics.descender;
    metrics.cell_height =
        static_cast<std::uint16_t>( std::max<int>( extent, round_pixels( size.height ) ) );
    // A monospace face's advance, from 'M'; else the widest advance.
    FT_Pos advance = size.max_advance;
    if( FT_Load_Char( face, 'M', load_flags ) == 0 ) {
        advance = face->glyph->advance.x;
    }
    metrics.cell_width = static_cast<std::uint16_t>( std::max<int>( round_pixels( advance ), 1 ) );
    if( FT_IS_SCALABLE( face ) ) {
        metrics.underline_position = static_cast<std::int16_t>(
            -round_pixels( FT_MulFix( face->underline_position, size.y_scale ) ) );
        metrics.underline_thickness = static_cast<std::uint16_t>( std::max<int>(
            round_pixels( FT_MulFix( face->underline_thickness, size.y_scale ) ), 1 ) );
    } else {
        metrics.underline_position = 1;
        metrics.underline_thickness = 1;
    }
    metrics_ = metrics;
    sized_ = true;
    return {};
}

std::string_view Face::family() const noexcept
{
    if( face_ == nullptr || face_of( face_ )->family_name == nullptr ) {
        return {};
    }
    return face_of( face_ )->family_name;
}

bool Face::bold() const noexcept
{
    return face_ != nullptr && ( face_of( face_ )->style_flags & FT_STYLE_FLAG_BOLD ) != 0;
}

bool Face::italic() const noexcept
{
    return face_ != nullptr && ( face_of( face_ )->style_flags & FT_STYLE_FLAG_ITALIC ) != 0;
}

bool Face::has_glyph( char32_t code ) const noexcept
{
    return face_ != nullptr && FT_Get_Char_Index( face_of( face_ ), code ) != 0;
}

std::optional<GlyphBitmap> Face::render( char32_t code, GlyphStyle style ) noexcept
{
    if( face_ == nullptr || !sized_ ) {
        return std::nullopt;
    }
    FT_Face face = face_of( face_ );
    const FT_UInt index = FT_Get_Char_Index( face, code );
    if( index == 0 || FT_Load_Glyph( face, index, load_flags ) != 0 ) {
        return std::nullopt;
    }
    FT_GlyphSlot slot = face->glyph;
    if( style.italic && !italic() ) {
        FT_GlyphSlot_Oblique( slot );
    }
    if( style.bold && !bold() ) {
        FT_GlyphSlot_Embolden( slot );
    }
    if( FT_Render_Glyph( slot, FT_RENDER_MODE_NORMAL ) != 0 ) {
        return std::nullopt;
    }
    const FT_Bitmap &bitmap = slot->bitmap;
    // 8-bit coverage only: a colour (emoji) or 1-bit bitmap is not one.
    if( bitmap.pixel_mode != FT_PIXEL_MODE_GRAY || bitmap.pitch < 0 ) {
        return std::nullopt;
    }
    GlyphBitmap glyph;
    glyph.width = static_cast<std::uint16_t>( bitmap.width );
    glyph.height = static_cast<std::uint16_t>( bitmap.rows );
    glyph.pitch = static_cast<std::uint16_t>( bitmap.pitch );
    if( bitmap.buffer != nullptr ) {
        glyph.coverage = { bitmap.buffer, std::size_t{ glyph.pitch } * glyph.height };
    }
    glyph.left = static_cast<std::int16_t>( slot->bitmap_left );
    glyph.top = static_cast<std::int16_t>( slot->bitmap_top );
    glyph.advance = round_pixels( slot->advance.x );
    return glyph;
}

} // namespace sq::font
