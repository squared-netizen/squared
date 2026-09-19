#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace sq::graphics2d::detail {

/** @brief A decoded image in tightly packed RGBA8888. */
struct DecodedImage final {
    /** @brief Pixels, width * height * 4 bytes, top row first. */
    std::vector<std::uint8_t> pixels;

    /** @brief Width in pixels; zero on failure. */
    int width{0};

    /** @brief Height in pixels; zero on failure. */
    int height{0};

    /** @brief Decoder message; empty on success. */
    std::string message;

    /** @brief Return whether the image decoded. */
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return width > 0 && height > 0;
    }
};

/**
 * @brief Decode an encoded image to RGBA8888.
 * @param bytes Encoded file contents; PNG today.
 * @param maximum_pixels Reject an image with more pixels than this, before
 * allocating for it. Zero disables the check.
 * @return The decoded image, or a DecodedImage carrying the decoder's message.
 *
 * @note Always produces four channels, whatever the source had, because every
 * path above this wants RGBA8888 and converting once here is cheaper than
 * every caller handling three formats.
 *
 * @note The dimension check happens before the decode allocates. A malicious
 * or corrupt PNG header can claim 65535x65535, which is 17 GB of RGBA; finding
 * that out after the allocation is too late on a phone.
 *
 * @note Internal to the squared build. Not installed, not public API: there is
 * one consumer today, and the extension policy says a second concrete use
 * comes before a public interface.
 */
[[nodiscard]] DecodedImage decode_image(
    std::span<const std::byte> bytes,
    std::size_t maximum_pixels = 0
);

}  // namespace sq::graphics2d::detail
