#pragma once

#include <squared/gles/gles_error.hpp>
#include <squared/gles/texture_filter.hpp>
#include <squared/gles/texture_format.hpp>
#include <squared/gles/texture_wrap.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

namespace sq::gles {

/**
 * @brief One 2D GL texture, owned.
 *
 * 16 bytes: a name, dimensions and a format. **No pixel data is retained.**
 * That is the priority-1 decision at the heart of this class: keeping a CPU
 * copy would make recovery after context loss automatic and would double the
 * resident cost of every texture in the application.
 *
 * Recovery is the caller's business, and squared::graphics2d::Texture is where
 * the policy for it lives. This class only offers what recovery needs:
 * upload() to refill, and invalidate() to forget a name whose context is gone.
 */
class Texture2D final {
public:
    Texture2D() noexcept = default;
    ~Texture2D();

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;
    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;

    /**
     * @brief Allocate storage and optionally fill it.
     * @param width Width in pixels; must be positive.
     * @param height Height in pixels; must be positive.
     * @param format Pixel format of the storage.
     * @param pixels Initial contents, tightly packed, or empty to allocate
     * uninitialised storage.
     * @return A no-error GlesError on success.
     * @note Row alignment is set to 1 before upload. GL defaults to 4, which
     * corrupts any R8 texture whose width is not a multiple of four &mdash;
     * the classic skewed-font-atlas bug.
     */
    [[nodiscard]] GlesError create(
        int width,
        int height,
        TextureFormat format,
        std::span<const std::byte> pixels = {}
    ) noexcept;

    /**
     * @brief Replace the whole contents, keeping the same storage.
     * @param pixels Tightly packed pixels matching the existing size and
     * format.
     * @return A no-error GlesError on success.
     */
    [[nodiscard]] GlesError upload(std::span<const std::byte> pixels) noexcept;

    /**
     * @brief Replace a rectangle of the contents.
     * @param x Left edge in pixels.
     * @param y Top edge in pixels.
     * @param width Width of the rectangle.
     * @param height Height of the rectangle.
     * @param pixels Tightly packed pixels for the rectangle.
     * @return A no-error GlesError on success.
     */
    [[nodiscard]] GlesError upload_region(
        int x,
        int y,
        int width,
        int height,
        std::span<const std::byte> pixels
    ) noexcept;

    /**
     * @brief Set minification, magnification and wrapping.
     * @param minify Filter used when the texture is drawn smaller.
     * @param magnify Filter used when it is drawn larger.
     * @param wrap Behaviour outside the zero-to-one range.
     * @return A no-error GlesError on success.
     */
    [[nodiscard]] GlesError set_sampling(
        TextureFilter minify,
        TextureFilter magnify,
        TextureWrap wrap
    ) noexcept;

    /**
     * @brief Bind to a texture unit.
     * @param unit Unit index; the value a sampler uniform is set to.
     */
    void bind(int unit = 0) const noexcept;

    /** @brief Release the GL object. Safe to call more than once. */
    void destroy() noexcept;

    /** @brief Forget the GL name without deleting it, after context loss. */
    void invalidate() noexcept;

    /** @brief Report whether this holds a GL object. */
    [[nodiscard]] bool valid() const noexcept { return name_ != 0; }

    /** @brief Return the raw GL name, or zero. */
    [[nodiscard]] std::uint32_t name() const noexcept { return name_; }

    /** @brief Return the width in pixels. */
    [[nodiscard]] int width() const noexcept { return width_; }

    /** @brief Return the height in pixels. */
    [[nodiscard]] int height() const noexcept { return height_; }

    /** @brief Return the storage format. */
    [[nodiscard]] TextureFormat format() const noexcept { return format_; }

    /** @brief Return the bytes one pixel of a format occupies. */
    [[nodiscard]] static int bytes_per_pixel(TextureFormat format) noexcept;

private:
    std::uint32_t name_{0};
    int width_{0};
    int height_{0};
    TextureFormat format_{TextureFormat::Rgba8};
};

}  // namespace sq::gles
