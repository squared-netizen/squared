// The GPU half of SpriteBatch, for GLES 3.0.
//
// Buffers, the shader, and the draw call. Everything else - vertex building,
// the flush decisions, the begin/draw/end state - is in
// src/common/sprite_batch_common.cpp and is shared by every backend.

#include <squared/graphics2d/sprite_batch.hpp>

#include <squared/graphics2d/orthographic_camera.hpp>
#include <squared/graphics2d/texture.hpp>
#include <squared/math/matrix4.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <GLES3/gl3.h>

namespace sq::graphics2d {

namespace {

constexpr std::size_t k_floats_per_vertex = 8;
constexpr std::size_t k_vertices_per_sprite = 4;
constexpr std::size_t k_indices_per_sprite = 6;

constexpr const char* k_vertex_shader = R"(#version 300 es
layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_texture_coordinate;
layout(location = 2) in vec4 a_color;

uniform mat4 u_projection;

out vec2 v_texture_coordinate;
out vec4 v_color;

void main() {
    v_texture_coordinate = a_texture_coordinate;
    v_color = a_color;
    gl_Position = u_projection * vec4(a_position, 0.0, 1.0);
}
)";

// mediump is the default on mobile GPUs and is enough for colour. The sampler
// and the interpolated coordinates keep it; a highp fragment shader costs
// real throughput on a tiler for no visible difference in 2D.
constexpr const char* k_fragment_shader = R"(#version 300 es
precision mediump float;

in vec2 v_texture_coordinate;
in vec4 v_color;

uniform sampler2D u_texture;

out vec4 fragment_color;

void main() {
    fragment_color = texture(u_texture, v_texture_coordinate) * v_color;
}
)";

GLuint compile(GLenum stage, const char* source) noexcept
{
    const GLuint shader = glCreateShader(stage);
    if (shader == 0) return 0;

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

}  // namespace

bool SpriteBatch::allocate_gpu_objects() noexcept
{
    // Never leave half-built objects behind on a retry after context loss.
    destroy_gpu_objects();

    const GLuint vertex = compile(GL_VERTEX_SHADER, k_vertex_shader);
    if (vertex == 0) return false;
    const GLuint fragment = compile(GL_FRAGMENT_SHADER, k_fragment_shader);
    if (fragment == 0) {
        glDeleteShader(vertex);
        return false;
    }

    const GLuint program = glCreateProgram();
    if (program == 0) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return false;
    }
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDetachShader(program, vertex);
    glDetachShader(program, fragment);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        glDeleteProgram(program);
        return false;
    }
    program_ = program;
    projection_uniform_ = glGetUniformLocation(program_, "u_projection");
    texture_uniform_ = glGetUniformLocation(program_, "u_texture");

    GLuint buffers[2] = {0, 0};
    glGenBuffers(2, buffers);
    if (buffers[0] == 0 || buffers[1] == 0) {
        destroy_gpu_objects();
        return false;
    }
    vertex_buffer_ = buffers[0];
    index_buffer_ = buffers[1];

    // Vertex storage is allocated once at full size and refilled each flush.
    // GL_STREAM_DRAW says so: the driver hands back fresh memory rather than
    // stalling until the GPU has finished reading the previous contents.
    const GLsizeiptr vertex_bytes = static_cast<GLsizeiptr>(
        maximum_sprites_ * k_vertices_per_sprite * k_floats_per_vertex
        * sizeof(float));
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glBufferData(GL_ARRAY_BUFFER, vertex_bytes, nullptr, GL_STREAM_DRAW);

    // Indices never change: quad N is always the same six values. Built once
    // and uploaded as static, so a flush uploads vertices only.
    std::vector<std::uint16_t> indices;
    indices.reserve(maximum_sprites_ * k_indices_per_sprite);
    for (std::size_t sprite = 0; sprite < maximum_sprites_; ++sprite) {
        const auto base = static_cast<std::uint16_t>(
            sprite * k_vertices_per_sprite);
        indices.push_back(base);
        indices.push_back(static_cast<std::uint16_t>(base + 1));
        indices.push_back(static_cast<std::uint16_t>(base + 2));
        indices.push_back(base);
        indices.push_back(static_cast<std::uint16_t>(base + 2));
        indices.push_back(static_cast<std::uint16_t>(base + 3));
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(indices.size() * sizeof(std::uint16_t)),
        indices.data(),
        GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    return glGetError() == GL_NO_ERROR;
}

bool SpriteBatch::set_projection(const OrthographicCamera& camera) noexcept
{
    if (program_ == 0) return false;

    glUseProgram(program_);
    // GL_FALSE: Matrix4 already stores column-major, and GLES 3.0 rejects
    // GL_TRUE here in any case.
    glUniformMatrix4fv(projection_uniform_, 1, GL_FALSE,
                       camera.combined().data());
    glUniform1i(texture_uniform_, 0);

    // Straight alpha, which is what the decoder produces and what a skin's
    // PNGs contain.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    return true;
}

void SpriteBatch::flush() noexcept
{
    if (sprite_count_ == 0 || active_texture_ == nullptr) {
        vertices_.clear();
        sprite_count_ = 0;
        return;
    }
    if (program_ == 0 || vertex_buffer_ == 0) return;

    glUseProgram(program_);
    active_texture_->bind(0);

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glBufferSubData(
        GL_ARRAY_BUFFER, 0,
        static_cast<GLsizeiptr>(vertices_.size() * sizeof(float)),
        vertices_.data());

    constexpr GLsizei stride =
        static_cast<GLsizei>(k_floats_per_vertex * sizeof(float));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const void*>(4 * sizeof(float)));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);
    glDrawElements(
        GL_TRIANGLES,
        static_cast<GLsizei>(sprite_count_ * k_indices_per_sprite),
        GL_UNSIGNED_SHORT,
        nullptr);

    vertices_.clear();
    sprite_count_ = 0;
}

void SpriteBatch::apply_clip(int x, int y, int width, int height) noexcept
{
    if (width < 0) {
        glDisable(GL_SCISSOR_TEST);
        return;
    }
    glEnable(GL_SCISSOR_TEST);
    // A negative extent is an error to GL, and a zero one is the legitimate
    // "clipped away entirely" case a fully scrolled-out widget produces.
    glScissor(x, y,
              static_cast<GLsizei>(width < 0 ? 0 : width),
              static_cast<GLsizei>(height < 0 ? 0 : height));
}

void SpriteBatch::destroy_gpu_objects() noexcept
{
    if (vertex_buffer_ != 0 || index_buffer_ != 0) {
        const GLuint buffers[2] = {vertex_buffer_, index_buffer_};
        glDeleteBuffers(2, buffers);
        vertex_buffer_ = 0;
        index_buffer_ = 0;
    }
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
    projection_uniform_ = -1;
    texture_uniform_ = -1;
}

void SpriteBatch::release() noexcept
{
    destroy_gpu_objects();
    discard_queue();
}

void SpriteBatch::invalidate() noexcept
{
    // The context is gone: these names belong to it, and deleting them would
    // free unrelated objects in the new one. Forget them instead.
    vertex_buffer_ = 0;
    index_buffer_ = 0;
    program_ = 0;
    projection_uniform_ = -1;
    texture_uniform_ = -1;
    invalidated_ = true;
    discard_queue();
}

void SpriteBatch::destroy() noexcept
{
    destroy_gpu_objects();
    vertices_.clear();
    vertices_.shrink_to_fit();
    maximum_sprites_ = 0;
    sprite_count_ = 0;
    active_texture_ = nullptr;
    drawing_ = false;
    invalidated_ = false;
}

bool SpriteBatch::valid() const noexcept
{
    return program_ != 0 && vertex_buffer_ != 0 && index_buffer_ != 0
        && !invalidated_;
}

}  // namespace sq::graphics2d
