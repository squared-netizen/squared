#include <squared/gles/buffer.hpp>

#include <squared/gles/buffer_target.hpp>
#include <squared/gles/buffer_usage.hpp>
#include <squared/gles/gles_error.hpp>
#include <squared/gles/gles_error_code.hpp>

#include <cstddef>
#include <utility>

#include <GLES3/gl3.h>

namespace sq::gles {

namespace {

GLenum to_gl_target(BufferTarget target) noexcept
{
    switch (target) {
    case BufferTarget::Vertex: return GL_ARRAY_BUFFER;
    case BufferTarget::Index: return GL_ELEMENT_ARRAY_BUFFER;
    case BufferTarget::Uniform: return GL_UNIFORM_BUFFER;
    }
    return GL_ARRAY_BUFFER;
}

GLenum to_gl_usage(BufferUsage usage) noexcept
{
    switch (usage) {
    case BufferUsage::Static: return GL_STATIC_DRAW;
    case BufferUsage::Dynamic: return GL_DYNAMIC_DRAW;
    case BufferUsage::Stream: return GL_STREAM_DRAW;
    }
    return GL_STATIC_DRAW;
}

}  // namespace

Buffer::~Buffer()
{
    destroy();
}

Buffer::Buffer(Buffer&& other) noexcept
    : name_(std::exchange(other.name_, 0))
    , target_(other.target_)
    , size_(std::exchange(other.size_, 0))
{
}

Buffer& Buffer::operator=(Buffer&& other) noexcept
{
    if (this != &other) {
        destroy();
        name_ = std::exchange(other.name_, 0);
        target_ = other.target_;
        size_ = std::exchange(other.size_, 0);
    }
    return *this;
}

GlesError Buffer::create(
    BufferTarget target,
    BufferUsage usage,
    std::span<const std::byte> bytes,
    std::size_t size_bytes
) noexcept
{
    const std::size_t requested = bytes.empty() ? size_bytes : bytes.size();
    if (requested == 0) {
        return GlesError{
            .code = GlesErrorCode::InvalidArgument,
            .message = "buffer size is zero"
        };
    }

    destroy();

    GLuint name = 0;
    glGenBuffers(1, &name);
    if (name == 0) return check_gl_errors();

    const GLenum gl_target = to_gl_target(target);
    glBindBuffer(gl_target, name);
    glBufferData(
        gl_target,
        static_cast<GLsizeiptr>(requested),
        bytes.empty() ? nullptr : bytes.data(),
        to_gl_usage(usage)
    );

    if (GlesError error = check_gl_errors()) {
        glDeleteBuffers(1, &name);
        return error;
    }

    name_ = name;
    target_ = target;
    size_ = requested;
    return {};
}

GlesError Buffer::update(
    std::size_t offset_bytes,
    std::span<const std::byte> bytes
) noexcept
{
    if (name_ == 0) {
        return GlesError{
            .code = GlesErrorCode::InvalidArgument,
            .message = "buffer has no storage"
        };
    }
    if (bytes.empty()) return {};
    if (offset_bytes + bytes.size() > size_) {
        return GlesError{
            .code = GlesErrorCode::InvalidArgument,
            .message = "update runs past the end of the buffer"
        };
    }

    const GLenum gl_target = to_gl_target(target_);
    glBindBuffer(gl_target, name_);
    glBufferSubData(
        gl_target,
        static_cast<GLintptr>(offset_bytes),
        static_cast<GLsizeiptr>(bytes.size()),
        bytes.data()
    );
    return check_gl_errors();
}

void Buffer::bind() const noexcept
{
    glBindBuffer(to_gl_target(target_), name_);
}

void Buffer::unbind() const noexcept
{
    glBindBuffer(to_gl_target(target_), 0);
}

void Buffer::destroy() noexcept
{
    if (name_ != 0) {
        glDeleteBuffers(1, &name_);
        name_ = 0;
    }
    size_ = 0;
}

void Buffer::invalidate() noexcept
{
    name_ = 0;
    size_ = 0;
}

}  // namespace sq::gles
