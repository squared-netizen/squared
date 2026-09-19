#pragma once

#include <cstdint>
#include <string>

namespace sq::gles {

/**
 * @brief One vertex attribute a linked program actually uses.
 *
 * Reported by Program::attributes(). Worth knowing when reading a program:
 * the linker removes attributes the shader never reads, so this list is what
 * the GPU will really look for, not what the source appeared to declare.
 */
struct AttributeInfo final {
    /** @brief Attribute name as written in the shader. */
    std::string name;

    /** @brief Location to pass to VertexArray::set_attribute(). */
    std::int32_t location{-1};

    /** @brief GL type enum; see glsl_type_name() for its GLSL spelling. */
    std::uint32_t type{0};

    /** @brief Element count; greater than one only for an array attribute. */
    std::int32_t size{0};
};

}  // namespace sq::gles
