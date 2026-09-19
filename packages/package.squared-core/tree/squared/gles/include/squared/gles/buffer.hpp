#pragma once

#include <squared/gles/buffer_target.hpp>
#include <squared/gles/buffer_usage.hpp>
#include <squared/gles/gles_error.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

namespace sq::gles {

/**
 * @brief One GL buffer object, owned.
 *
 * 12 bytes: a name, a target and a size. Nothing is mirrored on the CPU, so a
 * buffer costs what the GPU allocation costs and nothing more.
 */
class Buffer final {
public:
    Buffer() noexcept = default;
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    /**
     * @brief Allocate storage and optionally fill it.
     * @param target What the buffer will be bound as.
     * @param usage Hint for how often the contents will change.
     * @param bytes Initial contents; may be empty to allocate uninitialised
     * storage of size_bytes.
     * @param size_bytes Storage to allocate; ignored when bytes is non-empty.
     * @return A no-error GlesError on success.
     */
    [[nodiscard]] GlesError create(
        BufferTarget target,
        BufferUsage usage,
        std::span<const std::byte> bytes,
        std::size_t size_bytes = 0
    ) noexcept;

    /**
     * @brief Replace part of the contents.
     * @param offset_bytes Byte offset to write at.
     * @param bytes Data to write; must fit within the allocated size.
     * @return A no-error GlesError on success.
     */
    [[nodiscard]] GlesError update(
        std::size_t offset_bytes,
        std::span<const std::byte> bytes
    ) noexcept;

    /** @brief Bind this buffer to its target. */
    void bind() const noexcept;

    /** @brief Unbind whatever is bound to this buffer's target. */
    void unbind() const noexcept;

    /** @brief Release the GL object. Safe to call more than once. */
    void destroy() noexcept;

    /**
     * @brief Forget the GL name without deleting it.
     *
     * Call this after the rendering context has been lost. The name belonged
     * to a context that no longer exists; deleting it is at best a no-op and
     * at worst frees an unrelated object in the new context. Every object in
     * this module has this method for that reason, and getting it wrong
     * produces corruption that looks like a driver bug.
     */
    void invalidate() noexcept;

    /** @brief Report whether this buffer holds a GL object. */
    [[nodiscard]] bool valid() const noexcept { return name_ != 0; }

    /** @brief Return the raw GL name, or zero. For interop and teaching. */
    [[nodiscard]] std::uint32_t name() const noexcept { return name_; }

    /** @brief Return the target this buffer was created for. */
    [[nodiscard]] BufferTarget target() const noexcept { return target_; }

    /** @brief Return the allocated size in bytes. */
    [[nodiscard]] std::size_t size_bytes() const noexcept { return size_; }

private:
    std::uint32_t name_{0};
    BufferTarget target_{BufferTarget::Vertex};
    std::size_t size_{0};
};

}  // namespace sq::gles
