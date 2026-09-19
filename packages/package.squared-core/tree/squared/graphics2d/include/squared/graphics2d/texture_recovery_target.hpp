#pragma once

#include <cstdint>

namespace sq::graphics2d {

class Texture;

/**
 * @brief Synchronous destination supplied to a regeneration callback.
 *
 * Pixel memory only needs to remain valid for the duration of upload_rgba().
 */
class TextureRecoveryTarget final {
public:
    TextureRecoveryTarget(const TextureRecoveryTarget&) = delete;
    TextureRecoveryTarget& operator=(const TextureRecoveryTarget&) = delete;

    /**
     * @brief Upload one tightly packed RGBA8888 image synchronously.
     * @param width Image width in pixels; must be greater than zero.
     * @param height Image height in pixels; must be greater than zero.
     * @param pixels Tightly packed RGBA8888 source; must be non-null and
     * hold width * height * 4 bytes, valid for the duration of the call.
     * @return true when the upload replaced the recovered texture.
     */
    [[nodiscard]] bool upload_rgba(
        int width,
        int height,
        const std::uint8_t* pixels
    ) noexcept;

private:
    friend class Texture;
    explicit TextureRecoveryTarget(Texture& texture) noexcept;

    Texture* texture_{nullptr};
    bool uploaded_{false};
};

} // namespace sq::graphics2d
