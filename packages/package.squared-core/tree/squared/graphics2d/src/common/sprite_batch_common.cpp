// SpriteBatch: everything that is not a graphics call.

#include <squared/graphics/context.hpp>
//
// Vertex building, the flush decisions, and the begin/draw/end state machine.
// The GPU half - buffers, the shader, the draw call - is in
// src/<backend>/sprite_batch.cpp, the same split Texture uses.

#include <squared/graphics2d/sprite_batch.hpp>

#include <squared/graphics/color.hpp>
#include <squared/graphics2d/sprite.hpp>
#include <squared/graphics2d/texture.hpp>
#include <squared/graphics2d/texture_region.hpp>

#include <cassert>
#include <cmath>
#include <cstddef>
#include <vector>

namespace sq::graphics2d {

namespace {

/// Floats per vertex: x, y, u, v, r, g, b, a.
constexpr std::size_t k_floats_per_vertex = 8;

/// Four corners to a quad.
constexpr std::size_t k_vertices_per_sprite = 4;

constexpr std::size_t k_floats_per_sprite =
    k_floats_per_vertex * k_vertices_per_sprite;

/**
 * @brief Upper bound on one batch.
 *
 * Six indices per sprite must stay addressable by a 16-bit index, which is
 * what the index buffer uses: 65536 / 4 corners. Larger batches would need
 * 32-bit indices for no benefit - a flush is cheap and a batch this size is
 * already far past the point where one more makes any difference.
 */
constexpr std::size_t k_maximum_sprites = 16384;

constexpr float k_degrees_to_radians = 3.14159265358979323846F / 180.0F;

}  // namespace

SpriteBatch::~SpriteBatch()
{
    destroy();
}

bool SpriteBatch::initialize(std::size_t maximum_sprites) noexcept
{
    if (maximum_sprites == 0 || maximum_sprites > k_maximum_sprites) {
        return false;
    }

    destroy();
    maximum_sprites_ = maximum_sprites;

    // The whole batch is allocated once, here, and never grows: a frame loop
    // must not allocate, and a vector that reallocated mid-frame would be
    // exactly that. Capacity is the budget, and flush() is what enforces it.
    vertices_.assign(maximum_sprites_ * k_floats_per_sprite, 0.0F);
    vertices_.clear();

    if (!allocate_gpu_objects()) {
        destroy();
        return false;
    }
    return true;
}

bool SpriteBatch::begin(const OrthographicCamera& camera) noexcept
{
    // Nested begin() is a programmer error, not a runtime condition: it means
    // two pieces of code believe they own the batch.
    assert(!drawing_ && "SpriteBatch::begin called while already drawing");
    if (drawing_ || !valid()) return false;

    drawing_ = true;
    sprite_count_ = 0;
    skipped_draws_ = 0;
    active_texture_ = nullptr;
    vertices_.clear();
    return set_projection(camera);
}

void SpriteBatch::draw(
    const TextureRegion& region,
    float x,
    float y,
    float width,
    float height,
    sq::graphics::Color color
) noexcept
{
    if (!drawing_) return;

    // A region whose texture was discarded or reloaded under it would sample
    // whatever now occupies that texture unit. Skipping is the only safe
    // answer, and it is why TextureRegion carries a generation at all.
    if (!region.valid()) {
        ++skipped_draws_;
        return;
    }

    const float positions[8] = {
        x,         y,
        x,         y + height,
        x + width, y + height,
        x + width, y
    };
    append_quad(region, positions, color);
}

void SpriteBatch::draw(const Sprite& sprite) noexcept
{
    if (!drawing_) return;
    const TextureRegion& region = sprite.region();
    if (!region.valid()) {
        ++skipped_draws_;
        return;
    }

    const float origin_x = sprite.origin_x();
    const float origin_y = sprite.origin_y();
    const float scale_x = sprite.scale_x();
    const float scale_y = sprite.scale_y();

    // Corners relative to the origin, scaled, then rotated about it, then
    // translated. Done here rather than in a matrix because a sprite is four
    // points and a 4x4 multiply per corner would be three quarters waste.
    const float left = -origin_x * scale_x;
    const float bottom = -origin_y * scale_y;
    const float right = (sprite.width() - origin_x) * scale_x;
    const float top = (sprite.height() - origin_y) * scale_y;

    const float radians = sprite.rotation() * k_degrees_to_radians;
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);

    const float pivot_x = sprite.x() + origin_x;
    const float pivot_y = sprite.y() + origin_y;

    const float corners[8] = {left, bottom, left, top, right, top, right, bottom};
    float positions[8];
    for (std::size_t corner = 0; corner < 4; ++corner) {
        const float corner_x = corners[corner * 2];
        const float corner_y = corners[corner * 2 + 1];
        positions[corner * 2] =
            pivot_x + (corner_x * cosine) - (corner_y * sine);
        positions[corner * 2 + 1] =
            pivot_y + (corner_x * sine) + (corner_y * cosine);
    }
    append_quad(region, positions, sprite.color());
}

void SpriteBatch::append_quad(
    const TextureRegion& region,
    const float* positions,
    sq::graphics::Color color
) noexcept
{
    const Texture* texture = &region.texture();

    // Two reasons to flush, and both are the same reason: what is queued can
    // no longer be drawn in one call. A different texture cannot share a draw
    // call with what is queued, and a full buffer has nowhere to put this.
    if (active_texture_ != nullptr && texture != active_texture_) {
        flush();
    }
    if (sprite_count_ >= maximum_sprites_) {
        flush();
    }
    active_texture_ = texture;

    const float u1 = region.u1();
    const float v1 = region.v1();
    const float u2 = region.u2();
    const float v2 = region.v2();

    // Corner order matches positions: bottom-left, top-left, top-right,
    // bottom-right. v1 is the top edge, so the bottom corners take v2.
    const float texture_coordinates[8] = {u1, v2, u1, v1, u2, v1, u2, v2};

    for (std::size_t corner = 0; corner < k_vertices_per_sprite; ++corner) {
        vertices_.push_back(positions[corner * 2]);
        vertices_.push_back(positions[corner * 2 + 1]);
        vertices_.push_back(texture_coordinates[corner * 2]);
        vertices_.push_back(texture_coordinates[corner * 2 + 1]);
        vertices_.push_back(color.red);
        vertices_.push_back(color.green);
        vertices_.push_back(color.blue);
        vertices_.push_back(color.alpha);
    }
    ++sprite_count_;
}

void SpriteBatch::set_clip(int x, int y, int width, int height) noexcept
{
    // Everything queued was drawn under the previous clip and must be issued
    // before this one takes effect.
    flush();
    apply_clip(x, y, width, height);
}

void SpriteBatch::clear_clip() noexcept
{
    flush();
    apply_clip(0, 0, -1, -1);
}

void SpriteBatch::end() noexcept
{
    assert(drawing_ && "SpriteBatch::end called without begin");
    if (!drawing_) return;
    flush();
    // A clip is a property of the drawing that asked for it, not of the batch.
    // Leaving one set would silently restrict whatever draws next.
    apply_clip(0, 0, -1, -1);
    drawing_ = false;
    active_texture_ = nullptr;
}

void SpriteBatch::discard_queue() noexcept
{
    // A batch in progress does not survive losing its context. Leaving
    // drawing_ set would make the next begin() look like a nested one - a
    // programmer error - when the real cause was the context going away.
    drawing_ = false;
    sprite_count_ = 0;
    active_texture_ = nullptr;
    vertices_.clear();
}

bool SpriteBatch::restore(const graphics::Context& graphics) noexcept
{
    // The context knows whether anything was lost; asking it removes the one
    // decision a caller could get backwards.
    return restore(graphics.resources_preserved());
}

bool SpriteBatch::restore(bool context_preserved) noexcept
{
    if (context_preserved && !invalidated_ && valid()) return true;

    discard_queue();
    invalidated_ = false;
    return allocate_gpu_objects();
}

std::size_t SpriteBatch::sprite_capacity() const noexcept
{
    return maximum_sprites_;
}

std::size_t SpriteBatch::skipped_draws() const noexcept
{
    return skipped_draws_;
}

std::size_t SpriteBatch::queued_sprites() const noexcept
{
    return sprite_count_;
}

bool SpriteBatch::drawing() const noexcept
{
    return drawing_;
}

}  // namespace sq::graphics2d
