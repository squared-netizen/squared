#pragma once

namespace sq::gles {

/** @brief Pixel format of a texture's storage. */
enum class TextureFormat {
    /** @brief Eight bits each of red, green, blue and alpha. */
    Rgba8,

    /** @brief Eight bits each of red, green and blue, no alpha. */
    Rgb8,

    /**
     * @brief One eight-bit channel, read as red.
     *
     * The usual choice for a font atlas or a mask. A shader reads it as
     * `texture(sampler, uv).r`, not `.a` &mdash; GLES 3.0 dropped the old
     * ALPHA format that put it in the alpha channel, and that surprise is why
     * this enumerator says so here.
     */
    R8
};

}  // namespace sq::gles
