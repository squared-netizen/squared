#pragma once

namespace sq::gles {

/** @brief How a texture is sampled between texels. */
enum class TextureFilter {
    /** @brief Take the nearest texel. Crisp; correct for pixel art. */
    Nearest,

    /** @brief Blend the four nearest texels. Smooth; the usual default. */
    Linear
};

}  // namespace sq::gles
