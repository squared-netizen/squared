#include <squared/math/matrix4.hpp>

#include <cmath>

namespace squared::math {

Matrix4 Matrix4::orthographic(
    float left,
    float right,
    float bottom,
    float top,
    float near_plane,
    float far_plane
) noexcept
{
    const float width = right - left;
    const float height = top - bottom;
    const float depth = far_plane - near_plane;
    if (std::abs(width) < 0.000001F ||
        std::abs(height) < 0.000001F ||
        std::abs(depth) < 0.000001F) {
        return {};
    }

    return Matrix4{{
        2.0F / width, 0.0F, 0.0F, 0.0F,
        0.0F, 2.0F / height, 0.0F, 0.0F,
        0.0F, 0.0F, -2.0F / depth, 0.0F,
        -(right + left) / width,
        -(top + bottom) / height,
        -(far_plane + near_plane) / depth,
        1.0F
    }};
}

}  // namespace squared::math
