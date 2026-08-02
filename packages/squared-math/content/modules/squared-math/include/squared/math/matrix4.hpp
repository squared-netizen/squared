#pragma once

#include <array>

namespace squared::math {

/**
 * @brief Column-major four-by-four matrix compatible with OpenGL ES.
 */
class Matrix4 {
public:
    /** @brief Construct an identity matrix. */
    constexpr Matrix4() noexcept
        : values_{
              1.0F, 0.0F, 0.0F, 0.0F,
              0.0F, 1.0F, 0.0F, 0.0F,
              0.0F, 0.0F, 1.0F, 0.0F,
              0.0F, 0.0F, 0.0F, 1.0F
          }
    {
    }

    /**
     * @brief Create an orthographic projection matrix.
     */
    [[nodiscard]] static Matrix4 orthographic(
        float left,
        float right,
        float bottom,
        float top,
        float near_plane = -1.0F,
        float far_plane = 1.0F
    ) noexcept;

    /** @brief Read the contiguous OpenGL-compatible matrix data. */
    [[nodiscard]] constexpr const float* data() const noexcept
    {
        return values_.data();
    }

private:
    explicit constexpr Matrix4(std::array<float, 16> values) noexcept
        : values_(values)
    {
    }

    std::array<float, 16> values_;
};

}  // namespace squared::math
