#pragma once

#include <algorithm>
#include <cstdint>

namespace squared::graphics {

/**
 * @brief Normalized four-component color used by Squared graphics APIs.
 */
struct Color {
    /** @brief Red component from zero to one. */
    float red{1.0F};
    /** @brief Green component from zero to one. */
    float green{1.0F};
    /** @brief Blue component from zero to one. */
    float blue{1.0F};
    /** @brief Alpha component from zero to one. */
    float alpha{1.0F};

    /**
     * @brief Construct opaque white.
     * @return A color with every component at one.
     */
    static constexpr Color white() noexcept
    {
        return {};
    }

    /**
     * @brief Construct fully transparent black.
     * @return A color with every component at zero.
     */
    static constexpr Color transparent() noexcept
    {
        return {0.0F, 0.0F, 0.0F, 0.0F};
    }

    /**
     * @brief Convert byte components to normalized floating-point values.
     * @param red_value Red byte value in the inclusive range [0, 255].
     * @param green_value Green byte value in the inclusive range [0, 255].
     * @param blue_value Blue byte value in the inclusive range [0, 255].
     * @param alpha_value Alpha byte value in the inclusive range [0, 255].
     * @return A color with each component divided by 255.
     */
    static constexpr Color from_rgba8(
        std::uint8_t red_value,
        std::uint8_t green_value,
        std::uint8_t blue_value,
        std::uint8_t alpha_value = 255
    ) noexcept
    {
        constexpr float divisor = 255.0F;
        return {
            red_value / divisor,
            green_value / divisor,
            blue_value / divisor,
            alpha_value / divisor
        };
    }

    /**
     * @brief Return a copy with every component clamped to `[0, 1]`.
     * @return A color whose components never exceed the normalized range.
     */
    [[nodiscard]] constexpr Color clamped() const noexcept
    {
        return {
            std::clamp(red, 0.0F, 1.0F),
            std::clamp(green, 0.0F, 1.0F),
            std::clamp(blue, 0.0F, 1.0F),
            std::clamp(alpha, 0.0F, 1.0F)
        };
    }
};

}  // namespace squared::graphics
