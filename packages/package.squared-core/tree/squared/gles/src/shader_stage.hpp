#pragma once

#include <cstdint>

namespace sq::gles
{

/** @brief Which programmable stage a shader belongs to. */
enum class ShaderStage : std::uint8_t {
    /** @brief Runs once per vertex; GL_VERTEX_SHADER. */
    Vertex,

    /** @brief Runs once per fragment; GL_FRAGMENT_SHADER. */
    Fragment
};

} // namespace sq::gles
