#pragma once

#include <cstdint>

namespace sq::graphics
{

/**
 * @brief Texture coordinate wrapping mode.
 */
enum class TextureWrap : std::uint8_t { ClampToEdge, Repeat, MirroredRepeat };

} // namespace sq::graphics
