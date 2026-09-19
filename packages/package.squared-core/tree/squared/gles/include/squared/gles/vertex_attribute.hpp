#pragma once

#include <cstddef>
#include <cstdint>

namespace sq::gles {

/**
 * @brief How one attribute is laid out inside a vertex buffer.
 *
 * The four numbers that confuse everyone the first time, named:
 * `components` is how many floats this attribute has, `stride` is the size of
 * one whole vertex, and `offset` is where this attribute starts inside that
 * vertex. A position and a colour interleaved in one buffer share a stride and
 * differ in offset.
 */
struct VertexAttribute final {
    /** @brief Attribute location, from Program::attributes(). */
    std::int32_t location{-1};

    /** @brief Floats in this attribute: 1, 2, 3 or 4. */
    std::int32_t components{0};

    /** @brief Bytes from one vertex to the next, not from one attribute. */
    std::size_t stride{0};

    /** @brief Bytes from the start of a vertex to this attribute. */
    std::size_t offset{0};

    /**
     * @brief Divisor for instanced drawing; zero means per-vertex.
     * @note One means the attribute advances once per instance rather than
     * once per vertex. GLES 3.0 and later.
     */
    std::uint32_t instance_divisor{0};
};

}  // namespace sq::gles
