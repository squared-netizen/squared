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
    TextureRegion() noexcept = default;

    /** @brief Reference the complete texture. */
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

    /** @brief Return whether this region references a valid texture. */
    [[nodiscard]] bool valid() const noexcept
    {
        return texture_ && texture_->valid() && width_ > 0 && height_ > 0;
    }

    /** @brief Return the referenced texture. */
    [[nodiscard]] const Texture& texture() const noexcept
    {
        return *texture_;
    }

    /** @brief Return the region width in source pixels. */
    [[nodiscard]] int width() const noexcept { return width_; }

    /** @brief Return the region height in source pixels. */
    [[nodiscard]] int height() const noexcept { return height_; }

    /** @brief Return the left normalized texture coordinate. */
    [[nodiscard]] float u1() const noexcept { return u1_; }

    /** @brief Return the top normalized texture coordinate. */
    [[nodiscard]] float v1() const noexcept { return v1_; }

    /** @brief Return the right normalized texture coordinate. */
    [[nodiscard]] float u2() const noexcept { return u2_; }

    /** @brief Return the bottom normalized texture coordinate. */
    [[nodiscard]] float v2() const noexcept { return v2_; }

    /** @brief Return whether atlas storage is rotated 90 degrees clockwise. */
    [[nodiscard]] bool rotated_clockwise() const noexcept
    {
        return rotated_clockwise_;
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
