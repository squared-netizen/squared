#pragma once

#include <squared/math/matrix4.hpp>
#include <squared/math/vector2.hpp>

namespace squared::graphics2d {

/**
 * @brief Coordinate orientation used by an orthographic camera.
 */
enum class CoordinateOrigin {
    BottomLeft,
    TopLeft
};

/**
 * @brief Two-dimensional orthographic camera with pan and zoom.
 */
class OrthographicCamera {
public:
    /**
     * @brief Construct a camera centered in its logical viewport.
     */
    OrthographicCamera(
        float viewport_width,
        float viewport_height,
        CoordinateOrigin origin = CoordinateOrigin::TopLeft
    ) noexcept;

    /** @brief Set the logical viewport dimensions. */
    void set_viewport(float width, float height) noexcept;

    /** @brief Set the camera center in logical coordinates. */
    void set_position(float x, float y) noexcept;

    /** @brief Set the zoom factor; values below a safe minimum are clamped. */
    void set_zoom(float zoom) noexcept;

    /** @brief Recalculate the projection matrix after property changes. */
    void update() noexcept;

    /** @brief Return the current projection matrix. */
    [[nodiscard]] const squared::math::Matrix4& combined() const noexcept;

    /** @brief Return the camera center. */
    [[nodiscard]] squared::math::Vector2 position() const noexcept;

    /** @brief Return the current zoom factor. */
    [[nodiscard]] float zoom() const noexcept;

private:
    float viewport_width_;
    float viewport_height_;
    squared::math::Vector2 position_;
    float zoom_{1.0F};
    CoordinateOrigin origin_;
    squared::math::Matrix4 combined_;
};

}  // namespace squared::graphics2d
