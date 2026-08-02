#pragma once

#include <squared/graphics/color.hpp>
#include <squared/graphics2d/texture_region.hpp>

namespace squared::graphics2d {

/**
 * @brief Lightweight mutable state for drawing one TextureRegion.
 */
class Sprite {
public:
    explicit Sprite(const TextureRegion& region) noexcept
        : region_(&region),
          width_(static_cast<float>(region.width())),
          height_(static_cast<float>(region.height()))
    {
    }

    /** @brief Set the top-left position in logical coordinates. */
    void set_position(float x, float y) noexcept
    {
        x_ = x;
        y_ = y;
    }

    /** @brief Set the unscaled logical dimensions. */
    void set_size(float width, float height) noexcept
    {
        width_ = width;
        height_ = height;
    }

    /** @brief Set the transform origin relative to the top-left corner. */
    void set_origin(float x, float y) noexcept
    {
        origin_x_ = x;
        origin_y_ = y;
    }

    /** @brief Set independent horizontal and vertical scale factors. */
    void set_scale(float x, float y) noexcept
    {
        scale_x_ = x;
        scale_y_ = y;
    }

    /** @brief Set clockwise rotation in top-left coordinates. */
    void set_rotation(float degrees) noexcept
    {
        rotation_degrees_ = degrees;
    }

    /** @brief Set the color multiplied with the sampled texture. */
    void set_color(squared::graphics::Color color) noexcept
    {
        color_ = color;
    }

    /** @brief Return the referenced texture region. */
    [[nodiscard]] const TextureRegion& region() const noexcept
    {
        return *region_;
    }

    /** @brief Return the horizontal position. */
    [[nodiscard]] float x() const noexcept { return x_; }

    /** @brief Return the vertical position. */
    [[nodiscard]] float y() const noexcept { return y_; }

    /** @brief Return the unscaled width. */
    [[nodiscard]] float width() const noexcept { return width_; }

    /** @brief Return the unscaled height. */
    [[nodiscard]] float height() const noexcept { return height_; }

    /** @brief Return the horizontal transform origin. */
    [[nodiscard]] float origin_x() const noexcept { return origin_x_; }

    /** @brief Return the vertical transform origin. */
    [[nodiscard]] float origin_y() const noexcept { return origin_y_; }

    /** @brief Return the horizontal scale factor. */
    [[nodiscard]] float scale_x() const noexcept { return scale_x_; }

    /** @brief Return the vertical scale factor. */
    [[nodiscard]] float scale_y() const noexcept { return scale_y_; }

    /** @brief Return rotation in degrees. */
    [[nodiscard]] float rotation() const noexcept
    {
        return rotation_degrees_;
    }

    /** @brief Return the current tint color. */
    [[nodiscard]] squared::graphics::Color color() const noexcept
    {
        return color_;
    }

private:
    const TextureRegion* region_;
    float x_{0.0F};
    float y_{0.0F};
    float width_{0.0F};
    float height_{0.0F};
    float origin_x_{0.0F};
    float origin_y_{0.0F};
    float scale_x_{1.0F};
    float scale_y_{1.0F};
    float rotation_degrees_{0.0F};
    squared::graphics::Color color_;
};

}  // namespace squared::graphics2d
