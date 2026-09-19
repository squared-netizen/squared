#include <squared/gles/texture2d.hpp>

#include <squared/gles/gles_error.hpp>
#include <squared/gles/gles_error_code.hpp>
#include <squared/gles/texture_filter.hpp>
#include <squared/gles/texture_format.hpp>
#include <squared/gles/texture_wrap.hpp>

#include <cstddef>
#include <span>
#include <utility>

#include <GLES3/gl3.h>

namespace sq::gles {

namespace {

struct GlFormat final {
    GLenum internal_format;
    GLenum format;
};

GlFormat to_gl_format(TextureFormat format) noexcept
{
    switch (format) {
    case TextureFormat::Rgba8: return {GL_RGBA8, GL_RGBA};
    case TextureFormat::Rgb8: return {GL_RGB8, GL_RGB};
    case TextureFormat::R8: return {GL_R8, GL_RED};
    }
    return {GL_RGBA8, GL_RGBA};
}

GLint to_gl_filter(TextureFilter filter) noexcept
{
    return filter == TextureFilter::Nearest ? GL_NEAREST : GL_LINEAR;
}

GLint to_gl_wrap(TextureWrap wrap) noexcept
{
    switch (wrap) {
    case TextureWrap::ClampToEdge: return GL_CLAMP_TO_EDGE;
    case TextureWrap::Repeat: return GL_REPEAT;
    case TextureWrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
    }
    return GL_CLAMP_TO_EDGE;
}

}  // namespace

int Texture2D::bytes_per_pixel(TextureFormat format) noexcept
{
    switch (format) {
    case TextureFormat::Rgba8: return 4;
    case TextureFormat::Rgb8: return 3;
    case TextureFormat::R8: return 1;
    }
    return 4;
}

Texture2D::~Texture2D()
{
    destroy();
}

Texture2D::Texture2D(Texture2D&& other) noexcept
    : name_(std::exchange(other.name_, 0))
    , width_(std::exchange(other.width_, 0))
    , height_(std::exchange(other.height_, 0))
    , format_(other.format_)
{
}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept
{
    if (this != &other) {
        destroy();
        name_ = std::exchange(other.name_, 0);
        width_ = std::exchange(other.width_, 0);
        height_ = std::exchange(other.height_, 0);
        format_ = other.format_;
    }
    return *this;
}

GlesError Texture2D::create(
    int width,
    int height,
    TextureFormat format,
    std::span<const std::byte> pixels
) noexcept
{
    if (width <= 0 || height <= 0) {
        return GlesError{
            .code = GlesErrorCode::InvalidArgument,
            .message = "texture dimensions must be positive"
        };
    }

    const auto required = static_cast<std::size_t>(width)
        * static_cast<std::size_t>(height)
        * static_cast<std::size_t>(bytes_per_pixel(format));
    if (!pixels.empty() && pixels.size() < required) {
        return GlesError{
            .code = GlesErrorCode::InvalidArgument,
            .message = "pixel span is smaller than the requested size"
        };
    }

    destroy();

    GLuint name = 0;
    glGenTextures(1, &name);
    if (name == 0) return check_gl_errors();

    glBindTexture(GL_TEXTURE_2D, name);

    // GL unpacks rows on a four-byte boundary by default. An R8 texture whose
    // width is not a multiple of four is then read with a gap at the end of
    // every row, which is the classic skewed font atlas.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    const GlFormat gl_format = to_gl_format(format);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        static_cast<GLint>(gl_format.internal_format),
        width,
        height,
        0,
        gl_format.format,
        GL_UNSIGNED_BYTE,
        pixels.empty() ? nullptr : pixels.data()
    );

    if (GlesError error = check_gl_errors()) {
        glDeleteTextures(1, &name);
        return error;
    }

    name_ = name;
    width_ = width;
    height_ = height;
    format_ = format;

    // Sensible defaults so a freshly created texture samples rather than
    // coming out black: GL's default minification filter needs mipmaps.
    return set_sampling(
        TextureFilter::Linear, TextureFilter::Linear, TextureWrap::ClampToEdge
    );
}

GlesError Texture2D::upload(std::span<const std::byte> pixels) noexcept
{
    return upload_region(0, 0, width_, height_, pixels);
}

GlesError Texture2D::upload_region(
    int x,
    int y,
    int width,
    int height,
    std::span<const std::byte> pixels
) noexcept
{
    if (name_ == 0) {
        return GlesError{
            .code = GlesErrorCode::InvalidArgument,
            .message = "texture has no storage"
        };
    }
    if (width <= 0 || height <= 0 || x < 0 || y < 0
        || x + width > width_ || y + height > height_) {
        return GlesError{
            .code = GlesErrorCode::InvalidArgument,
            .message = "region lies outside the texture"
        };
    }

    const auto required = static_cast<std::size_t>(width)
        * static_cast<std::size_t>(height)
        * static_cast<std::size_t>(bytes_per_pixel(format_));
    if (pixels.size() < required) {
        return GlesError{
            .code = GlesErrorCode::InvalidArgument,
            .message = "pixel span is smaller than the region"
        };
    }

    glBindTexture(GL_TEXTURE_2D, name_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        x,
        y,
        width,
        height,
        to_gl_format(format_).format,
        GL_UNSIGNED_BYTE,
        pixels.data()
    );
    return check_gl_errors();
}

GlesError Texture2D::set_sampling(
    TextureFilter minify,
    TextureFilter magnify,
    TextureWrap wrap
) noexcept
{
    if (name_ == 0) {
        return GlesError{
            .code = GlesErrorCode::InvalidArgument,
            .message = "texture has no storage"
        };
    }

    glBindTexture(GL_TEXTURE_2D, name_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, to_gl_filter(minify));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, to_gl_filter(magnify));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, to_gl_wrap(wrap));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, to_gl_wrap(wrap));
    return check_gl_errors();
}

void Texture2D::bind(int unit) const noexcept
{
    glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(unit));
    glBindTexture(GL_TEXTURE_2D, name_);
}

void Texture2D::destroy() noexcept
{
    if (name_ != 0) {
        glDeleteTextures(1, &name_);
        name_ = 0;
    }
    width_ = 0;
    height_ = 0;
}

void Texture2D::invalidate() noexcept
{
    name_ = 0;
    width_ = 0;
    height_ = 0;
}

}  // namespace sq::gles
