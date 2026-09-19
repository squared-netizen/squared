#include <squared/gles/program.hpp>

#include <squared/gles/attribute_info.hpp>
#include <squared/gles/gles_error.hpp>
#include <squared/gles/gles_error_code.hpp>
#include <squared/gles/shader.hpp>
#include <squared/gles/shader_stage.hpp>
#include <squared/gles/uniform_info.hpp>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <GLES3/gl3.h>

namespace sq::gles {

namespace {

std::string read_program_log(GLuint name)
{
    GLint length = 0;
    glGetProgramiv(name, GL_INFO_LOG_LENGTH, &length);
    if (length <= 1) return {};

    std::vector<char> buffer(static_cast<std::size_t>(length));
    GLsizei written = 0;
    glGetProgramInfoLog(name, length, &written, buffer.data());
    return std::string{buffer.data(), static_cast<std::size_t>(written)};
}

/** @brief Longest active attribute or uniform name, for the query buffer. */
GLint longest_name(GLuint program, GLenum which) noexcept
{
    GLint length = 0;
    glGetProgramiv(program, which, &length);
    return length > 0 ? length : 1;
}

}  // namespace

Program::~Program()
{
    destroy();
}

Program::Program(Program&& other) noexcept
    : name_(std::exchange(other.name_, 0))
{
}

Program& Program::operator=(Program&& other) noexcept
{
    if (this != &other) {
        destroy();
        name_ = std::exchange(other.name_, 0);
    }
    return *this;
}

GlesError Program::link(const Shader& vertex, const Shader& fragment) noexcept
{
    if (!vertex.valid() || !fragment.valid()) {
        return GlesError{
            .code = GlesErrorCode::InvalidArgument,
            .message = "both shaders must be compiled before linking"
        };
    }

    destroy();

    const GLuint name = glCreateProgram();
    if (name == 0) return check_gl_errors();

    glAttachShader(name, vertex.name());
    glAttachShader(name, fragment.name());
    glLinkProgram(name);

    // Detach either way. A linked program keeps no reference to its shaders,
    // and leaving them attached keeps their storage alive for nothing.
    glDetachShader(name, vertex.name());
    glDetachShader(name, fragment.name());

    GLint linked = GL_FALSE;
    glGetProgramiv(name, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        GlesError error{
            .code = GlesErrorCode::LinkFailed,
            .message = read_program_log(name)
        };
        if (error.message.empty()) {
            error.message = "linking failed and the driver gave no log";
        }
        glDeleteProgram(name);
        return error;
    }

    name_ = name;
    return {};
}

GlesError Program::build(
    std::string_view vertex_source,
    std::string_view fragment_source
) noexcept
{
    Shader vertex;
    if (GlesError error = vertex.compile(ShaderStage::Vertex, vertex_source)) {
        return error;
    }

    Shader fragment;
    if (GlesError error =
            fragment.compile(ShaderStage::Fragment, fragment_source)) {
        return error;
    }

    return link(vertex, fragment);
}

void Program::use() const noexcept
{
    glUseProgram(name_);
}

std::int32_t Program::uniform_location(std::string_view name) const noexcept
{
    if (name_ == 0) return -1;
    // glGetUniformLocation needs a null-terminated string.
    const std::string terminated{name};
    return glGetUniformLocation(name_, terminated.c_str());
}

void Program::set_float(std::int32_t location, float value) const noexcept
{
    glUniform1f(location, value);
}

void Program::set_int(std::int32_t location, std::int32_t value) const noexcept
{
    glUniform1i(location, value);
}

void Program::set_vec2(std::int32_t location, float x, float y) const noexcept
{
    glUniform2f(location, x, y);
}

void Program::set_vec4(
    std::int32_t location,
    float x,
    float y,
    float z,
    float w
) const noexcept
{
    glUniform4f(location, x, y, z, w);
}

void Program::set_mat4(
    std::int32_t location,
    const float* values
) const noexcept
{
    // GL_FALSE: the values are already column-major, which is how
    // sq::math::Matrix4 stores them. GLES 3.0 rejects GL_TRUE here anyway.
    glUniformMatrix4fv(location, 1, GL_FALSE, values);
}

GlesError Program::attributes(std::vector<AttributeInfo>& out) const
{
    out.clear();
    if (name_ == 0) {
        return GlesError{
            .code = GlesErrorCode::InvalidArgument,
            .message = "program is not linked"
        };
    }

    GLint count = 0;
    glGetProgramiv(name_, GL_ACTIVE_ATTRIBUTES, &count);
    if (count <= 0) return check_gl_errors();

    const GLint capacity =
        longest_name(name_, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH);
    std::vector<char> buffer(static_cast<std::size_t>(capacity));

    out.reserve(static_cast<std::size_t>(count));
    for (GLint index = 0; index < count; ++index) {
        GLsizei written = 0;
        GLint size = 0;
        GLenum type = 0;
        glGetActiveAttrib(
            name_,
            static_cast<GLuint>(index),
            capacity,
            &written,
            &size,
            &type,
            buffer.data()
        );

        std::string attribute_name{
            buffer.data(), static_cast<std::size_t>(written)
        };
        const GLint location =
            glGetAttribLocation(name_, attribute_name.c_str());

        out.push_back(AttributeInfo{
            .name = std::move(attribute_name),
            .location = location,
            .type = static_cast<std::uint32_t>(type),
            .size = size
        });
    }
    return check_gl_errors();
}

GlesError Program::uniforms(std::vector<UniformInfo>& out) const
{
    out.clear();
    if (name_ == 0) {
        return GlesError{
            .code = GlesErrorCode::InvalidArgument,
            .message = "program is not linked"
        };
    }

    GLint count = 0;
    glGetProgramiv(name_, GL_ACTIVE_UNIFORMS, &count);
    if (count <= 0) return check_gl_errors();

    const GLint capacity = longest_name(name_, GL_ACTIVE_UNIFORM_MAX_LENGTH);
    std::vector<char> buffer(static_cast<std::size_t>(capacity));

    out.reserve(static_cast<std::size_t>(count));
    for (GLint index = 0; index < count; ++index) {
        GLsizei written = 0;
        GLint size = 0;
        GLenum type = 0;
        glGetActiveUniform(
            name_,
            static_cast<GLuint>(index),
            capacity,
            &written,
            &size,
            &type,
            buffer.data()
        );

        std::string uniform_name{
            buffer.data(), static_cast<std::size_t>(written)
        };
        const GLint location =
            glGetUniformLocation(name_, uniform_name.c_str());

        out.push_back(UniformInfo{
            .name = std::move(uniform_name),
            .location = location,
            .type = static_cast<std::uint32_t>(type),
            .size = size
        });
    }
    return check_gl_errors();
}

void Program::destroy() noexcept
{
    if (name_ != 0) {
        glDeleteProgram(name_);
        name_ = 0;
    }
}

void Program::invalidate() noexcept
{
    name_ = 0;
}

}  // namespace sq::gles
