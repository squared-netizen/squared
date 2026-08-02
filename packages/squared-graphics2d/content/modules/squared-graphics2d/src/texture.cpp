#include <squared/graphics2d/texture.hpp>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengles2_khrplatform.h>
#include <SDL_opengles2_gl2platform.h>
#include <SDL_opengles2_gl2.h>

#include <algorithm>
#include <array>
#include <utility>

namespace squared::graphics2d {
namespace {

GLint to_gl_filter(TextureFilter filter) noexcept
{
    return filter == TextureFilter::Nearest ? GL_NEAREST : GL_LINEAR;
}

GLint to_gl_wrap(TextureWrap wrap) noexcept
{
    switch (wrap) {
    case TextureWrap::Repeat:
        return GL_REPEAT;
    case TextureWrap::MirroredRepeat:
        return GL_MIRRORED_REPEAT;
    case TextureWrap::ClampToEdge:
    default:
        return GL_CLAMP_TO_EDGE;
    }
}

std::uint8_t to_byte(float value) noexcept
{
    return static_cast<std::uint8_t>(
        std::clamp(value, 0.0F, 1.0F) * 255.0F + 0.5F
    );
}

}  // namespace

Texture::Texture(Texture&& other) noexcept
    : handle_(std::exchange(other.handle_, 0)),
      width_(std::exchange(other.width_, 0)),
      height_(std::exchange(other.height_, 0))
{
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this == &other) return *this;
    destroy();
    handle_ = std::exchange(other.handle_, 0);
    width_ = std::exchange(other.width_, 0);
    height_ = std::exchange(other.height_, 0);
    return *this;
}

Texture::~Texture()
{
    destroy();
}

bool Texture::load(const char* asset_path) noexcept
{
    SDL_Surface* loaded = IMG_Load(asset_path);
    if (!loaded) {
        SDL_Log("IMG_Load failed for %s: %s", asset_path, IMG_GetError());
        return false;
    }

    SDL_Surface* rgba = SDL_ConvertSurfaceFormat(
        loaded,
        SDL_PIXELFORMAT_ABGR8888,
        0
    );
    SDL_FreeSurface(loaded);
    if (!rgba) {
        SDL_Log("Texture conversion failed: %s", SDL_GetError());
        return false;
    }

    const bool created = create_rgba(rgba->w, rgba->h, static_cast<
        const std::uint8_t*
    >(rgba->pixels));
    SDL_FreeSurface(rgba);
    return created;
}

bool Texture::create_rgba(
    int width,
    int height,
    const std::uint8_t* pixels
) noexcept
{
    if (width <= 0 || height <= 0 || !pixels) return false;
    destroy();

    glGenTextures(1, &handle_);
    if (!handle_) return false;

    while (glGetError() != GL_NO_ERROR) {
    }
    width_ = width;
    height_ = height;
    bind();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        width_,
        height_,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    set_filter(TextureFilter::Nearest, TextureFilter::Nearest);

    if (glGetError() != GL_NO_ERROR) {
        SDL_Log("OpenGL ES texture upload failed");
        destroy();
        return false;
    }
    return true;
}

bool Texture::create_solid(squared::graphics::Color color) noexcept
{
    const auto safe = color.clamped();
    const std::array<std::uint8_t, 4> pixel{
        to_byte(safe.red),
        to_byte(safe.green),
        to_byte(safe.blue),
        to_byte(safe.alpha)
    };
    return create_rgba(1, 1, pixel.data());
}

void Texture::destroy() noexcept
{
    if (handle_) {
        glDeleteTextures(1, &handle_);
        handle_ = 0;
    }
    width_ = 0;
    height_ = 0;
}

void Texture::set_filter(
    TextureFilter minification,
    TextureFilter magnification
) noexcept
{
    if (!handle_) return;
    bind();
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        to_gl_filter(minification)
    );
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        to_gl_filter(magnification)
    );
}

void Texture::set_wrap(
    TextureWrap horizontal,
    TextureWrap vertical
) noexcept
{
    if (!handle_) return;
    bind();
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        to_gl_wrap(horizontal)
    );
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        to_gl_wrap(vertical)
    );
}

void Texture::bind(unsigned int unit) const noexcept
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, handle_);
}

bool Texture::valid() const noexcept
{
    return handle_ != 0;
}

int Texture::width() const noexcept
{
    return width_;
}

int Texture::height() const noexcept
{
    return height_;
}

unsigned int Texture::native_handle() const noexcept
{
    return handle_;
}

}  // namespace squared::graphics2d
