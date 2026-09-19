#include "detail/image_decode.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

// Declarations only. The implementation is instantiated once in
// third_party/stb/stb_image_impl.c, which is compiled with warnings off:
// stb_image is not ours to keep clean, and squared's -Wconversion would bury
// a real warning under hundreds from it.
#define STBI_NO_STDIO
#include <stb_image.h>

namespace sq::graphics2d::detail {

namespace {

/** @brief Build a failed result without naming every field at each site. */
DecodedImage failure(std::string message)
{
    DecodedImage image;
    image.message = std::move(message);
    return image;
}

}  // namespace


DecodedImage decode_image(
    std::span<const std::byte> bytes,
    std::size_t maximum_pixels
)
{
    if (bytes.empty()) {
        return failure("image data is empty");
    }
    if (bytes.size() > static_cast<std::size_t>(INT32_MAX)) {
        return failure("image data is too large to decode");
    }

    const auto* data = reinterpret_cast<const stbi_uc*>(bytes.data());
    const auto length = static_cast<int>(bytes.size());

    // Read the header first so an absurd size is rejected before anything is
    // allocated for it.
    int width = 0;
    int height = 0;
    int channels = 0;
    if (stbi_info_from_memory(data, length, &width, &height, &channels) == 0) {
        return failure(stbi_failure_reason() != nullptr ? stbi_failure_reason() : "not a recognised image");
    }
    if (width <= 0 || height <= 0) {
        return failure("image reports a zero dimension");
    }
    if (maximum_pixels != 0) {
        const auto pixels = static_cast<std::size_t>(width)
            * static_cast<std::size_t>(height);
        if (pixels > maximum_pixels) {
            return failure("image exceeds the configured pixel limit");
        }
    }

    // Four channels always: every consumer above wants RGBA8888.
    stbi_uc* decoded = stbi_load_from_memory(
        data, length, &width, &height, &channels, 4
    );
    if (decoded == nullptr) {
        return failure(stbi_failure_reason() != nullptr ? stbi_failure_reason() : "decode failed");
    }

    const auto count = static_cast<std::size_t>(width)
        * static_cast<std::size_t>(height) * 4U;

    DecodedImage image;
    image.pixels.assign(decoded, decoded + count);
    image.width = width;
    image.height = height;

    stbi_image_free(decoded);
    return image;
}

}  // namespace sq::graphics2d::detail
