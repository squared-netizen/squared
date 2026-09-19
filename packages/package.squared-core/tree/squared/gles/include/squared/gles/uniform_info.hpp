#pragma once

#include <cstdint>
#include <string>

namespace sq::gles {

/**
 * @brief One uniform a linked program actually uses.
 *
 * Reported by Program::uniforms(). A uniform the shader declares but never
 * reads is optimised away and will not appear here, and its location will be
 * -1 &mdash; which is the usual reason a uniform "does not work".
 */
struct UniformInfo final {
    /** @brief Uniform name as written in the shader. */
    std::string name;

    /** @brief Location to pass to the Program::set_* methods. */
    std::int32_t location{-1};

    /** @brief GL type enum; see glsl_type_name() for its GLSL spelling. */
    std::uint32_t type{0};

    /** @brief Element count; greater than one only for an array uniform. */
    std::int32_t size{0};
};

}  // namespace sq::gles
