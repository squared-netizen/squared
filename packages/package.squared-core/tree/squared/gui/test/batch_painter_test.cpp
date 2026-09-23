// BatchPainter against the headless backend.
//
// Two things here cannot be seen by looking at the screen: how many draw calls
// an interface costs, and whether a clip rectangle was converted into the
// right scissor. The null backend records both.

#include <squared/graphics2d/orthographic_camera.hpp>
#include <squared/graphics2d/sprite_batch.hpp>
#include <squared/graphics2d/texture.hpp>
#include <squared/graphics2d/texture_recovery_options.hpp>
#include <squared/graphics2d/texture_region.hpp>
#include <squared/gui/batch_painter.hpp>
#include <squared/gui/rectangle.hpp>

#include <cassert>
#include <cstddef>
#include <cstdio>

namespace sq::graphics2d::detail {
extern std::size_t g_null_draw_calls;
extern std::size_t g_null_sprites_drawn;
extern int g_null_clip_x;
extern int g_null_clip_y;
extern int g_null_clip_width;
extern int g_null_clip_height;
}  // namespace sq::graphics2d::detail

using namespace sq;
namespace counters = sq::graphics2d::detail;

namespace {

void reset()
{
    counters::g_null_draw_calls = 0;
    counters::g_null_sprites_drawn = 0;
    counters::g_null_clip_width = -1;
}

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

    // One "atlas page" holding both the widget graphics and the white texel,
    // which is how a real skin is packed.
    graphics2d::Texture atlas_page = make_texture();
    const graphics2d::TextureRegion skin_region{atlas_page};
    const graphics2d::TextureRegion skin_white{atlas_page};

    graphics2d::SpriteBatch batch;
    assert(batch.initialize(256));

    // --- the whole point of taking white from the skin --------------------
    {
        reset();
        gui::BatchPainter painter{batch, nullptr};
        assert(painter.set_fill_source(&skin_white));

        assert(batch.begin(camera));
        painter.draw_region(skin_region, {0.0F, 0.0F, 10.0F, 10.0F});
        painter.fill_rectangle({0.0F, 0.0F, 10.0F, 10.0F},
                               graphics::Color::white());
        painter.draw_region(skin_region, {10.0F, 0.0F, 10.0F, 10.0F});
        painter.fill_rectangle({10.0F, 0.0F, 10.0F, 10.0F},
                               graphics::Color::white());
        batch.end();

        // fills share the page with the graphics, so it is all one call
        assert(counters::g_null_draw_calls == 1);
        assert(counters::g_null_sprites_drawn == 4);
    }

    // --- and what it costs when the skin has no white region --------------
    {
        reset();
        gui::BatchPainter painter{batch, nullptr};
        assert(painter.set_fill_source(nullptr));   // owns a 1x1 instead

        assert(batch.begin(camera));
        painter.draw_region(skin_region, {0.0F, 0.0F, 10.0F, 10.0F});
        painter.fill_rectangle({0.0F, 0.0F, 10.0F, 10.0F},
                               graphics::Color::white());
        painter.draw_region(skin_region, {10.0F, 0.0F, 10.0F, 10.0F});
        painter.fill_rectangle({10.0F, 0.0F, 10.0F, 10.0F},
                               graphics::Color::white());
        batch.end();

        // every fill is a texture switch: four sprites, four draw calls
        assert(counters::g_null_draw_calls == 4);
    }

    // --- stroke is four fills ---------------------------------------------
    {
        reset();
        gui::BatchPainter painter{batch, nullptr};
        assert(painter.set_fill_source(&skin_white));
        assert(batch.begin(camera));
        painter.stroke_rectangle({0.0F, 0.0F, 20.0F, 20.0F},
                                 graphics::Color::white(), 2.0F);
        batch.end();
        assert(counters::g_null_sprites_drawn == 4);
        assert(counters::g_null_draw_calls == 1);
    }

    // --- the clip conversion: logical, top-left -> pixels, bottom-left -----
    {
        reset();
        gui::BatchPainter painter{batch, nullptr};
        assert(painter.set_fill_source(&skin_white));
        // logical 640x480 drawn into a 1280x960 framebuffer: scale 2
        painter.set_viewport(640.0F, 480.0F, 1280, 960);

        assert(batch.begin(camera));
        // a band across the top of the screen in GUI coordinates
        painter.push_clip({0.0F, 0.0F, 640.0F, 48.0F});
        assert(painter.clip_depth() == 1);
        assert(counters::g_null_clip_x == 0);
        assert(counters::g_null_clip_width == 1280);
        assert(counters::g_null_clip_height == 96);
        // the top of the GUI is the top of the framebuffer, which in scissor
        // coordinates is the far end: 960 - 0 - 96
        assert(counters::g_null_clip_y == 864);

        painter.pop_clip();
        assert(painter.clip_depth() == 0);
        assert(counters::g_null_clip_width == -1);   // cleared
        batch.end();
    }

    // --- nested clips intersect, never widen ------------------------------
    {
        reset();
        gui::BatchPainter painter{batch, nullptr};
        assert(painter.set_fill_source(&skin_white));
        painter.set_viewport(640.0F, 480.0F, 640, 480);

        assert(batch.begin(camera));
        painter.push_clip({100.0F, 100.0F, 200.0F, 200.0F});
        // a child asking for more than its parent allowed gets the overlap
        painter.push_clip({0.0F, 0.0F, 640.0F, 480.0F});
        assert(counters::g_null_clip_x == 100);
        assert(counters::g_null_clip_width == 200);
        assert(counters::g_null_clip_height == 200);

        painter.pop_clip();
        assert(counters::g_null_clip_x == 100);      // back to the parent
        painter.pop_clip();
        assert(counters::g_null_clip_width == -1);
        batch.end();
    }

    // --- a clip change flushes what was queued under the old one ----------
    {
        reset();
        gui::BatchPainter painter{batch, nullptr};
        assert(painter.set_fill_source(&skin_white));
        painter.set_viewport(640.0F, 480.0F, 640, 480);

        assert(batch.begin(camera));
        painter.fill_rectangle({0.0F, 0.0F, 10.0F, 10.0F},
                               graphics::Color::white());
        assert(counters::g_null_draw_calls == 0);
        painter.push_clip({0.0F, 0.0F, 100.0F, 100.0F});
        // the queued fill belonged to the unclipped state and had to be drawn
        assert(counters::g_null_draw_calls == 1);
        painter.fill_rectangle({0.0F, 0.0F, 10.0F, 10.0F},
                               graphics::Color::white());
        painter.pop_clip();
        assert(counters::g_null_draw_calls == 2);
        batch.end();
    }

    // --- nesting past the tracked depth does not misclip -------------------
    {
        reset();
        gui::BatchPainter painter{batch, nullptr};
        assert(painter.set_fill_source(&skin_white));
        painter.set_viewport(640.0F, 480.0F, 640, 480);
        assert(batch.begin(camera));

        painter.push_clip({10.0F, 10.0F, 100.0F, 100.0F});
        for (int i = 0; i < 40; ++i) {
            painter.push_clip({0.0F, 0.0F, 640.0F, 480.0F});
        }
        assert(painter.clip_depth() == 41);
        // still the innermost tracked clip, not a wider one
        assert(counters::g_null_clip_width == 100);
        for (int i = 0; i < 41; ++i) painter.pop_clip();
        assert(painter.clip_depth() == 0);
        assert(counters::g_null_clip_width == -1);
        batch.end();
    }

    // --- an unbalanced pop is ignored rather than underflowing -------------
    {
        gui::BatchPainter painter{batch, nullptr};
        painter.pop_clip();
        assert(painter.clip_depth() == 0);
    }

    // --- text with no font draws nothing and measures to zero -------------
    {
        reset();
        gui::BatchPainter painter{batch, nullptr};
        assert(painter.set_fill_source(&skin_white));
        const gui::Size size = painter.measure_text("hello");
        assert(size.width == 0.0F && size.height == 0.0F);

        assert(batch.begin(camera));
        painter.draw_text("hello", 0.0F, 0.0F, graphics::Color::white());
        batch.end();
        assert(counters::g_null_draw_calls == 0);
    }

    // --- end() leaves no clip set for whatever draws next -----------------
    {
        reset();
        gui::BatchPainter painter{batch, nullptr};
        assert(painter.set_fill_source(&skin_white));
        painter.set_viewport(640.0F, 480.0F, 640, 480);
        assert(batch.begin(camera));
        painter.push_clip({0.0F, 0.0F, 10.0F, 10.0F});
        batch.end();
        assert(counters::g_null_clip_width == -1);
    }

    std::printf(
        "batch painter: all assertions passed  sizeof(BatchPainter)=%zu\n",
        sizeof(gui::BatchPainter));
    return 0;
}
