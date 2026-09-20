// The GPU half of Texture, headless.
//
// Every member resolves and nothing is drawn, so graphics2d and everything
// above it can be built and tested on a machine with no GPU. Names are handed
// out from a counter so valid(), has_content() and the generation counter all
// behave exactly as they do against a real driver - which is what makes a test
// written here a test of the contract.

#include <squared/graphics2d/texture.hpp>

#include <squared/graphics2d/texture_filter.hpp>
#include <squared/graphics2d/texture_wrap.hpp>

#include <cstdint>

namespace sq::graphics2d {

namespace {

/** @brief Next headless texture name. Zero stays reserved for "no object". */
unsigned int g_next_name = 1;

}  // namespace

bool Texture::upload_rgba(
    int width,
    int height,
    const std::uint8_t* pixels
) noexcept
{
    if (width <= 0 || height <= 0 || pixels == nullptr) return false;

    if (handle_ == 0) handle_ = g_next_name++;


    // GL unpacks rows on a four-byte boundary by default. RGBA is always
    // aligned, but this class uploads whatever a recovery callback hands it,
    // and a one-pixel solid texture is four bytes wide. Setting it to 1 costs
    // nothing and removes a whole class of surprise.


    width_ = width;
    height_ = height;
    invalidated_ = false;
    ++content_generation_;

    apply_sampling();
    return true;
}

void Texture::apply_sampling() noexcept
{
    // Sampling state is recorded on the texture and applied by nothing.
}

void Texture::destroy_backend_object() noexcept
{
    handle_ = 0;
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

void Texture::set_filter(
    TextureFilter minification,
    TextureFilter magnification
) noexcept
{
    minification_ = minification;
    magnification_ = magnification;
    apply_sampling();
}

void Texture::set_wrap(
    TextureWrap horizontal,
    TextureWrap vertical
) noexcept
{
    horizontal_wrap_ = horizontal;
    vertical_wrap_ = vertical;
    apply_sampling();
}

void Texture::bind(unsigned int unit) const noexcept
{
    static_cast<void>(unit);
}

}  // namespace sq::graphics2d
