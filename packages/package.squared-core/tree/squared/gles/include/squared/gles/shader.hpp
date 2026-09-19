#pragma once

#include <squared/gles/gles_error.hpp>
#include <squared/gles/shader_stage.hpp>

#include <cstdint>
#include <string_view>

namespace sq::gles {

/**
 * @brief One compiled shader stage, owned.
 *
 * A shader exists to be attached to a Program and then released; Program keeps
 * nothing of it after linking. 8 bytes, no retained source and no retained log.
 *
 * The compile log is returned in the GlesError on failure rather than stored,
 * because a successful shader's log is almost always empty and a failed one is
 * about to be thrown away. Drivers put genuinely useful text there, so it is
 * passed through verbatim.
 */
class Shader final {
public:
    Shader() noexcept = default;
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    /**
     * @brief Compile GLSL source for one stage.
     * @param stage Which programmable stage this is.
     * @param source GLSL source; the `#version` directive must be its first
     * line, as GLSL requires.
     * @return A no-error GlesError on success, or CompileFailed carrying the
     * driver's log.
     */
    [[nodiscard]] GlesError compile(
        ShaderStage stage,
        std::string_view source
    ) noexcept;

    /** @brief Release the GL object. Safe to call more than once. */
    void destroy() noexcept;

    /** @brief Forget the GL name without deleting it, after context loss. */
    void invalidate() noexcept;

    /** @brief Report whether this shader holds a compiled GL object. */
    [[nodiscard]] bool valid() const noexcept { return name_ != 0; }

    /** @brief Return the raw GL name, or zero. */
    [[nodiscard]] std::uint32_t name() const noexcept { return name_; }

    /** @brief Return the stage this shader was compiled for. */
    [[nodiscard]] ShaderStage stage() const noexcept { return stage_; }

private:
    std::uint32_t name_{0};
    ShaderStage stage_{ShaderStage::Vertex};
};

}  // namespace sq::gles
