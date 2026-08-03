#pragma once

#include <squared/graphics/color.hpp>

#include <cstdint>

namespace squared::graphics2d {

/**
 * @brief Texture filtering mode.
 */
enum class TextureFilter {
    Nearest,
    Linear
};

/**
 * @brief Texture coordinate wrapping mode.
 */
enum class TextureWrap {
    ClampToEdge,
    Repeat,
    MirroredRepeat
};

/**
 * @brief Move-only texture owned by the selected graphics backend.
 *
 * A valid graphics context must be current whenever a texture is created or
 * destroyed. Automatic context-loss restoration is intentionally deferred
 * until the asset manager is introduced.
 */
class Texture final {
public:
    Texture() noexcept = default;
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;
    ~Texture();

    /**
     * @brief Load an image asset through the selected backend.
     */
    [[nodiscard]] bool load(const char* asset_path) noexcept;

    /**
     * @brief Create an RGBA8888 texture from tightly packed pixels.
     */
    [[nodiscard]] bool create_rgba(
        int width,
        int height,
        const std::uint8_t* pixels
    ) noexcept;

    /** @brief Create a one-pixel texture containing a solid color. */
    [[nodiscard]] bool create_solid(
        squared::graphics::Color color
    ) noexcept;

    /** @brief Destroy the backend texture. */
    void destroy() noexcept;

    /** @brief Set minification and magnification filters. */
    void set_filter(
        TextureFilter minification,
        TextureFilter magnification
    ) noexcept;

    /** @brief Set horizontal and vertical texture wrapping. */
    void set_wrap(
        TextureWrap horizontal,
        TextureWrap vertical
    ) noexcept;

    /** @brief Bind the texture to a zero-based texture unit. */
    void bind(unsigned int unit = 0) const noexcept;

    /** @brief Return whether the texture owns a backend object. */
    [[nodiscard]] bool valid() const noexcept;

    /** @brief Return the texture width in pixels. */
    [[nodiscard]] int width() const noexcept;

    /** @brief Return the texture height in pixels. */
    [[nodiscard]] int height() const noexcept;

private:
    friend class SpriteBatch;

    unsigned int handle_{0};
    int width_{0};
    int height_{0};
};

}  // namespace squared::graphics2d
