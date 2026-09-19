#pragma once

#include <squared/gles/gles_error.hpp>
#include <squared/gles/vertex_attribute.hpp>

#include <cstdint>
#include <span>

namespace sq::gles {

class Buffer;

/**
 * @brief One vertex array object, owned.
 *
 * A VAO records which buffers are bound and how their bytes are interpreted,
 * so drawing is one bind instead of a dozen calls. 4 bytes; the state lives in
 * the driver.
 *
 * GLES 3.0 guarantees VAOs. On GLES 2.0 they are an extension, which is part
 * of why squared's floor is 3.0.
 */
class VertexArray final {
public:
    VertexArray() noexcept = default;
    ~VertexArray();

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;
    VertexArray(VertexArray&& other) noexcept;
    VertexArray& operator=(VertexArray&& other) noexcept;

    /** @brief Create the GL object. */
    [[nodiscard]] GlesError create() noexcept;

    /**
     * @brief Record a vertex buffer and its attribute layout.
     * @param vertices Buffer holding the vertex data.
     * @param attributes One entry per attribute to enable.
     * @return A no-error GlesError on success.
     * @note This binds the VAO, binds the buffer, and enables each attribute.
     * The bindings are recorded in the VAO, so the buffer does not have to
     * stay bound afterwards &mdash; but it does have to stay alive.
     */
    [[nodiscard]] GlesError set_vertices(
        const Buffer& vertices,
        std::span<const VertexAttribute> attributes
    ) noexcept;

    /**
     * @brief Record an index buffer.
     * @param indices Buffer created with BufferTarget::Index.
     * @return A no-error GlesError on success.
     */
    [[nodiscard]] GlesError set_indices(const Buffer& indices) noexcept;

    /** @brief Bind this vertex array. */
    void bind() const noexcept;

    /** @brief Unbind whatever vertex array is bound. */
    static void unbind() noexcept;

    /** @brief Release the GL object. Safe to call more than once. */
    void destroy() noexcept;

    /** @brief Forget the GL name without deleting it, after context loss. */
    void invalidate() noexcept;

    /** @brief Report whether this holds a GL object. */
    [[nodiscard]] bool valid() const noexcept { return name_ != 0; }

    /** @brief Return the raw GL name, or zero. */
    [[nodiscard]] std::uint32_t name() const noexcept { return name_; }

private:
    std::uint32_t name_{0};
};

}  // namespace sq::gles
