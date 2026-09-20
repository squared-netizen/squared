// Backend-independent half of Texture.
//
// Everything here is policy, bookkeeping and validation - no graphics calls.
// The backend half (src/<backend>/texture.cpp) supplies upload_rgba, bind,
// set_filter, set_wrap, destroy and invalidate, which are the only members
// that touch a GPU.
//
// Splitting it this way means the recovery logic - the part with the
// interesting failure modes - is written once and tested once, rather than
// once per backend.

#include <squared/graphics2d/texture.hpp>

#include <squared/files/file_handle.hpp>
#include <squared/files/file_read_result.hpp>
#include <squared/graphics/color.hpp>
#include <squared/graphics2d/texture_recovery_options.hpp>
#include <squared/graphics2d/texture_recovery_policy.hpp>
#include <squared/graphics2d/texture_recovery_target.hpp>

#include "detail/image_decode.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace sq::graphics2d {

namespace {

/** @brief Convert a normalised component to an eight-bit texel value. */
std::uint8_t to_byte(float value) noexcept
{
    if (value <= 0.0F) return 0;
    if (value >= 1.0F) return 255;
    return static_cast<std::uint8_t>((value * 255.0F) + 0.5F);
}

/**
 * @brief Whether a policy needs a source file to recover.
 *
 * ReloadFromAsset is the default, and a texture created from memory has no
 * asset behind it. Accepting the combination would produce a texture that
 * looks correct and silently fails to come back after a phone call, which is
 * the worst shape a bug can take - so the creating call refuses it instead.
 */
bool needs_asset(TextureRecoveryPolicy policy) noexcept
{
    return policy == TextureRecoveryPolicy::ReloadFromAsset;
}

}  // namespace

Texture::Texture(Texture&& other) noexcept
    : handle_(std::exchange(other.handle_, 0))
    , width_(std::exchange(other.width_, 0))
    , height_(std::exchange(other.height_, 0))
    , recovery_policy_(other.recovery_policy_)
    , source_(std::move(other.source_))
    , pixels_(std::move(other.pixels_))
    , recovery_callback_(std::exchange(other.recovery_callback_, nullptr))
    , recovery_user_data_(std::exchange(other.recovery_user_data_, nullptr))
    , minification_(other.minification_)
    , magnification_(other.magnification_)
    , horizontal_wrap_(other.horizontal_wrap_)
    , vertical_wrap_(other.vertical_wrap_)
    , content_generation_(std::exchange(other.content_generation_, 0))
    , invalidated_(std::exchange(other.invalidated_, false))
{
    other.source_ = files::FileHandle{};
    other.pixels_.clear();
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other) {
        destroy();
        handle_ = std::exchange(other.handle_, 0);
        width_ = std::exchange(other.width_, 0);
        height_ = std::exchange(other.height_, 0);
        recovery_policy_ = other.recovery_policy_;
        source_ = std::move(other.source_);
        pixels_ = std::move(other.pixels_);
        recovery_callback_ = std::exchange(other.recovery_callback_, nullptr);
        recovery_user_data_ =
            std::exchange(other.recovery_user_data_, nullptr);
        minification_ = other.minification_;
        magnification_ = other.magnification_;
        horizontal_wrap_ = other.horizontal_wrap_;
        vertical_wrap_ = other.vertical_wrap_;
        content_generation_ = std::exchange(other.content_generation_, 0);
        invalidated_ = std::exchange(other.invalidated_, false);
        other.source_ = files::FileHandle{};
        other.pixels_.clear();
    }
    return *this;
}

Texture::~Texture()
{
    destroy();
}

bool Texture::load(const files::FileHandle& source) noexcept
{
    return load(source, TextureRecoveryOptions{});
}

bool Texture::load(
    const files::FileHandle& source,
    TextureRecoveryOptions recovery
) noexcept
{
    if (!source.valid()) return false;

    files::FileReadResult bytes = source.read_bytes();
    if (!bytes) return false;

    detail::DecodedImage image = detail::decode_image(bytes.bytes);
    if (!image) return false;

    // Release the encoded bytes before uploading: on a large atlas the encoded
    // file, the decoded pixels and the GPU copy would otherwise be resident at
    // the same moment, and the decoded copy alone can be several megabytes.
    bytes.bytes.clear();
    bytes.bytes.shrink_to_fit();

    if (!upload_rgba(image.width, image.height, image.pixels.data())) {
        return false;
    }

    recovery_policy_ = recovery.policy;
    recovery_callback_ = recovery.callback;
    recovery_user_data_ = recovery.user_data;
    source_ = source;

    if (recovery.policy == TextureRecoveryPolicy::RetainPixels) {
        pixels_ = std::move(image.pixels);
    } else {
        pixels_.clear();
        pixels_.shrink_to_fit();
    }
    return true;
}

bool Texture::create_rgba(
    int width,
    int height,
    const std::uint8_t* pixels
) noexcept
{
    // The default options ask for ReloadFromAsset, which this path cannot
    // honour. RetainPixels is the only policy that works for memory the caller
    // supplied and did not name.
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
    if (width <= 0 || height <= 0 || pixels == nullptr) return false;

    // Refuse rather than accept a policy with no source behind it. See
    // needs_asset above.
    if (needs_asset(recovery.policy)) return false;
    if (recovery.policy == TextureRecoveryPolicy::Regenerate
        && recovery.callback == nullptr) {
        return false;
    }

    if (!upload_rgba(width, height, pixels)) return false;

    recovery_policy_ = recovery.policy;
    recovery_callback_ = recovery.callback;
    recovery_user_data_ = recovery.user_data;
    source_ = files::FileHandle{};

    if (recovery.policy == TextureRecoveryPolicy::RetainPixels) {
        const auto count = static_cast<std::size_t>(width)
            * static_cast<std::size_t>(height) * 4U;
        pixels_.assign(pixels, pixels + count);
    } else {
        pixels_.clear();
        pixels_.shrink_to_fit();
    }
    return true;
}

bool Texture::create_solid(sq::graphics::Color color) noexcept
{
    return create_solid(color, TextureRecoveryOptions::retain_pixels());
}

bool Texture::create_solid(
    sq::graphics::Color color,
    TextureRecoveryOptions recovery
) noexcept
{
    const std::uint8_t texel[4] = {
        to_byte(color.red),
        to_byte(color.green),
        to_byte(color.blue),
        to_byte(color.alpha)
    };
    return create_rgba(1, 1, texel, recovery);
}

void Texture::release() noexcept
{
    // The GPU object goes; the recipe for rebuilding it stays. This is the
    // deliberate half of the release/invalidate pair: here the context is
    // still current, so deleting is correct.
    destroy_backend_object();
    ++content_generation_;
    invalidated_ = false;
}

void Texture::invalidate() noexcept
{
    // The context is gone, so the name belongs to a context that no longer
    // exists. Deleting it is at best a no-op and at worst frees an unrelated
    // object in the new context. Forget it instead.
    handle_ = 0;
    invalidated_ = true;
    ++content_generation_;
}

bool Texture::restorable() const noexcept
{
    switch (recovery_policy_) {
    case TextureRecoveryPolicy::ReloadFromAsset:
        return source_.valid();
    case TextureRecoveryPolicy::RetainPixels:
        return !pixels_.empty() && width_ > 0 && height_ > 0;
    case TextureRecoveryPolicy::Regenerate:
        return recovery_callback_ != nullptr;
    case TextureRecoveryPolicy::Discard:
        return false;
    }
    return false;
}

bool Texture::restore(bool context_preserved) noexcept
{
    // Nothing to do when the object survived the context change.
    if (context_preserved && handle_ != 0 && !invalidated_) return true;

    handle_ = 0;
    invalidated_ = false;

    switch (recovery_policy_) {
    case TextureRecoveryPolicy::ReloadFromAsset:
        return restore_asset();

    case TextureRecoveryPolicy::RetainPixels:
        if (pixels_.empty() || width_ <= 0 || height_ <= 0) return false;
        return upload_rgba(width_, height_, pixels_.data());

    case TextureRecoveryPolicy::Regenerate:
        return restore_callback();

    case TextureRecoveryPolicy::Discard:
        // The content is gone on purpose. width_ and height_ are cleared so a
        // caller cannot mistake the dimensions for live pixels, and
        // content_generation_ has already moved, so every region built against
        // it now reports invalid.
        width_ = 0;
        height_ = 0;
        return false;
    }
    return false;
}

bool Texture::restore_asset() noexcept
{
    if (!source_.valid()) return false;

    files::FileReadResult bytes = source_.read_bytes();
    if (!bytes) return false;

    detail::DecodedImage image = detail::decode_image(bytes.bytes);
    if (!image) return false;

    bytes.bytes.clear();
    bytes.bytes.shrink_to_fit();

    return upload_rgba(image.width, image.height, image.pixels.data());
}

bool Texture::restore_callback() noexcept
{
    if (recovery_callback_ == nullptr) return false;

    TextureRecoveryTarget target{*this};
    if (!recovery_callback_(recovery_user_data_, target)) return false;

    // A callback that reported success without uploading leaves an empty
    // texture behind, which would draw as garbage rather than fail. Treat it
    // as the failure it is.
    return handle_ != 0;
}

TextureRecoveryPolicy Texture::recovery_policy() const noexcept
{
    return recovery_policy_;
}

std::size_t Texture::retained_recovery_bytes() const noexcept
{
    return pixels_.capacity();
}

bool Texture::valid() const noexcept
{
    return handle_ != 0;
}

bool Texture::has_content() const noexcept
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

std::uint32_t Texture::content_generation() const noexcept
{
    return content_generation_;
}

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
    if (texture_ == nullptr || uploaded_) return false;
    if (!texture_->upload_rgba(width, height, pixels)) return false;
    uploaded_ = true;
    return true;
}

}  // namespace sq::graphics2d
