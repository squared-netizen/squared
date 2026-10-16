#pragma once

#include <cstdint>

namespace sq::graphics
{

/**
 * @brief How a texture is sampled between texels.
 *
 * The one filter vocabulary in squared: `graphics::Texture` and the lower-level
 * `gles::Texture2D` both take it.
 */
enum class TextureFilter : std::uint8_t {
    /** @brief Take the nearest texel. Crisp; correct for pixel art. */
    Nearest,

    /** @brief Blend the four nearest texels. Smooth; the usual default. */
    Linear
};

} // namespace sq::graphics
