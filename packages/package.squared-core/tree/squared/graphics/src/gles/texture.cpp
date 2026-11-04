// The GPU half of Texture, for GLES 3.0.
//
// Eight members: everything that issues a graphics call. The policy and
// bookkeeping live in src/common/texture_common.cpp and are shared by every
// backend.

module;

#include <GLES3/gl3.h>

module sq.graphics;

import std;

namespace sq::graphics
{

namespace
{

GLint to_gl_filter( TextureFilter filter ) noexcept
{
    return filter == TextureFilter::Nearest ? GL_NEAREST : GL_LINEAR;
}

GLint to_gl_wrap( TextureWrap wrap ) noexcept
{
    switch( wrap ) {
        case TextureWrap::ClampToEdge:
            return GL_CLAMP_TO_EDGE;
        case TextureWrap::Repeat:
            return GL_REPEAT;
        case TextureWrap::MirroredRepeat:
            return GL_MIRRORED_REPEAT;
    }
    return GL_CLAMP_TO_EDGE;
}

} // namespace

bool Texture::upload_rgba( int width, int height, const std::uint8_t *pixels ) noexcept
{
    if( width <= 0 || height <= 0 || pixels == nullptr ) {
        return false;
    }

    if( handle_ == 0 ) {
        GLuint name = 0;
        glGenTextures( 1, &name );
        if( name == 0 ) {
            return false;
        }
        handle_ = name;
    }

    glBindTexture( GL_TEXTURE_2D, handle_ );

    // GL unpacks rows on a four-byte boundary by default. RGBA is always
    // aligned, but this class uploads whatever a recovery callback hands it,
    // and a one-pixel solid texture is four bytes wide. Setting it to 1 costs
    // nothing and removes a whole class of surprise.
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels );
    // A name that held an alpha texture before keeps its swizzle: put it back.
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_GREEN );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_BLUE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ALPHA );

    width_ = width;
    height_ = height;
    invalidated_ = false;
    format_ = TextureFormat::Rgba8;
    ++content_generation_;

    apply_sampling();
    return true;
}

bool Texture::upload_alpha( int width, int height, const std::uint8_t *pixels ) noexcept
{
    if( width <= 0 || height <= 0 || pixels == nullptr ) {
        return false;
    }

    if( handle_ == 0 ) {
        GLuint name = 0;
        glGenTextures( 1, &name );
        if( name == 0 ) {
            return false;
        }
        handle_ = name;
    }

    glBindTexture( GL_TEXTURE_2D, handle_ );
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // rows of any width
    // One byte a texel, in the red channel (GLES 3 has R8 and swizzles; the
    // old ALPHA format is gone from its sized formats). Sampled as white with
    // that alpha, so the sprite shader's colour tints it: no shader of its own.
    glTexImage2D( GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, pixels );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ONE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ONE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ONE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED );

    width_ = width;
    height_ = height;
    invalidated_ = false;
    format_ = TextureFormat::Alpha8;
    ++content_generation_;

    apply_sampling();
    return true;
}

void Texture::update_backend( int x, int y, int width, int height, const std::uint8_t *pixels,
                              int stride ) noexcept
{
    glBindTexture( GL_TEXTURE_2D, handle_ );
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    glPixelStorei( GL_UNPACK_ROW_LENGTH, stride );
    const GLenum layout = format_ == TextureFormat::Alpha8 ? GL_RED : GL_RGBA;
    glTexSubImage2D( GL_TEXTURE_2D, 0, x, y, width, height, layout, GL_UNSIGNED_BYTE, pixels );
    glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 ); // the default, for every other upload
}

void Texture::apply_sampling() noexcept
{
    if( handle_ == 0 ) {
        return;
    }

    glBindTexture( GL_TEXTURE_2D, handle_ );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, to_gl_filter( minification_ ) );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, to_gl_filter( magnification_ ) );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, to_gl_wrap( horizontal_wrap_ ) );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, to_gl_wrap( vertical_wrap_ ) );
}

void Texture::destroy_backend_object() noexcept
{
    if( handle_ != 0 ) {
        const GLuint name = handle_;
        glDeleteTextures( 1, &name );
        handle_ = 0;
    }
}

void Texture::destroy() noexcept
{
    destroy_backend_object();
    width_ = 0;
    height_ = 0;
    pixels_.clear();
    pixels_.shrink_to_fit();
    invalidated_ = false;
    ++content_generation_;
}

void Texture::set_filter( TextureFilter minification, TextureFilter magnification ) noexcept
{
    minification_ = minification;
    magnification_ = magnification;
    apply_sampling();
}

void Texture::set_wrap( TextureWrap horizontal, TextureWrap vertical ) noexcept
{
    horizontal_wrap_ = horizontal;
    vertical_wrap_ = vertical;
    apply_sampling();
}

void Texture::bind( unsigned int unit ) const noexcept
{
    glActiveTexture( GL_TEXTURE0 + static_cast<GLenum>( unit ) );
    glBindTexture( GL_TEXTURE_2D, handle_ );
}

} // namespace sq::graphics
