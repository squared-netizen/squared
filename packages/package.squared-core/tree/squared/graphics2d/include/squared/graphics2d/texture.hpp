#pragma once

#include <squared/files/file_handle.hpp>
#include <squared/graphics/color.hpp>
#include <squared/graphics2d/texture_filter.hpp>
#include <squared/graphics2d/texture_recovery_callback.hpp>
#include <squared/graphics2d/texture_recovery_options.hpp>
#include <squared/graphics2d/texture_recovery_policy.hpp>
#include <squared/graphics2d/texture_wrap.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace sq::graphics {
class Context;
}  // namespace sq::graphics

namespace sq::graphics2d {

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
     * @param source Handle naming the encoded image. The handle is retained,
     * so the file system it came from must outlive this texture.
     * @return true when the texture loaded successfully.
     * @note A handle rather than a path because a path alone cannot be read
     * back: the handle carries its file system with it, which is what lets
     * restore() refill this texture after context loss without reaching into
     * the asset manager that created it.
     */
    [[nodiscard]] bool load(const files::FileHandle& source) noexcept;

    /**
     * @brief Load an asset with an explicit recovery policy.
     * @param source Handle naming the encoded image.
     * @param recovery Policy controlling post-context-loss recovery.
     * @return true when the texture loaded successfully.
     */
    [[nodiscard]] bool load(
        const files::FileHandle& source,
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
        sq::graphics::Color color
    ) noexcept;

    /**
     * @brief Create a solid texture with an explicit recovery policy.
     * @param color Normalized color for the single texel.
     * @param recovery Policy controlling post-context-loss recovery.
     * @return true when the texture storage was created.
     */
    [[nodiscard]] bool create_solid(
        sq::graphics::Color color,
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
    [[nodiscard]] bool restore(const graphics::Context& graphics) noexcept;

    /**
     * @brief Rebuild, stating explicitly whether the context survived.
     * @param context_preserved true when the GPU objects are still valid.
     *
     * @note No default. The previous one was `false`, which made `restore()`
     * read as cheap and behave destructively: it reloaded everything, moved
     * every texture's generation, and invalidated every TextureRegion copied
     * out of an atlas - on the first frame, before anything had been lost.
     *
     * @note Prefer the overload taking a Context. It reads the answer from
     * the object that knows it, so the polarity cannot be got backwards.
     */
    [[nodiscard]] bool restore(bool context_preserved) noexcept;

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

    /**
     * @brief Read the counter that changes whenever the content does.
     * @return A value that increments on every upload, restore, release and
     * invalidate.
     * @note This is what makes a stale TextureRegion detectable. A region
     * records the generation it was built against; when the two differ, the
     * region's coordinates no longer describe the pixels that are there, and
     * after a Discard there are no pixels at all. Without it, a region
     * outliving its texture's content is a crash after a phone call.
     */
    [[nodiscard]] std::uint32_t content_generation() const noexcept;

    /**
     * @brief Check whether the texture currently holds usable content.
     * @return true when a backend object exists and has not been released,
     * invalidated or discarded.
     */
    [[nodiscard]] bool has_content() const noexcept;

private:
    friend class SpriteBatch;
    friend class TextureRecoveryTarget;

    // Implemented per backend; everything else is backend-independent.
    [[nodiscard]] bool upload_rgba(
        int width, int height, const std::uint8_t* pixels
    ) noexcept;
    void destroy_backend_object() noexcept;
    void apply_sampling() noexcept;


    [[nodiscard]] bool restore_asset() noexcept;
    [[nodiscard]] bool restore_callback() noexcept;

    unsigned int handle_{0};
    int width_{0};
    int height_{0};
    // Matches TextureRecoveryOptions::policy. Every create call overwrites
    // this from the options it was given, so the value here is what an empty,
    // never-created texture reports.
    TextureRecoveryPolicy recovery_policy_{TextureRecoveryPolicy::ReloadFromAsset};
    files::FileHandle source_;
    std::vector<std::uint8_t> pixels_;
    TextureRecoveryCallback recovery_callback_{nullptr};
    void* recovery_user_data_{nullptr};
    TextureFilter minification_{TextureFilter::Nearest};
    TextureFilter magnification_{TextureFilter::Nearest};
    TextureWrap horizontal_wrap_{TextureWrap::ClampToEdge};
    TextureWrap vertical_wrap_{TextureWrap::ClampToEdge};
    std::uint32_t content_generation_{0};
    bool invalidated_{false};
};

} // namespace sq::graphics2d
