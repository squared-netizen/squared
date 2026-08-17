#pragma once

#include <squared/graphics/color.hpp>
#include <squared/graphics2d/orthographic_camera.hpp>
#include <squared/graphics2d/sprite.hpp>
#include <squared/graphics2d/texture_region.hpp>

#include <cstddef>
#include <vector>

namespace squared::graphics2d {

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
    [[nodiscard]] bool restore(bool context_preserved = false) noexcept;

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
        squared::graphics::Color color =
            squared::graphics::Color::white()
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
     * @brief Check initialization state.
     * @return true when initialization succeeded.
     */
    [[nodiscard]] bool valid() const noexcept;

private:
    [[nodiscard]] bool allocate_gpu_objects() noexcept;
    void append_quad(
        const TextureRegion& region,
        const float* positions,
        squared::graphics::Color color
    ) noexcept;

    std::vector<float> vertices_;
    std::size_t maximum_sprites_{0};
    std::size_t sprite_count_{0};
    unsigned int vertex_buffer_{0};
    unsigned int index_buffer_{0};
    unsigned int program_{0};
    int projection_uniform_{-1};
    int texture_uniform_{-1};
    unsigned int active_texture_{0};
    bool drawing_{false};
    bool invalidated_{false};
};

}  // namespace squared::graphics2d
