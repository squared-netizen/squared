#include <squared/gles/shader.hpp>

#include <squared/gles/gles_error.hpp>
#include <squared/gles/gles_error_code.hpp>
#include <squared/gles/shader_stage.hpp>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <GLES3/gl3.h>

namespace sq::gles {

namespace {

GLenum to_gl_stage(ShaderStage stage) noexcept
{
    switch (stage) {
    case ShaderStage::Vertex: return GL_VERTEX_SHADER;
    case ShaderStage::Fragment: return GL_FRAGMENT_SHADER;
    }
    return GL_VERTEX_SHADER;
}

/** @brief Read a shader's info log, whatever length the driver chose. */
std::string read_shader_log(GLuint name)
{
    GLint length = 0;
    glGetShaderiv(name, GL_INFO_LOG_LENGTH, &length);
    if (length <= 1) return {};

    std::vector<char> buffer(static_cast<std::size_t>(length));
    GLsizei written = 0;
    glGetShaderInfoLog(name, length, &written, buffer.data());
    return std::string{buffer.data(), static_cast<std::size_t>(written)};
}

}  // namespace

Shader::~Shader()
{
    destroy();
}

Shader::Shader(Shader&& other) noexcept
    : name_(std::exchange(other.name_, 0))
    , stage_(other.stage_)
{
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other) {
        destroy();
        name_ = std::exchange(other.name_, 0);
        stage_ = other.stage_;
    }
    return *this;
}

GlesError Shader::compile(ShaderStage stage, std::string_view source) noexcept
{
    if (source.empty()) {
        return GlesError{
            .code = GlesErrorCode::InvalidArgument,
            .message = "shader source is empty"
        };
    }

    destroy();

    const GLuint name = glCreateShader(to_gl_stage(stage));
    if (name == 0) return check_gl_errors();

    const auto* text = source.data();
    const auto length = static_cast<GLint>(source.size());
    glShaderSource(name, 1, &text, &length);
    glCompileShader(name);

    GLint compiled = GL_FALSE;
    glGetShaderiv(name, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        // The driver's log is the single most useful thing here, so it is
        // passed through verbatim rather than summarised.
        GlesError error{
            .code = GlesErrorCode::CompileFailed,
            .message = read_shader_log(name)
        };
        if (error.message.empty()) {
            error.message = "compilation failed and the driver gave no log";
        }
        glDeleteShader(name);
        return error;
    }

    name_ = name;
    stage_ = stage;
    return {};
}

void Shader::destroy() noexcept
{
    if (name_ != 0) {
        glDeleteShader(name_);
        name_ = 0;
    }
}

void Shader::invalidate() noexcept
{
    name_ = 0;
}

}  // namespace sq::gles
