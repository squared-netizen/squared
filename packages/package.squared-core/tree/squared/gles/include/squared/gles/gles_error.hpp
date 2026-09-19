#pragma once

#include <squared/gles/gles_error_code.hpp>

#include <string>

namespace sq::gles {

/** @brief Structured GL failure information. */
struct GlesError {
    /** @brief Failure category; None means no error. */
    GlesErrorCode code{GlesErrorCode::None};

    /**
     * @brief Diagnostic message.
     *
     * For CompileFailed and LinkFailed this is the driver's own log, verbatim
     * and unfiltered. Drivers say useful things there and squared does not
     * paraphrase them.
     */
    std::string message;

    /** @brief Return whether this structure represents a failure. */
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return code != GlesErrorCode::None;
    }
};

/**
 * @brief Drain the GL error queue and report the first error found.
 * @return A no-error GlesError when the queue was empty.
 * @note GL accumulates errors and hands them back one at a time; leaving them
 * queued makes the next unrelated check look like it failed. This drains the
 * whole queue so a check means what it says.
 */
[[nodiscard]] GlesError check_gl_errors() noexcept;

/**
 * @brief Return a GL enum's identifier as text.
 * @param value Any GL enum, such as a type from Program::attributes().
 * @return A static string such as "GL_FLOAT_VEC4", or "GL_?" when unknown.
 */
[[nodiscard]] const char* gl_enum_name(unsigned int value) noexcept;

/**
 * @brief Return the GLSL spelling of a GL type enum.
 * @param type A type as reported by Program::attributes() or uniforms().
 * @return A static string such as "vec4", "mat3" or "sampler2D", or "?" when
 * unknown.
 * @note This is the name you would write in the shader source, which is the
 * useful form when reading what the linker actually produced.
 */
[[nodiscard]] const char* glsl_type_name(unsigned int type) noexcept;

}  // namespace sq::gles
