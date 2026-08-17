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
     * @param viewport_width Logical viewport width in the same units as
     * rendered coordinates; must be greater than zero for a valid projection.
     * @param viewport_height Logical viewport height; must be greater than
     * zero for a valid projection.
     * @param origin Coordinate orientation of the viewport.
     */
    OrthographicCamera(
        float viewport_width,
        float viewport_height,
        CoordinateOrigin origin = CoordinateOrigin::TopLeft
    ) noexcept;

    /**
     * @brief Set the logical viewport dimensions.
     * @param width Logical viewport width; must be greater than zero for a
     * valid projection.
     * @param height Logical viewport height; must be greater than zero for a
     * valid projection.
     * @note Call update() afterward to recalculate the projection matrix.
     */
    void set_viewport(float width, float height) noexcept;

    /**
     * @brief Set the camera center in logical coordinates.
     * @param x Camera center horizontal position.
     * @param y Camera center vertical position.
     */
    void set_position(float x, float y) noexcept;

    /**
     * @brief Set the zoom factor; values below a safe minimum are clamped.
     * @param zoom Magnification factor; clamped at a safe positive minimum.
     */
    void set_zoom(float zoom) noexcept;

    /** @brief Recalculate the projection matrix after property changes. */
    void update() noexcept;

    /**
     * @brief Read the current projection matrix.
     * @return Matrix combining view and projection transforms.
     */
    [[nodiscard]] const squared::math::Matrix4& combined() const noexcept;

    /**
     * @brief Read the camera center.
     * @return Center position in logical coordinates.
     */
    [[nodiscard]] squared::math::Vector2 position() const noexcept;

    /**
     * @brief Read the current zoom factor.
     * @return Active magnification factor.
     */
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
