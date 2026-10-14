#pragma once

#include <cstdint>

namespace sq::gles
{

/** @brief How sampling outside the zero-to-one range behaves. */
enum class TextureWrap : std::uint8_t {
    /**
     * @brief Clamp to the edge texel.
     * @note The right choice for any region inside an atlas: Repeat would pull
     * in a neighbouring sprite, which shows up as a one-pixel fringe.
     */
    ClampToEdge,

    /** @brief Tile the texture. */
    Repeat,

    /** @brief Tile, flipping every other repetition. */
    MirroredRepeat
};

} // namespace sq::gles
