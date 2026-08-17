#pragma once

#include <squared/graphics2d/texture.hpp>

namespace squared::graphics2d {

/**
 * @brief Non-owning rectangular view into a Texture.
 *
 * The referenced Texture must outlive the region and every draw operation
 * that uses it.
 */
class TextureRegion {
public:
    /** @brief Construct an empty, invalid region. */
    TextureRegion() noexcept = default;

    /**
     * @brief Create an unbacked logical region for recording and validation.
     *
     * The region has dimensions and normalized coordinates but is not valid
     * for GPU rendering because it references no Texture.
     * @param width Logical width in pixels; clamped to non-negative.
     * @param height Logical height in pixels; clamped to non-negative.
     * @param rotated_clockwise Whether atlas storage is rotated 90 degrees
     * clockwise.
     */
    TextureRegion(int width, int height, bool rotated_clockwise = false) noexcept
        : width_(width > 0 ? width : 0),
          height_(height > 0 ? height : 0),
          rotated_clockwise_(rotated_clockwise)
    {
    }

    /**
     * @brief Reference the complete texture.
     * @param texture Texture shared non-owningly; must outlive the region.
     */
    explicit TextureRegion(const Texture& texture) noexcept
        : texture_(&texture),
          width_(texture.width()),
          height_(texture.height())
    {
    }

    /**
     * @brief Reference a pixel rectangle using top-left image coordinates.
     *
     * For clockwise-packed atlas entries, width and height describe the
     * logical unrotated region; their storage extents are swapped.
     * @param texture Texture shared non-owningly; must outlive the region.
     * @param x Left rect coordinate in source pixels.
     * @param y Top rect coordinate in source pixels.
     * @param width Logical width in source pixels.
     * @param height Logical height in source pixels.
     * @param rotated_clockwise Whether atlas storage is rotated.
     */
    TextureRegion(
        const Texture& texture,
        int x,
        int y,
        int width,
        int height,
        bool rotated_clockwise = false
    ) noexcept
        : texture_(&texture),
          width_(width),
          height_(height),
          rotated_clockwise_(rotated_clockwise)
    {
        const float texture_width =
            static_cast<float>(texture.width());
        const float texture_height =
            static_cast<float>(texture.height());
        if (texture_width > 0.0F && texture_height > 0.0F) {
            u1_ = static_cast<float>(x) / texture_width;
            v1_ = static_cast<float>(y) / texture_height;
            u2_ = static_cast<float>(
                x + (rotated_clockwise ? height : width)
            ) / texture_width;
            v2_ = static_cast<float>(
                y + (rotated_clockwise ? width : height)
            ) / texture_height;
        }
    }

    /**
     * @brief Check whether this region can be rendered.
     * @return true when a valid texture is referenced with positive size.
     */
    [[nodiscard]] bool valid() const noexcept
    {
        return texture_ && texture_->valid() && width_ > 0 && height_ > 0;
    }

    /**
     * @brief Access the referenced texture.
     * @return Reference to the texture; undefined when valid() is false.
     */
    [[nodiscard]] const Texture& texture() const noexcept
    {
        return *texture_;
    }

    /**
     * @brief Read the region width.
     * @return Width in source pixels.
     */
    [[nodiscard]] int width() const noexcept { return width_; }

    /**
     * @brief Read the region height.
     * @return Height in source pixels.
     */
    [[nodiscard]] int height() const noexcept { return height_; }

    /**
     * @brief Read the left normalized texture coordinate.
     * @return U coordinate in the inclusive range [0, 1].
     */
    [[nodiscard]] float u1() const noexcept { return u1_; }

    /**
     * @brief Read the top normalized texture coordinate.
     * @return V coordinate in the inclusive range [0, 1].
     */
    [[nodiscard]] float v1() const noexcept { return v1_; }

    /**
     * @brief Read the right normalized texture coordinate.
     * @return U coordinate in the inclusive range [0, 1].
     */
    [[nodiscard]] float u2() const noexcept { return u2_; }

    /**
     * @brief Read the bottom normalized texture coordinate.
     * @return V coordinate in the inclusive range [0, 1].
     */
    [[nodiscard]] float v2() const noexcept { return v2_; }

    /**
     * @brief Report whether atlas storage is rotated.
     * @return true when storage is rotated 90 degrees clockwise.
     */
    [[nodiscard]] bool rotated_clockwise() const noexcept
    {
        return rotated_clockwise_;
    }

    /**
     * @brief Return a logical subregion using top-left coordinates.
     *
     * The returned view references the same texture and preserves atlas
     * rotation. Invalid bounds return an empty region.
     * @param x Left coordinate in logical pixels.
     * @param y Top coordinate in logical pixels.
     * @param width Subregion width in logical pixels.
     * @param height Subregion height in logical pixels.
     * @return A non-owning subregion view, or an empty region when the
     * requested bounds fall outside this region.
     */
    [[nodiscard]] TextureRegion subregion(
        int x,
        int y,
        int width,
        int height
    ) const noexcept
    {
        if (x < 0 || y < 0 || width <= 0 || height <= 0 ||
            x + width > width_ || y + height > height_) {
            return {};
        }

        TextureRegion result;
        result.texture_ = texture_;
        result.width_ = width;
        result.height_ = height;
        result.rotated_clockwise_ = rotated_clockwise_;
        const float u_extent = u2_ - u1_;
        const float v_extent = v2_ - v1_;
        if (!rotated_clockwise_) {
            result.u1_ = u1_ + u_extent * static_cast<float>(x) /
                static_cast<float>(width_);
            result.u2_ = u1_ + u_extent * static_cast<float>(x + width) /
                static_cast<float>(width_);
            result.v1_ = v1_ + v_extent * static_cast<float>(y) /
                static_cast<float>(height_);
            result.v2_ = v1_ + v_extent * static_cast<float>(y + height) /
                static_cast<float>(height_);
        } else {
            result.u1_ = u2_ - u_extent * static_cast<float>(y + height) /
                static_cast<float>(height_);
            result.u2_ = u2_ - u_extent * static_cast<float>(y) /
                static_cast<float>(height_);
            result.v1_ = v1_ + v_extent * static_cast<float>(x) /
                static_cast<float>(width_);
            result.v2_ = v1_ + v_extent * static_cast<float>(x + width) /
                static_cast<float>(width_);
        }
        return result;
    }

private:
    const Texture* texture_{nullptr};
    int width_{0};
    int height_{0};
    float u1_{0.0F};
    float v1_{0.0F};
    float u2_{1.0F};
    float v2_{1.0F};
    bool rotated_clockwise_{false};
};

}  // namespace squared::graphics2d
