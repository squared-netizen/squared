#include <squared/graphics2d/texture.hpp>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengles2_khrplatform.h>
#include <SDL_opengles2_gl2platform.h>
#include <SDL_opengles2_gl2.h>

#include <algorithm>
#include <array>
#include <limits>
#include <utility>
#include <vector>

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

TextureRecoveryTarget::TextureRecoveryTarget(Texture& texture) noexcept
    : texture_(&texture)
{
}

bool TextureRecoveryTarget::upload_rgba(
    int width,
    int height,
    const std::uint8_t* pixels
) noexcept
{
    if (!texture_ || uploaded_) return false;
    uploaded_ = texture_->upload_rgba(width, height, pixels);
    return uploaded_;
}

Texture::Texture(Texture&& other) noexcept
    : handle_(std::exchange(other.handle_, 0)),
      width_(std::exchange(other.width_, 0)),
      height_(std::exchange(other.height_, 0)),
      recovery_policy_(std::exchange(
          other.recovery_policy_, TextureRecoveryPolicy::Discard
      )),
      asset_path_(std::move(other.asset_path_)),
      pixels_(std::move(other.pixels_)),
      recovery_callback_(std::exchange(other.recovery_callback_, nullptr)),
      recovery_user_data_(std::exchange(other.recovery_user_data_, nullptr)),
      minification_(other.minification_),
      magnification_(other.magnification_),
      horizontal_wrap_(other.horizontal_wrap_),
      vertical_wrap_(other.vertical_wrap_),
      invalidated_(std::exchange(other.invalidated_, false))
{
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this == &other) return *this;
    destroy();
    handle_ = std::exchange(other.handle_, 0);
    width_ = std::exchange(other.width_, 0);
    height_ = std::exchange(other.height_, 0);
    recovery_policy_ = std::exchange(
        other.recovery_policy_, TextureRecoveryPolicy::Discard
    );
    asset_path_ = std::move(other.asset_path_);
    pixels_ = std::move(other.pixels_);
    recovery_callback_ = std::exchange(other.recovery_callback_, nullptr);
    recovery_user_data_ = std::exchange(other.recovery_user_data_, nullptr);
    minification_ = other.minification_;
    magnification_ = other.magnification_;
    horizontal_wrap_ = other.horizontal_wrap_;
    vertical_wrap_ = other.vertical_wrap_;
    invalidated_ = std::exchange(other.invalidated_, false);
    return *this;
}

Texture::~Texture()
{
    destroy();
}

bool Texture::load(const char* asset_path) noexcept
{
    return load(asset_path, TextureRecoveryOptions::reload_from_asset());
}

bool Texture::load(
    const char* asset_path,
    TextureRecoveryOptions recovery
) noexcept
{
    if (!asset_path || !*asset_path) return false;
    if (recovery.policy == TextureRecoveryPolicy::Regenerate &&
        !recovery.callback) {
        SDL_Log("Texture regeneration policy requires a callback");
        return false;
    }
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

    std::string retained_path;
    std::vector<std::uint8_t> retained_pixels;
    try {
        if (recovery.policy == TextureRecoveryPolicy::ReloadFromAsset) {
            retained_path = asset_path;
        } else if (recovery.policy == TextureRecoveryPolicy::RetainPixels) {
            if (rgba->w <= 0 || rgba->h <= 0) {
                SDL_FreeSurface(rgba);
                return false;
            }
            const auto safe_width = static_cast<std::size_t>(rgba->w);
            const auto safe_height = static_cast<std::size_t>(rgba->h);
            if (safe_width >
                    std::numeric_limits<std::size_t>::max() / safe_height ||
                safe_width * safe_height >
                    std::numeric_limits<std::size_t>::max() / 4) {
                SDL_FreeSurface(rgba);
                return false;
            }
            const auto byte_count = safe_width * safe_height * 4;
            const auto* pixels = static_cast<const std::uint8_t*>(rgba->pixels);
            retained_pixels.assign(pixels, pixels + byte_count);
        }
    } catch (...) {
        SDL_FreeSurface(rgba);
        SDL_Log("Texture recovery recipe allocation failed");
        return false;
    }
    const bool created = upload_rgba(
        rgba->w,
        rgba->h,
        static_cast<const std::uint8_t*>(rgba->pixels)
    );
    SDL_FreeSurface(rgba);
    if (created) {
        recovery_policy_ = recovery.policy;
        asset_path_ = std::move(retained_path);
        pixels_ = std::move(retained_pixels);
        recovery_callback_ = recovery.policy == TextureRecoveryPolicy::Regenerate
            ? recovery.callback
            : nullptr;
        recovery_user_data_ = recovery.policy == TextureRecoveryPolicy::Regenerate
            ? recovery.user_data
            : nullptr;
    }
    return created;
}

bool Texture::create_rgba(
    int width,
    int height,
    const std::uint8_t* pixels
) noexcept
{
    return create_rgba(
        width, height, pixels, TextureRecoveryOptions::retain_pixels()
    );
}

bool Texture::create_rgba(
    int width,
    int height,
    const std::uint8_t* pixels,
    TextureRecoveryOptions recovery
) noexcept
{
    if (width <= 0 || height <= 0 || !pixels) return false;
    if (recovery.policy == TextureRecoveryPolicy::ReloadFromAsset) {
        SDL_Log("RGBA texture cannot use ReloadFromAsset without an asset");
        return false;
    }
    if (recovery.policy == TextureRecoveryPolicy::Regenerate &&
        !recovery.callback) {
        SDL_Log("Texture regeneration policy requires a callback");
        return false;
    }
    const auto safe_width = static_cast<std::size_t>(width);
    const auto safe_height = static_cast<std::size_t>(height);
    if (safe_width > std::numeric_limits<std::size_t>::max() / safe_height ||
        safe_width * safe_height >
            std::numeric_limits<std::size_t>::max() / 4) {
        return false;
    }
    std::vector<std::uint8_t> retained_pixels;
    if (recovery.policy == TextureRecoveryPolicy::RetainPixels) {
        try {
            retained_pixels.assign(
                pixels, pixels + safe_width * safe_height * 4
            );
        } catch (...) {
            SDL_Log("Texture restoration data allocation failed");
            return false;
        }
    }
    if (!upload_rgba(width, height, pixels)) return false;
    recovery_policy_ = recovery.policy;
    asset_path_.clear();
    pixels_ = std::move(retained_pixels);
    recovery_callback_ = recovery.policy == TextureRecoveryPolicy::Regenerate
        ? recovery.callback
        : nullptr;
    recovery_user_data_ = recovery.policy == TextureRecoveryPolicy::Regenerate
        ? recovery.user_data
        : nullptr;
    return true;
}

bool Texture::upload_rgba(
    int width,
    int height,
    const std::uint8_t* pixels
) noexcept
{
    if (width <= 0 || height <= 0 || !pixels) return false;
    const int previous_width = width_;
    const int previous_height = height_;
    release();

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
    set_filter(minification_, magnification_);
    set_wrap(horizontal_wrap_, vertical_wrap_);

    if (glGetError() != GL_NO_ERROR) {
        SDL_Log("OpenGL ES texture upload failed");
        release();
        width_ = previous_width;
        height_ = previous_height;
        return false;
    }
    invalidated_ = false;
    return true;
}

bool Texture::create_solid(squared::graphics::Color color) noexcept
{
    return create_solid(color, TextureRecoveryOptions::retain_pixels());
}

bool Texture::create_solid(
    squared::graphics::Color color,
    TextureRecoveryOptions recovery
) noexcept
{
    const auto safe = color.clamped();
    const std::array<std::uint8_t, 4> pixel{
        to_byte(safe.red),
        to_byte(safe.green),
        to_byte(safe.blue),
        to_byte(safe.alpha)
    };
    return create_rgba(1, 1, pixel.data(), recovery);
}

void Texture::destroy() noexcept
{
    release();
    width_ = 0;
    height_ = 0;
    recovery_policy_ = TextureRecoveryPolicy::Discard;
    std::string{}.swap(asset_path_);
    std::vector<std::uint8_t>{}.swap(pixels_);
    recovery_callback_ = nullptr;
    recovery_user_data_ = nullptr;
    minification_ = TextureFilter::Nearest;
    magnification_ = TextureFilter::Nearest;
    horizontal_wrap_ = TextureWrap::ClampToEdge;
    vertical_wrap_ = TextureWrap::ClampToEdge;
}

void Texture::release() noexcept
{
    if (invalidated_) {
        handle_ = 0;
        invalidated_ = false;
        return;
    }
    if (handle_) {
        glDeleteTextures(1, &handle_);
        handle_ = 0;
    }
    invalidated_ = false;
}

void Texture::invalidate() noexcept
{
    if (handle_) invalidated_ = true;
}

bool Texture::restore(bool context_preserved) noexcept
{
    if (invalidated_) {
        if (context_preserved && handle_ && glIsTexture(handle_) == GL_TRUE) {
            invalidated_ = false;
            return true;
        }
        handle_ = 0;
        invalidated_ = false;
    }
    if (handle_ && !invalidated_) return true;
    switch (recovery_policy_) {
    case TextureRecoveryPolicy::ReloadFromAsset:
        return restore_asset();
    case TextureRecoveryPolicy::RetainPixels:
        return !pixels_.empty() && upload_rgba(width_, height_, pixels_.data());
    case TextureRecoveryPolicy::Regenerate:
        return restore_callback();
    case TextureRecoveryPolicy::Discard:
    default:
        return false;
    }
}

bool Texture::restore_asset() noexcept
{
    SDL_Surface* loaded = IMG_Load(asset_path_.c_str());
    if (!loaded) {
        SDL_Log(
            "IMG_Load restore failed for %s: %s",
            asset_path_.c_str(), IMG_GetError()
        );
        return false;
    }
    SDL_Surface* rgba = SDL_ConvertSurfaceFormat(
        loaded, SDL_PIXELFORMAT_ABGR8888, 0
    );
    SDL_FreeSurface(loaded);
    if (!rgba) {
        SDL_Log("Texture restore conversion failed: %s", SDL_GetError());
        return false;
    }
    const bool restored = upload_rgba(
        rgba->w, rgba->h,
        static_cast<const std::uint8_t*>(rgba->pixels)
    );
    SDL_FreeSurface(rgba);
    return restored;
}

bool Texture::restore_callback() noexcept
{
    if (!recovery_callback_) return false;
    TextureRecoveryTarget target(*this);
    return recovery_callback_(recovery_user_data_, target) && target.uploaded_;
}

bool Texture::restorable() const noexcept
{
    switch (recovery_policy_) {
    case TextureRecoveryPolicy::ReloadFromAsset:
        return !asset_path_.empty();
    case TextureRecoveryPolicy::RetainPixels:
        return !pixels_.empty();
    case TextureRecoveryPolicy::Regenerate:
        return recovery_callback_ != nullptr;
    case TextureRecoveryPolicy::Discard:
    default:
        return false;
    }
}

TextureRecoveryPolicy Texture::recovery_policy() const noexcept
{
    return recovery_policy_;
}

std::size_t Texture::retained_recovery_bytes() const noexcept
{
    return pixels_.size();
}

void Texture::set_filter(
    TextureFilter minification,
    TextureFilter magnification
) noexcept
{
    minification_ = minification;
    magnification_ = magnification;
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
    horizontal_wrap_ = horizontal;
    vertical_wrap_ = vertical;
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
    return handle_ != 0 && !invalidated_;
}

int Texture::width() const noexcept
{
    return width_;
}

int Texture::height() const noexcept
{
    return height_;
}

}  // namespace squared::graphics2d
