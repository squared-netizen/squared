#pragma once

namespace sq::gles {

/** @brief Which programmable stage a shader belongs to. */
enum class ShaderStage {
    /** @brief Runs once per vertex; GL_VERTEX_SHADER. */
    Vertex,

    /** @brief Runs once per fragment; GL_FRAGMENT_SHADER. */
    Fragment
};

}  // namespace sq::gles
