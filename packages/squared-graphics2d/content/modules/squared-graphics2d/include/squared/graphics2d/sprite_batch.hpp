#pragma once

#include <squared/graphics/color.hpp>
#include <squared/graphics2d/orthographic_camera.hpp>
#include <squared/graphics2d/sprite.hpp>
#include <squared/graphics2d/texture_region.hpp>

#include <cstddef>
#include <vector>

namespace squared::graphics2d {

/**
 * @brief Efficiently draw ordered textured quads with OpenGL ES 2.
 */
class SpriteBatch final {
public:
    SpriteBatch() noexcept = default;
    SpriteBatch(const SpriteBatch&) = delete;
    SpriteBatch& operator=(const SpriteBatch&) = delete;
    SpriteBatch(SpriteBatch&&) = delete;
    SpriteBatch& operator=(SpriteBatch&&) = delete;
    ~SpriteBatch();

    /**
     * @brief Allocate GPU buffers and compile the built-in sprite shader.
     *
     * @param maximum_sprites Maximum sprites buffered before an automatic
     * flush.
     */
    [[nodiscard]] bool initialize(
        std::size_t maximum_sprites = 2048
    ) noexcept;

    /** @brief Release all owned OpenGL ES objects. */
    void destroy() noexcept;

    /** @brief Begin an ordered batch using the camera projection. */
    [[nodiscard]] bool begin(
        const OrthographicCamera& camera
    ) noexcept;

    /** @brief Draw one region without rotation. */
    void draw(
        const TextureRegion& region,
        float x,
        float y,
        float width,
        float height,
        squared::graphics::Color color =
            squared::graphics::Color::white()
    ) noexcept;

    /** @brief Draw one transformed Sprite. */
    void draw(const Sprite& sprite) noexcept;

    /** @brief Flush queued sprites and finish the batch. */
    void end() noexcept;

    /** @brief Flush queued sprites without ending the batch. */
    void flush() noexcept;

    /** @brief Return whether initialization succeeded. */
    [[nodiscard]] bool valid() const noexcept;

private:
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
};

}  // namespace squared::graphics2d
