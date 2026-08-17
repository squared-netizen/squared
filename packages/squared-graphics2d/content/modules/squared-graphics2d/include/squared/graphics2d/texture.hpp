#pragma once

#include <squared/graphics/color.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

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
 * @brief CPU-side recipe used to recover a texture after context loss.
 *
 * Policies are backend-neutral. They describe what source data Squared keeps
 * or asks the application to regenerate; they do not expose the selected
 * windowing or graphics API.
 */
enum class TextureRecoveryPolicy {
    ReloadFromAsset,
    RetainPixels,
    Regenerate,
    Discard
};

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

using TextureRecoveryCallback = bool (*)(
    void* user_data,
    TextureRecoveryTarget& target
) noexcept;

/** @brief Recovery selection supplied when a texture is created. */
struct TextureRecoveryOptions final {
    /** @brief Selected portable recovery policy. */
    TextureRecoveryPolicy policy{TextureRecoveryPolicy::RetainPixels};

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

/**
 * @brief Move-only texture owned by the selected graphics backend.
 *
 * A valid graphics context must be current whenever a texture is created,
 * released, restored, or destroyed. Asset paths and RGBA source data are
 * retained according to the selected portable recovery policy.
 */
class Texture final {
public:
    /** @brief Construct an empty texture owning no backend object. */
    Texture() noexcept = default;
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    /**
     * @brief Move-construct from another texture.
     * @param other Texture to assume; left empty afterward.
     */
    Texture(Texture&& other) noexcept;

    /**
     * @brief Move-assign from another texture.
     * @param other Texture to assume; left empty afterward.
     * @return Reference to this texture.
     * @note Releases this texture's backend object first, if any.
     */
    Texture& operator=(Texture&& other) noexcept;

    /** @brief Destroy the backend object, if any. */
    ~Texture();

    /**
     * @brief Load an image asset through the selected backend.
     * @param asset_path Null-terminated path of the image asset.
     * @return true when the texture loaded successfully.
     */
    [[nodiscard]] bool load(const char* asset_path) noexcept;

    /**
     * @brief Load an asset with an explicit recovery policy.
     * @param asset_path Null-terminated path of the image asset.
     * @param recovery Policy controlling post-context-loss recovery.
     * @return true when the texture loaded successfully.
     */
    [[nodiscard]] bool load(
        const char* asset_path,
        TextureRecoveryOptions recovery
    ) noexcept;

    /**
     * @brief Create an RGBA8888 texture from tightly packed pixels.
     * @param width Image width in pixels; must be greater than zero.
     * @param height Image height in pixels; must be greater than zero.
     * @param pixels Tightly packed RGBA8888 source, width * height * 4 bytes.
     * @return true when the texture storage was created.
     */
    [[nodiscard]] bool create_rgba(
        int width,
        int height,
        const std::uint8_t* pixels
    ) noexcept;

    /**
     * @brief Create RGBA8888 storage with an explicit recovery policy.
     * @param width Image width in pixels; must be greater than zero.
     * @param height Image height in pixels; must be greater than zero.
     * @param pixels Tightly packed RGBA8888 source, width * height * 4 bytes.
     * @param recovery Policy controlling post-context-loss recovery.
     * @return true when the texture storage was created.
     */
    [[nodiscard]] bool create_rgba(
        int width,
        int height,
        const std::uint8_t* pixels,
        TextureRecoveryOptions recovery
    ) noexcept;

    /**
     * @brief Create a one-pixel texture containing a solid color.
     * @param color Normalized color for the single texel.
     * @return true when the texture storage was created.
     */
    [[nodiscard]] bool create_solid(
        squared::graphics::Color color
    ) noexcept;

    /**
     * @brief Create a solid texture with an explicit recovery policy.
     * @param color Normalized color for the single texel.
     * @param recovery Policy controlling post-context-loss recovery.
     * @return true when the texture storage was created.
     */
    [[nodiscard]] bool create_solid(
        squared::graphics::Color color,
        TextureRecoveryOptions recovery
    ) noexcept;

    /** @brief Destroy the backend texture. */
    void destroy() noexcept;

    /**
     * @brief Release only the GPU object while retaining its restoration recipe.
     */
    void release() noexcept;

    /**
     * @brief Mark the GPU object stale without issuing graphics calls.
     */
    void invalidate() noexcept;

    /**
     * @brief Recreate a released GPU object in the current graphics context.
     * @param context_preserved Whether the GPU context survived loss; true
     * skips recomputing fast-path state where possible.
     * @return true when the backend object was restored.
     */
    [[nodiscard]] bool restore(bool context_preserved = false) noexcept;

    /**
     * @brief Check whether restoration source data is available.
     * @return true when this texture has enough source data to restore.
     */
    [[nodiscard]] bool restorable() const noexcept;

    /**
     * @brief Read the configured recovery policy.
     * @return Policy selected at creation time.
     */
    [[nodiscard]] TextureRecoveryPolicy recovery_policy() const noexcept;

    /**
     * @brief Measure retained recovery data.
     * @return Bytes retained solely as an RGBA recovery recipe.
     */
    [[nodiscard]] std::size_t retained_recovery_bytes() const noexcept;

    /**
     * @brief Set minification and magnification filters.
     * @param minification Texture-wide minification filter.
     * @param magnification Texture-wide magnification filter.
     */
    void set_filter(
        TextureFilter minification,
        TextureFilter magnification
    ) noexcept;

    /**
     * @brief Set horizontal and vertical texture wrapping.
     * @param horizontal Texture-wide horizontal wrap mode.
     * @param vertical Texture-wide vertical wrap mode.
     */
    void set_wrap(
        TextureWrap horizontal,
        TextureWrap vertical
    ) noexcept;

    /**
     * @brief Bind the texture to a zero-based texture unit.
     * @param unit Zero-based texture unit to bind into.
     */
    void bind(unsigned int unit = 0) const noexcept;

    /**
     * @brief Check whether this texture owns a backend object.
     * @return true while the texture owns a valid backend object.
     */
    [[nodiscard]] bool valid() const noexcept;

    /**
     * @brief Read the texture width.
     * @return Width in pixels; zero when no backend object exists.
     */
    [[nodiscard]] int width() const noexcept;

    /**
     * @brief Read the texture height.
     * @return Height in pixels; zero when no backend object exists.
     */
    [[nodiscard]] int height() const noexcept;

private:
    friend class SpriteBatch;
    friend class TextureRecoveryTarget;

    [[nodiscard]] bool upload_rgba(
        int width, int height, const std::uint8_t* pixels
    ) noexcept;
    [[nodiscard]] bool restore_asset() noexcept;
    [[nodiscard]] bool restore_callback() noexcept;

    unsigned int handle_{0};
    int width_{0};
    int height_{0};
    TextureRecoveryPolicy recovery_policy_{TextureRecoveryPolicy::Discard};
    std::string asset_path_;
    std::vector<std::uint8_t> pixels_;
    TextureRecoveryCallback recovery_callback_{nullptr};
    void* recovery_user_data_{nullptr};
    TextureFilter minification_{TextureFilter::Nearest};
    TextureFilter magnification_{TextureFilter::Nearest};
    TextureWrap horizontal_wrap_{TextureWrap::ClampToEdge};
    TextureWrap vertical_wrap_{TextureWrap::ClampToEdge};
    bool invalidated_{false};
};

}  // namespace squared::graphics2d
