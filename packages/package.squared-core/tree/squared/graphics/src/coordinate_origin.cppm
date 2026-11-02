export module sq.graphics:src.coordinate_origin;

import std;

export namespace sq::graphics
{

/**
 * @brief Coordinate orientation used by an orthographic camera.
 */
enum class CoordinateOrigin : std::uint8_t { BottomLeft, TopLeft };

} // namespace sq::graphics
