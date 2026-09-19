#pragma once

#include <squared/graphics2d/texture_recovery_callback.hpp>
#include <squared/graphics2d/texture_recovery_policy.hpp>

namespace sq::graphics2d {

/** @brief Recovery selection supplied when a texture is created. */
struct TextureRecoveryOptions final {
    /**
     * @brief Selected portable recovery policy.
     *
     * Defaults to ReloadFromAsset: the cheapest policy is the one you get by
     * not thinking about it, and the expensive ones have to be typed.
     * RetainPixels would be the safest default and also the one that doubles
     * the resident cost of every texture in the application, which priority 1
     * does not permit as a default.
     *
     * @note This default only recovers a texture that has an asset behind it.
     * A texture created from memory with this policy has no source to reload
     * from, and the creating call reports that rather than producing something
     * that will silently fail to come back after a phone call.
     */
    TextureRecoveryPolicy policy{TextureRecoveryPolicy::ReloadFromAsset};

    /** @brief Callback invoked to regenerate pixels after context loss. */
    TextureRecoveryCallback callback{nullptr};

    /** @brief Opaque value forwarded to callback; non-owning. */
    void* user_data{nullptr};

    /**
     * @brief Build options that reload from the asset path.
     * @return Options with the ReloadFromAsset policy.
     */
    [[nodiscard]] static constexpr TextureRecoveryOptions
    reload_from_asset() noexcept
    {
        return {TextureRecoveryPolicy::ReloadFromAsset, nullptr, nullptr};
    }

    /**
     * @brief Build options that retain CPU pixels for GPU re-upload.
     * @return Options with the RetainPixels policy.
     */
    [[nodiscard]] static constexpr TextureRecoveryOptions
    retain_pixels() noexcept
    {
        return {TextureRecoveryPolicy::RetainPixels, nullptr, nullptr};
    }

    /**
     * @brief Build options that regenerate pixels through a callback.
     * @param callback Callback restoring pixel content; must remain valid
     * for the life of the texture. Non-owning.
     * @param user_data Opaque value forwarded to callback; non-owning.
     * @return Options with the Regenerate policy.
     */
    [[nodiscard]] static constexpr TextureRecoveryOptions regenerate(
        TextureRecoveryCallback callback,
        void* user_data = nullptr
    ) noexcept
    {
        return {TextureRecoveryPolicy::Regenerate, callback, user_data};
    }

    /**
     * @brief Build options that discard the texture after context loss.
     * @return Options with the Discard policy.
     */
    [[nodiscard]] static constexpr TextureRecoveryOptions discard() noexcept
    {
        return {TextureRecoveryPolicy::Discard, nullptr, nullptr};
    }
};

} // namespace sq::graphics2d
