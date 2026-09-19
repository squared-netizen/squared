#include <squared/gles/vertex_array.hpp>

#include <squared/gles/buffer.hpp>
#include <squared/gles/buffer_target.hpp>
#include <squared/gles/gles_error.hpp>
#include <squared/gles/gles_error_code.hpp>
#include <squared/gles/vertex_attribute.hpp>

#include <cstddef>
#include <span>
#include <utility>

#include <GLES3/gl3.h>

namespace sq::gles {

VertexArray::~VertexArray()
{
    destroy();
}

VertexArray::VertexArray(VertexArray&& other) noexcept
    : name_(std::exchange(other.name_, 0))
{
}

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept
{
    if (this != &other) {
        destroy();
        name_ = std::exchange(other.name_, 0);
    }
    return *this;
}

GlesError VertexArray::create() noexcept
{
    destroy();

    GLuint name = 0;
    glGenVertexArrays(1, &name);
    if (name == 0) return check_gl_errors();

    name_ = name;
    return {};
}

GlesError VertexArray::set_vertices(
    const Buffer& vertices,
    std::span<const VertexAttribute> attributes
) noexcept
{
    if (name_ == 0 || !vertices.valid()) {
        return GlesError{
            .code = GlesErrorCode::InvalidArgument,
            .message = "vertex array or buffer has no GL object"
        };
    }
    if (vertices.target() != BufferTarget::Vertex) {
        return GlesError{
            .code = GlesErrorCode::InvalidArgument,
            .message = "buffer was not created as BufferTarget::Vertex"
        };
    }

    glBindVertexArray(name_);
    vertices.bind();

    for (const VertexAttribute& attribute : attributes) {
        if (attribute.location < 0) continue;

        const auto location = static_cast<GLuint>(attribute.location);
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(
            location,
            attribute.components,
            GL_FLOAT,
            GL_FALSE,
            static_cast<GLsizei>(attribute.stride),
            reinterpret_cast<const void*>(attribute.offset)
        );
        if (attribute.instance_divisor != 0) {
            glVertexAttribDivisor(location, attribute.instance_divisor);
        }
    }

    glBindVertexArray(0);
    return check_gl_errors();
}

GlesError VertexArray::set_indices(const Buffer& indices) noexcept
{
    if (name_ == 0 || !indices.valid()) {
        return GlesError{
            .code = GlesErrorCode::InvalidArgument,
            .message = "vertex array or buffer has no GL object"
        };
    }
    if (indices.target() != BufferTarget::Index) {
        return GlesError{
            .code = GlesErrorCode::InvalidArgument,
            .message = "buffer was not created as BufferTarget::Index"
        };
    }

    // The element buffer binding is part of vertex array state, which is why
    // it is recorded here and not at draw time.
    glBindVertexArray(name_);
    indices.bind();
    glBindVertexArray(0);
    return check_gl_errors();
}

void VertexArray::bind() const noexcept
{
    glBindVertexArray(name_);
}

void VertexArray::unbind() noexcept
{
    glBindVertexArray(0);
}

void VertexArray::destroy() noexcept
{
    if (name_ != 0) {
        glDeleteVertexArrays(1, &name_);
        name_ = 0;
    }
}

void VertexArray::invalidate() noexcept
{
    name_ = 0;
}

}  // namespace sq::gles
