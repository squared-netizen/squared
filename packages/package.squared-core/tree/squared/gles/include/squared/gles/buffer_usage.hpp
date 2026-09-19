#pragma once

namespace sq::gles {

/**
 * @brief How often a buffer's contents are expected to change.
 *
 * This is a hint. The driver is free to ignore it, and getting it wrong costs
 * performance rather than correctness. The distinction that matters in
 * practice: Stream tells the driver you will overwrite the whole buffer every
 * frame, which lets it hand you fresh memory instead of stalling until the GPU
 * has finished reading the old contents.
 */
enum class BufferUsage {
    /** @brief Written once, drawn many times; GL_STATIC_DRAW. */
    Static,

    /** @brief Rewritten occasionally; GL_DYNAMIC_DRAW. */
    Dynamic,

    /** @brief Rewritten every frame; GL_STREAM_DRAW. */
    Stream
};

}  // namespace sq::gles
