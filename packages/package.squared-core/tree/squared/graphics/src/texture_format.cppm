export module sq.graphics:src.texture_format;

import std;

export namespace sq::graphics
{

/**
 * @brief What one texel of a `Texture` holds.
 *
 * `Rgba8` is four bytes a texel, colour and alpha, as loaded images are.
 * `Alpha8` is one byte, drawn as white with that alpha, so the draw colour
 * tints it: glyph coverage, masks. A quarter of the memory, on the GPU and in
 * a retained copy.
 */
enum class TextureFormat : std::uint8_t {
    Rgba8,
    Alpha8,
};

/** @brief Bytes a texel of `format` takes. */
[[nodiscard]] constexpr int texel_bytes( TextureFormat format ) noexcept
{
    return format == TextureFormat::Alpha8 ? 1 : 4;
}

} // namespace sq::graphics
