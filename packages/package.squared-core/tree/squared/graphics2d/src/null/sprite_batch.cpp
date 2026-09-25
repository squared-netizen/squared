// The GPU half of SpriteBatch, headless.
//
// Nothing is drawn, but every observable behaves as it does against a real
// driver: objects are allocated and released, valid() tracks them, and a flush
// consumes the queue. That is what lets the batching logic - flush on texture
// switch, flush when full, the begin/draw/end state - be tested on a host.
//
// It also counts what it would have drawn. A draw call count is the thing a
// batching bug shows up in, and it is invisible in a real driver.

#include <squared/graphics2d/sprite_batch.hpp>

#include <squared/graphics2d/orthographic_camera.hpp>
#include <squared/graphics2d/texture.hpp>

#include <cstddef>

namespace sq::graphics2d {

namespace detail {

// Not in the public header: a test observes these, an application has no
// business knowing a null backend exists at all.
std::size_t g_null_draw_calls = 0;
std::size_t g_null_sprites_drawn = 0;

// What the last clip call asked for, so a test can assert the conversion from
// logical units to a scissor rectangle without a driver.
int g_null_clip_x = 0;
int g_null_clip_y = 0;
int g_null_clip_width = -1;
int g_null_clip_height = -1;

// Texture coordinates of the first quad in the last flush, in corner order
// (x, y), (x, y + h), (x + w, y + h), (x + w, y), as u,v pairs. Enough to
// assert that an image's top edge lands at the top of its quad.
float g_null_first_quad_uv[8] = {};

}  // namespace detail

namespace {

/// Next headless object name. Zero stays reserved for "no object".
unsigned int g_next_name = 1;

}  // namespace

bool SpriteBatch::allocate_gpu_objects() noexcept
{
    destroy_gpu_objects();
    program_ = g_next_name++;
    vertex_buffer_ = g_next_name++;
    index_buffer_ = g_next_name++;
    projection_uniform_ = 0;
    texture_uniform_ = 1;
    return true;
}

bool SpriteBatch::set_projection(const OrthographicCamera& camera) noexcept
{
    static_cast<void>(camera);
    return program_ != 0;
}

void SpriteBatch::flush() noexcept
{
    if (sprite_count_ == 0 || active_texture_ == nullptr) {
        vertices_.clear();
        sprite_count_ = 0;
        return;
    }
    if (program_ == 0 || vertex_buffer_ == 0) return;

    ++detail::g_null_draw_calls;
    detail::g_null_sprites_drawn += sprite_count_;

    // Each vertex is x, y, u, v, r, g, b, a.
    for (std::size_t corner = 0; corner < 4; ++corner) {
        detail::g_null_first_quad_uv[corner * 2] = vertices_[corner * 8 + 2];
        detail::g_null_first_quad_uv[corner * 2 + 1] = vertices_[corner * 8 + 3];
    }

    vertices_.clear();
    sprite_count_ = 0;
}

void SpriteBatch::apply_clip(int x, int y, int width, int height) noexcept
{
    detail::g_null_clip_x = x;
    detail::g_null_clip_y = y;
    detail::g_null_clip_width = width;
    detail::g_null_clip_height = height;
}

void SpriteBatch::destroy_gpu_objects() noexcept
{
    vertex_buffer_ = 0;
    index_buffer_ = 0;
    program_ = 0;
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
