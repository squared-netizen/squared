#pragma once

#include <squared/graphics/color.hpp>

#include <cstddef>
#include <vector>

namespace sq::graphics {
class Context;
}  // namespace sq::graphics

namespace sq::graphics2d {

class OrthographicCamera;
class Sprite;
class Texture;
class TextureRegion;

/**
 * @brief Efficiently draw ordered textured quads with the selected backend.
 */
class SpriteBatch final {
public:
    /** @brief Construct a batch owning no backend objects. */
    SpriteBatch() noexcept = default;
    SpriteBatch(const SpriteBatch&) = delete;
    SpriteBatch& operator=(const SpriteBatch&) = delete;
    SpriteBatch(SpriteBatch&&) = delete;
    SpriteBatch& operator=(SpriteBatch&&) = delete;

    /** @brief Destroy all owned backend objects. */
    ~SpriteBatch();

    /**
     * @brief Allocate GPU buffers and compile the built-in sprite shader.
     *
     * @param maximum_sprites Maximum sprites buffered before an automatic
     * flush.
     * @return true when backend initialization succeeded.
     */
    [[nodiscard]] bool initialize(
        std::size_t maximum_sprites = 2048
    ) noexcept;

    /** @brief Release all owned backend objects. */
    void destroy() noexcept;

    /**
     * @brief Release GPU objects while preserving batch capacity for restoration.
     */
    void release() noexcept;

    /**
     * @brief Mark shaders and buffers stale without issuing graphics calls.
     */
    void invalidate() noexcept;

    /**
     * @brief Rebuild released shaders and buffers in the current context.
     * @param context_preserved Whether the GPU context survived loss; true
     * skips recomputing fast-path state where possible.
     * @return true when backend objects were restored.
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
     * @brief Begin an ordered batch using the camera projection.
     * @param camera Camera whose projection matrix drives the batch.
     * @return true when the batch started; false if not initialized.
     */
    [[nodiscard]] bool begin(
        const OrthographicCamera& camera
    ) noexcept;

    /**
     * @brief Draw one region without rotation.
     * @param region Region to sample; must remain valid for the batch.
     * @param x Horizontal position in logical pixels.
     * @param y Vertical position in logical pixels.
     * @param width Drawn width in logical pixels.
     * @param height Drawn height in logical pixels.
     * @param color Tint multiplied with the sampled texture; defaults to
     * opaque white.
     */
    void draw(
        const TextureRegion& region,
        float x,
        float y,
        float width,
        float height,
        sq::graphics::Color color =
            sq::graphics::Color::white()
    ) noexcept;

    /**
     * @brief Draw one transformed Sprite.
     * @param sprite Sprite whose region and transform are drawn.
     */
    void draw(const Sprite& sprite) noexcept;

    /** @brief Flush queued sprites and finish the batch. */
    void end() noexcept;

    /** @brief Flush queued sprites without ending the batch. */
    void flush() noexcept;

    /**
     * @brief Restrict drawing to a rectangle of the framebuffer.
     * @param x Left edge in framebuffer pixels.
     * @param y Bottom edge in framebuffer pixels, measured from the bottom.
     * @param width Width in pixels; zero or negative clips everything away.
     * @param height Height in pixels.
     *
     * @note Pixels, not logical units, and bottom-left origin - this is the
     * scissor rectangle as the graphics API means it. Converting from whatever
     * coordinate system the caller uses is the caller's job, because only the
     * caller knows its own camera and drawable size.
     *
     * @note Flushes first. The scissor cannot change part-way through a draw
     * call, so everything already queued must be drawn under the old one. That
     * is why this lives on the batch: the flush and the change are inseparable,
     * and anywhere else they could be desynchronised.
     */
    void set_clip(int x, int y, int width, int height) noexcept;

    /** @brief Stop restricting drawing. Flushes first, for the same reason. */
    void clear_clip() noexcept;

    /**
     * @brief Check initialization state.
     * @return true when initialization succeeded.
     */
    [[nodiscard]] bool valid() const noexcept;

    /**
     * @brief Read how many sprites one batch may hold before it must flush.
     * @return The capacity given to initialize().
     */
    [[nodiscard]] std::size_t sprite_capacity() const noexcept;

    /**
     * @brief Read how many sprites are queued and not yet drawn.
     * @return Zero outside begin()/end(), or straight after a flush.
     */
    [[nodiscard]] std::size_t queued_sprites() const noexcept;

    /**
     * @brief Read how many draws were refused since the last begin().
     *
     * A draw is refused when its region is no longer valid - its texture was
     * discarded, reloaded, or restored under it. Drawing it would sample
     * whatever now occupies that texture unit, so it is skipped.
     *
     * Skipping silently is correct and invisible, which is the problem: an
     * interface that draws nothing looks exactly like one that was never
     * asked to draw. This is the difference, and it is one assertion in a
     * test rather than an afternoon of bisecting.
     */
    [[nodiscard]] std::size_t skipped_draws() const noexcept;

    /** @brief Report whether begin() has been called without a matching end(). */
    [[nodiscard]] bool drawing() const noexcept;

private:
    // Implemented per backend; everything else is backend-independent.
    [[nodiscard]] bool allocate_gpu_objects() noexcept;
    [[nodiscard]] bool set_projection(
        const OrthographicCamera& camera
    ) noexcept;
    void destroy_gpu_objects() noexcept;

    // Backend-independent: abandons whatever a lost or released context left
    // queued. Called by each backend's invalidate() and release().
    void discard_queue() noexcept;

    // Per backend. A negative width means "no clip".
    void apply_clip(int x, int y, int width, int height) noexcept;
    void append_quad(
        const TextureRegion& region,
        const float* positions,
        sq::graphics::Color color
    ) noexcept;

    std::vector<float> vertices_;
    std::size_t maximum_sprites_{0};
    std::size_t sprite_count_{0};
    std::size_t skipped_draws_{0};
    unsigned int vertex_buffer_{0};
    unsigned int index_buffer_{0};
    unsigned int program_{0};
    int projection_uniform_{-1};
    int texture_uniform_{-1};
    // The texture the current batch is drawing from, by address rather than
    // by GL name. Identity is what a flush decision actually asks about, and
    // the name is private to Texture anyway - the backend binds through its
    // public bind().
    const Texture* active_texture_{nullptr};
    bool drawing_{false};
    bool invalidated_{false};
};

} // namespace sq::graphics2d
