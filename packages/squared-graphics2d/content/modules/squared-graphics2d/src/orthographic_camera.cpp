#include <squared/graphics2d/orthographic_camera.hpp>

#include <algorithm>

namespace squared::graphics2d {

OrthographicCamera::OrthographicCamera(
    float viewport_width,
    float viewport_height,
    CoordinateOrigin origin
) noexcept
    : viewport_width_(viewport_width),
      viewport_height_(viewport_height),
      position_{viewport_width * 0.5F, viewport_height * 0.5F},
      origin_(origin)
{
    update();
}

void OrthographicCamera::set_viewport(
    float width,
    float height
) noexcept
{
    viewport_width_ = std::max(width, 1.0F);
    viewport_height_ = std::max(height, 1.0F);
}

void OrthographicCamera::set_position(float x, float y) noexcept
{
    position_ = {x, y};
}

void OrthographicCamera::set_zoom(float zoom) noexcept
{
    zoom_ = std::max(zoom, 0.0001F);
}

void OrthographicCamera::update() noexcept
{
    const float half_width = viewport_width_ * zoom_ * 0.5F;
    const float half_height = viewport_height_ * zoom_ * 0.5F;
    const float left = position_.x - half_width;
    const float right = position_.x + half_width;

    if (origin_ == CoordinateOrigin::TopLeft) {
        combined_ = squared::math::Matrix4::orthographic(
            left,
            right,
            position_.y + half_height,
            position_.y - half_height
        );
    } else {
        combined_ = squared::math::Matrix4::orthographic(
            left,
            right,
            position_.y - half_height,
            position_.y + half_height
        );
    }
}

const squared::math::Matrix4&
OrthographicCamera::combined() const noexcept
{
    return combined_;
}

squared::math::Vector2 OrthographicCamera::position() const noexcept
{
    return position_;
}

float OrthographicCamera::zoom() const noexcept
{
    return zoom_;
}

}  // namespace squared::graphics2d
