#pragma once

namespace sq::gles {

/** @brief What a buffer's contents are bound as. */
enum class BufferTarget {
    /** @brief Per-vertex attribute data; GL_ARRAY_BUFFER. */
    Vertex,

    /** @brief Vertex indices; GL_ELEMENT_ARRAY_BUFFER. */
    Index,

    /** @brief A uniform block; GL_UNIFORM_BUFFER. GLES 3.0 and later. */
    Uniform
};

}  // namespace sq::gles
