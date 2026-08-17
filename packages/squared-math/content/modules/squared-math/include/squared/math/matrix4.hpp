#pragma once

#include <array>

namespace squared::math {

/**
 * @brief Column-major four-by-four matrix compatible with OpenGL ES.
 *
 * Storage is column-major with `values_` laid out for direct upload to an
 * OpenGL uniform. Vectors are treated as columns, so a transform applies as
 * `M * v`. Handedness is inherited from the consuming camera and projection
 * conventions rather than imposed here. All operations are value types and
 * never allocate.
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
     *
     * @param left Near-plane left boundary in world units.
     * @param right Near-plane right boundary in world units.
     * @param bottom Top-plane bottom boundary in world units.
     * @param top Top-plane top boundary in world units.
     * @param near_plane Depth of the near plane.
     * @param far_plane Depth of the far plane.
     * @return An orthographic projection, or the identity matrix when the
     * bounds are degenerate.
     */
    [[nodiscard]] static Matrix4 orthographic(
        float left,
        float right,
        float bottom,
        float top,
        float near_plane = -1.0F,
        float far_plane = 1.0F
    ) noexcept;

    /**
     * @brief Read the contiguous OpenGL-compatible matrix data.
     * @return Pointer to 16 column-major values; valid for this object's
     * lifetime.
     */
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
