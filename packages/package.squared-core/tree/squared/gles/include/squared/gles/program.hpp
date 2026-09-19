#pragma once

#include <squared/gles/attribute_info.hpp>
#include <squared/gles/gles_error.hpp>
#include <squared/gles/uniform_info.hpp>

#include <cstdint>
#include <string_view>
#include <vector>

namespace sq::gles {

class Shader;

/**
 * @brief One linked GL program, owned.
 *
 * 8 bytes. Shaders are attached, linked and detached; nothing of them is kept.
 *
 * Attribute and uniform lists are queried on demand rather than cached,
 * filling a caller-owned vector. A program that is drawn with a thousand times
 * a second is introspected once, by a tool or a person, so the list costs
 * nothing until somebody asks.
 */
class Program final {
public:
    Program() noexcept = default;
    ~Program();

    Program(const Program&) = delete;
    Program& operator=(const Program&) = delete;
    Program(Program&& other) noexcept;
    Program& operator=(Program&& other) noexcept;

    /**
     * @brief Link a vertex and a fragment shader into a program.
     * @param vertex Compiled vertex shader.
     * @param fragment Compiled fragment shader.
     * @return A no-error GlesError on success, or LinkFailed carrying the
     * driver's log verbatim.
     */
    [[nodiscard]] GlesError link(
        const Shader& vertex,
        const Shader& fragment
    ) noexcept;

    /**
     * @brief Compile and link in one step.
     * @param vertex_source GLSL vertex source.
     * @param fragment_source GLSL fragment source.
     * @return A no-error GlesError on success, or the first failure, carrying
     * the log of whichever stage failed.
     */
    [[nodiscard]] GlesError build(
        std::string_view vertex_source,
        std::string_view fragment_source
    ) noexcept;

    /** @brief Make this program current. */
    void use() const noexcept;

    /**
     * @brief Return the location of a uniform.
     * @param name Uniform name as written in the shader.
     * @return The location, or -1 when the program has no such active uniform.
     * @note -1 is not an error. A uniform the shader declares but never reads
     * is removed by the linker, and that is the usual reason setting one
     * appears to do nothing.
     */
    [[nodiscard]] std::int32_t uniform_location(
        std::string_view name
    ) const noexcept;

    /** @brief Set a float uniform. The program must be current. */
    void set_float(std::int32_t location, float value) const noexcept;

    /** @brief Set an int or sampler uniform. The program must be current. */
    void set_int(std::int32_t location, std::int32_t value) const noexcept;

    /** @brief Set a vec2 uniform. The program must be current. */
    void set_vec2(std::int32_t location, float x, float y) const noexcept;

    /** @brief Set a vec4 uniform. The program must be current. */
    void set_vec4(
        std::int32_t location,
        float x,
        float y,
        float z,
        float w
    ) const noexcept;

    /**
     * @brief Set a mat4 uniform from 16 floats in column-major order.
     * @param location Uniform location.
     * @param values Sixteen floats; GL expects column-major, which is the
     * order sq::math::Matrix4 already stores.
     */
    void set_mat4(
        std::int32_t location,
        const float* values
    ) const noexcept;

    /**
     * @brief List the vertex attributes this program actually uses.
     * @param out Cleared, then filled with one entry per active attribute.
     * @return A no-error GlesError on success.
     * @note The caller owns the vector so it can be reused. Nothing is cached.
     */
    [[nodiscard]] GlesError attributes(
        std::vector<AttributeInfo>& out
    ) const;

    /**
     * @brief List the uniforms this program actually uses.
     * @param out Cleared, then filled with one entry per active uniform.
     * @return A no-error GlesError on success.
     */
    [[nodiscard]] GlesError uniforms(std::vector<UniformInfo>& out) const;

    /** @brief Release the GL object. Safe to call more than once. */
    void destroy() noexcept;

    /** @brief Forget the GL name without deleting it, after context loss. */
    void invalidate() noexcept;

    /** @brief Report whether this program holds a linked GL object. */
    [[nodiscard]] bool valid() const noexcept { return name_ != 0; }

    /** @brief Return the raw GL name, or zero. */
    [[nodiscard]] std::uint32_t name() const noexcept { return name_; }

private:
    std::uint32_t name_{0};
};

}  // namespace sq::gles
