#pragma once

#include <cstdint>

namespace sq::graphics
{

/**
 * @brief Coordinate orientation used by an orthographic camera.
 */
enum class CoordinateOrigin : std::uint8_t { BottomLeft, TopLeft };

} // namespace sq::graphics
