#pragma once

#include <squared/graphics/color.hpp>
#include <squared/graphics2d/texture_region.hpp>

namespace squared::graphics2d {

/**
 * @brief Lightweight mutable state for drawing one TextureRegion.
 */
class Sprite {
public:
    /**
     * @brief Construct a sprite whose size matches the region.
     * @param region Region referenced non-owningly; must outlive the sprite.
     * Initial width and height are set from the region.
     */
    explicit Sprite(const TextureRegion& region) noexcept
        : region_(&region),
          width_(static_cast<float>(region.width())),
          height_(static_cast<float>(region.height()))
    {
    }

    /**
     * @brief Set the top-left position in logical coordinates.
     * @param x Horizontal position.
     * @param y Vertical position.
     */
    void set_position(float x, float y) noexcept
    {
        x_ = x;
        y_ = y;
    }

    /**
     * @brief Set the unscaled logical dimensions.
     * @param width Unscaled width in logical pixels.
     * @param height Unscaled height in logical pixels.
     */
    void set_size(float width, float height) noexcept
    {
        width_ = width;
        height_ = height;
    }

    /**
     * @brief Set the transform origin relative to the top-left corner.
     * @param x Origin offset in logical pixels.
     * @param y Origin offset in logical pixels.
     */
    void set_origin(float x, float y) noexcept
    {
        origin_x_ = x;
        origin_y_ = y;
    }

    /**
     * @brief Set independent horizontal and vertical scale factors.
     * @param x Horizontal multiplier.
     * @param y Vertical multiplier.
     */
    void set_scale(float x, float y) noexcept
    {
        scale_x_ = x;
        scale_y_ = y;
    }

    /**
     * @brief Set clockwise rotation in top-left coordinates.
     * @param degrees Clockwise rotation in degrees.
     */
    void set_rotation(float degrees) noexcept
    {
        rotation_degrees_ = degrees;
    }

    /**
     * @brief Set the color multiplied with the sampled texture.
     * @param color Tint color; white preserves the texture unchanged.
     */
    void set_color(squared::graphics::Color color) noexcept
    {
        color_ = color;
    }

    /**
     * @brief Access the referenced texture region.
     * @return Reference to the region provided at construction.
     */
    [[nodiscard]] const TextureRegion& region() const noexcept
    {
        return *region_;
    }

    /**
     * @brief Read the horizontal position.
     * @return Current x position in logical pixels.
     */
    [[nodiscard]] float x() const noexcept { return x_; }

    /**
     * @brief Read the vertical position.
     * @return Current y position in logical pixels.
     */
    [[nodiscard]] float y() const noexcept { return y_; }

    /**
     * @brief Read the unscaled width.
     * @return Unscaled width in logical pixels.
     */
    [[nodiscard]] float width() const noexcept { return width_; }

    /**
     * @brief Read the unscaled height.
     * @return Unscaled height in logical pixels.
     */
    [[nodiscard]] float height() const noexcept { return height_; }

    /**
     * @brief Read the horizontal transform origin.
     * @return Origin offset in logical pixels.
     */
    [[nodiscard]] float origin_x() const noexcept { return origin_x_; }

    /**
     * @brief Read the vertical transform origin.
     * @return Origin offset in logical pixels.
     */
    [[nodiscard]] float origin_y() const noexcept { return origin_y_; }

    /**
     * @brief Read the horizontal scale factor.
     * @return Currently applied horizontal multiplier.
     */
    [[nodiscard]] float scale_x() const noexcept { return scale_x_; }

    /**
     * @brief Read the vertical scale factor.
     * @return Currently applied vertical multiplier.
     */
    [[nodiscard]] float scale_y() const noexcept { return scale_y_; }

    /**
     * @brief Read rotation in degrees.
     * @return Clockwise rotation in degrees.
     */
    [[nodiscard]] float rotation() const noexcept
    {
        return rotation_degrees_;
    }

    /**
     * @brief Read the current tint color.
     * @return Color used for tinting the sampled texture.
     */
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
