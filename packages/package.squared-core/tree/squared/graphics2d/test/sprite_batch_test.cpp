// SpriteBatch against the headless backend.
//
// Batching is invisible from outside: the same pixels appear whether they took
// one draw call or a thousand, and only the frame time tells you which. The
// null backend counts draw calls, so the decisions can be asserted directly.

#include <squared/files/files.hpp>
#include <squared/graphics2d/orthographic_camera.hpp>
#include <squared/graphics2d/sprite.hpp>
#include <squared/graphics2d/sprite_batch.hpp>
#include <squared/graphics2d/texture.hpp>
#include <squared/graphics2d/texture_recovery_options.hpp>
#include <squared/graphics2d/texture_region.hpp>

#include <cassert>
#include <cstddef>
#include <cstdio>

namespace sq::graphics2d::detail {
extern std::size_t g_null_draw_calls;
extern std::size_t g_null_sprites_drawn;
}

using namespace sq;
namespace counters = sq::graphics2d::detail;

namespace {

void reset() { counters::g_null_draw_calls = 0; counters::g_null_sprites_drawn = 0; }

graphics2d::Texture make_texture()
{
    const unsigned char pixels[4] = {255, 255, 255, 255};
    graphics2d::Texture texture;
    const bool created = texture.create_rgba(
        1, 1, pixels, graphics2d::TextureRecoveryOptions::retain_pixels());
    assert(created);
    return texture;
}

}  // namespace

int main()
{
    graphics2d::OrthographicCamera camera{640.0F, 480.0F};
    camera.update();

    graphics2d::Texture first = make_texture();
    graphics2d::Texture second = make_texture();
    const graphics2d::TextureRegion region_a{first};
    const graphics2d::TextureRegion region_b{second};

    // --- setup refusals ---------------------------------------------------
    {
        graphics2d::SpriteBatch batch;
        assert(!batch.valid());
        assert(!batch.initialize(0));          // a batch of nothing
        assert(!batch.initialize(1000000));    // past the 16-bit index limit
        assert(batch.initialize(64));
        assert(batch.valid());
        assert(batch.sprite_capacity() == 64);
        assert(!batch.drawing());
    }

    // --- one texture, one draw call ---------------------------------------
    {
        reset();
        graphics2d::SpriteBatch batch;
        assert(batch.initialize(64));
        assert(batch.begin(camera));
        assert(batch.drawing());
        for (int i = 0; i < 10; ++i) {
            batch.draw(region_a, static_cast<float>(i), 0.0F, 8.0F, 8.0F);
        }
        assert(batch.queued_sprites() == 10);
        assert(counters::g_null_draw_calls == 0);   // nothing drawn until flush
        batch.end();
        assert(counters::g_null_draw_calls == 1);
        assert(counters::g_null_sprites_drawn == 10);
        assert(!batch.drawing());
        assert(batch.queued_sprites() == 0);
    }

    // --- a texture switch costs exactly one extra call ---------------------
    {
        reset();
        graphics2d::SpriteBatch batch;
        assert(batch.initialize(64));
        assert(batch.begin(camera));
        batch.draw(region_a, 0.0F, 0.0F, 8.0F, 8.0F);
        batch.draw(region_a, 8.0F, 0.0F, 8.0F, 8.0F);
        batch.draw(region_b, 16.0F, 0.0F, 8.0F, 8.0F);   // flushes the first two
        assert(counters::g_null_draw_calls == 1);
        batch.end();
        assert(counters::g_null_draw_calls == 2);
        assert(counters::g_null_sprites_drawn == 3);
    }

    // --- interleaving textures is the pathological case, and shows it ------
    {
        reset();
        graphics2d::SpriteBatch batch;
        assert(batch.initialize(64));
        assert(batch.begin(camera));
        for (int i = 0; i < 6; ++i) {
            batch.draw(i % 2 == 0 ? region_a : region_b, 0.0F, 0.0F, 8.0F, 8.0F);
        }
        batch.end();
        // six sprites, six draw calls: sorting by texture is worth it, and
        // this is the number that says so
        assert(counters::g_null_draw_calls == 6);
    }

    // --- a full batch flushes at the boundary ------------------------------
    {
        reset();
        graphics2d::SpriteBatch batch;
        assert(batch.initialize(2));
        assert(batch.begin(camera));
        batch.draw(region_a, 0.0F, 0.0F, 8.0F, 8.0F);
        batch.draw(region_a, 8.0F, 0.0F, 8.0F, 8.0F);
        assert(counters::g_null_draw_calls == 0);
        batch.draw(region_a, 16.0F, 0.0F, 8.0F, 8.0F);   // does not fit
        assert(counters::g_null_draw_calls == 1);
        assert(batch.queued_sprites() == 1);
        batch.end();
        assert(counters::g_null_draw_calls == 2);
        assert(counters::g_null_sprites_drawn == 3);
    }

    // --- a stale region is skipped, not drawn ------------------------------
    {
        reset();
        graphics2d::Texture doomed = make_texture();
        graphics2d::TextureRegion region{doomed};

        graphics2d::SpriteBatch batch;
        assert(batch.initialize(64));
        assert(batch.begin(camera));
        batch.draw(region, 0.0F, 0.0F, 8.0F, 8.0F);
        assert(batch.queued_sprites() == 1);

        doomed.invalidate();
        assert(!region.valid());
        batch.draw(region, 8.0F, 0.0F, 8.0F, 8.0F);
        // still one: drawing it would sample whatever now holds that unit
        assert(batch.queued_sprites() == 1);
        batch.end();
        assert(counters::g_null_sprites_drawn == 1);
    }

    // --- draw outside begin does nothing -----------------------------------
    {
        reset();
        graphics2d::SpriteBatch batch;
        assert(batch.initialize(64));
        batch.draw(region_a, 0.0F, 0.0F, 8.0F, 8.0F);
        assert(batch.queued_sprites() == 0);
        assert(counters::g_null_draw_calls == 0);
    }

    // --- a Sprite draws, rotated and scaled --------------------------------
    {
        reset();
        graphics2d::Sprite sprite{region_a};
        sprite.set_position(10.0F, 20.0F);
        sprite.set_size(16.0F, 16.0F);
        sprite.set_origin(8.0F, 8.0F);
        sprite.set_rotation(45.0F);
        sprite.set_scale(2.0F, 2.0F);

        graphics2d::SpriteBatch batch;
        assert(batch.initialize(64));
        assert(batch.begin(camera));
        batch.draw(sprite);
        assert(batch.queued_sprites() == 1);
        batch.end();
        assert(counters::g_null_draw_calls == 1);
        assert(counters::g_null_sprites_drawn == 1);
    }

    // --- context loss ------------------------------------------------------
    {
        reset();
        graphics2d::SpriteBatch batch;
        assert(batch.initialize(64));
        assert(batch.begin(camera));
        batch.draw(region_a, 0.0F, 0.0F, 8.0F, 8.0F);

        batch.invalidate();
        assert(!batch.valid());
        // the in-flight batch is abandoned: a lost context is not a nested
        // begin, and the next one must not look like a programmer error
        assert(!batch.drawing());
        assert(batch.queued_sprites() == 0);
        assert(!batch.begin(camera));            // refuses while invalid

        assert(batch.restore());
        assert(batch.valid());
        // whatever was queued belonged to a context that is gone
        assert(batch.queued_sprites() == 0);
        assert(!batch.drawing());
        assert(counters::g_null_draw_calls == 0);

        assert(batch.begin(camera));
        batch.draw(region_a, 0.0F, 0.0F, 8.0F, 8.0F);
        batch.end();
        assert(counters::g_null_draw_calls == 1);
    }

    // --- release keeps the batch usable after restore ----------------------
    {
        graphics2d::SpriteBatch batch;
        assert(batch.initialize(64));
        batch.release();
        assert(!batch.valid());
        assert(batch.restore());
        assert(batch.valid());
    }

    std::printf("sprite batch: all assertions passed  sizeof(SpriteBatch)=%zu\n",
                sizeof(graphics2d::SpriteBatch));
    return 0;
}
