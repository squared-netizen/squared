// Texture against the headless backend.
//
// Recovery is the reason this test exists. Every branch of it runs once per
// context loss on a device, which is minutes apart and hard to provoke; here
// it runs in microseconds and the assertions can see state a device cannot be
// asked about.

#include <squared/files/files.hpp>
#include <squared/graphics2d/atlas_region.hpp>
#include <squared/graphics/context.hpp>
#include <squared/graphics/context_config.hpp>
#include <squared/graphics2d/texture.hpp>
#include <squared/graphics2d/texture_region.hpp>
#include <squared/graphics2d/texture_recovery_options.hpp>
#include <squared/graphics2d/texture_recovery_target.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO
#include <stb_image_write.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace sq;

namespace {

std::vector<unsigned char> g_encoded;

void collect(void*, void* data, int size)
{
    const auto* bytes = static_cast<const unsigned char*>(data);
    g_encoded.insert(g_encoded.end(), bytes, bytes + size);
}

int g_regenerate_calls = 0;

bool regenerate(void*, graphics2d::TextureRecoveryTarget& target) noexcept
{
    ++g_regenerate_calls;
    const unsigned char pixel[4] = {9, 8, 7, 255};
    return target.upload_rgba(1, 1, pixel);
}

bool regenerate_lying(void*, graphics2d::TextureRecoveryTarget&) noexcept
{
    return true;   // claims success, uploads nothing
}

}  // namespace

int main()
{
    // Not /tmp: Termux has no writable /tmp, and this test runs on device as
    // often as on a host. TMPDIR is set there; a relative directory is the
    // fallback, and PosixFileSystem creates the root on first write either way.
    const char* tmpdir = std::getenv("TMPDIR");
    const std::string base =
        std::string{tmpdir != nullptr ? tmpdir : "."} + "/sq_texture_test";
    files::PosixFileSystem fs{files::PosixFileSystemRoots{
        .internal_root = base, .local_root = base, .external_root = {}
    }};

    // a 2x2 RGBA png written to the filesystem under test
    unsigned char source[2 * 2 * 4];
    for (int i = 0; i < 4; ++i) {
        source[i * 4 + 0] = static_cast<unsigned char>(i * 60);
        source[i * 4 + 1] = 1;
        source[i * 4 + 2] = 2;
        source[i * 4 + 3] = 255;
    }
    assert(stbi_write_png_to_func(collect, nullptr, 2, 2, 4, source, 2 * 4));
    files::FileHandle image = fs.local("art/hero.png");
    assert(!image.write_bytes(std::as_bytes(std::span{g_encoded})));

    // --- load, and the generation moves ----------------------------------
    {
        graphics2d::Texture texture;
        const std::uint32_t before = texture.content_generation();
        assert(texture.load(image));
        assert(texture.valid() && texture.has_content());
        assert(texture.width() == 2 && texture.height() == 2);
        assert(texture.content_generation() != before);
        assert(texture.recovery_policy()
               == graphics2d::TextureRecoveryPolicy::ReloadFromAsset);
        assert(texture.restorable());
        // ReloadFromAsset keeps no pixels: that is the whole point of it
        assert(texture.retained_recovery_bytes() == 0);
    }

    // --- invalidate must not lose the recipe, and must move the generation
    {
        graphics2d::Texture texture;
        assert(texture.load(image));
        const std::uint32_t before = texture.content_generation();

        texture.invalidate();
        assert(!texture.valid());
        assert(!texture.has_content());
        assert(texture.content_generation() != before);
        assert(texture.restorable());          // the handle is still there

        assert(texture.restore(false));
        assert(texture.has_content());
        assert(texture.width() == 2 && texture.height() == 2);
    }

    // --- context_preserved skips the work --------------------------------
    {
        graphics2d::Texture texture;
        assert(texture.load(image));
        const std::uint32_t before = texture.content_generation();
        assert(texture.restore(true));
        assert(texture.content_generation() == before);   // nothing happened
    }

    // --- RetainPixels costs bytes and needs no file ----------------------
    {
        graphics2d::Texture texture;
        assert(texture.create_rgba(2, 2, source,
               graphics2d::TextureRecoveryOptions::retain_pixels()));
        assert(texture.retained_recovery_bytes() >= 2 * 2 * 4);
        assert(texture.restorable());

        texture.invalidate();
        assert(texture.restore(false));
        assert(texture.has_content() && texture.width() == 2);
    }

    // --- create-from-memory refuses ReloadFromAsset ----------------------
    {
        graphics2d::Texture texture;
        graphics2d::TextureRecoveryOptions options;   // defaults to reload
        assert(options.policy
               == graphics2d::TextureRecoveryPolicy::ReloadFromAsset);
        assert(!texture.create_rgba(2, 2, source, options));
        assert(!texture.valid());

        // and Regenerate without a callback is refused too
        graphics2d::TextureRecoveryOptions broken;
        broken.policy = graphics2d::TextureRecoveryPolicy::Regenerate;
        broken.callback = nullptr;
        assert(!texture.create_rgba(2, 2, source, broken));
    }

    // --- the no-argument create paths pick a policy that works -----------
    {
        graphics2d::Texture texture;
        assert(texture.create_rgba(2, 2, source));
        assert(texture.recovery_policy()
               == graphics2d::TextureRecoveryPolicy::RetainPixels);
        assert(texture.restorable());

        graphics2d::Texture solid;
        assert(solid.create_solid(
            graphics::Color{.red = 1.0F, .green = 0.5F, .blue = 0.0F,
                            .alpha = 1.0F}));
        assert(solid.width() == 1 && solid.height() == 1);
        assert(solid.restorable());
    }

    // --- Regenerate runs the callback ------------------------------------
    {
        g_regenerate_calls = 0;
        graphics2d::Texture texture;
        assert(texture.create_rgba(2, 2, source,
               graphics2d::TextureRecoveryOptions::regenerate(regenerate)));
        assert(g_regenerate_calls == 0);       // not called at creation
        assert(texture.retained_recovery_bytes() == 0);

        texture.invalidate();
        assert(texture.restore(false));
        assert(g_regenerate_calls == 1);
        assert(texture.width() == 1);          // the callback's own size

        // a callback that claims success without uploading is a failure
        graphics2d::Texture liar;
        assert(liar.create_rgba(2, 2, source,
               graphics2d::TextureRecoveryOptions::regenerate(
                   regenerate_lying)));
        liar.invalidate();
        assert(!liar.restore(false));
    }

    // --- Discard: the signal that stops a crash after a phone call -------
    {
        graphics2d::Texture texture;
        assert(texture.create_rgba(2, 2, source,
               graphics2d::TextureRecoveryOptions::discard()));
        const std::uint32_t built_against = texture.content_generation();
        assert(!texture.restorable());

        texture.invalidate();
        assert(!texture.restore(false));
        assert(!texture.has_content());
        assert(texture.width() == 0 && texture.height() == 0);
        // a region built against built_against can now tell it is stale
        assert(texture.content_generation() != built_against);
    }

    // --- a missing file fails rather than half-loading --------------------
    {
        graphics2d::Texture texture;
        assert(!texture.load(fs.local("art/missing.png")));
        assert(!texture.valid());

        graphics2d::Texture unbound;
        assert(!unbound.load(files::FileHandle{}));
    }

    // --- move does not double-free or duplicate the recipe ----------------
    {
        graphics2d::Texture first;
        assert(first.load(image));
        graphics2d::Texture second = std::move(first);
        assert(second.has_content() && second.restorable());
        assert(!first.valid() && !first.restorable());
    }

    // --- restore(context) asks the context, so polarity cannot be wrong ---
    {
        graphics::Context context;
        graphics::ContextConfig config;
        config.logical_width = 320;
        config.logical_height = 240;
        assert(context.create(config));

        // A first context has lost nothing, so a restore through it is a
        // no-op rather than a reload. This is what stopped every application
        // reloading its assets before its first frame.
        assert(context.resources_preserved());

        graphics2d::Texture texture;
        assert(texture.load(image));
        const std::uint32_t before = texture.content_generation();
        assert(texture.restore(context));
        assert(texture.content_generation() == before);
    }

    // Leave nothing behind, including the root this test created.
    // --- a region survives a restore that puts the same pixels back -------
    {
        graphics2d::Texture texture;
        assert(texture.load(image));
        graphics2d::TextureRegion whole{texture};
        assert(whole.valid() && !whole.stale());
        assert(whole.width() == 2 && whole.height() == 2);

        texture.invalidate();
        // the texture is gone, and the region knows without being told
        assert(!whole.valid());
        assert(whole.stale());

        assert(texture.restore(false));
        // still stale: the content came back, but it is a different upload,
        // and a region cannot know the layout matches
        assert(whole.stale());
        assert(!whole.valid());

        // refresh() is the caller saying it does match
        assert(whole.refresh());
        assert(whole.valid() && !whole.stale());
    }

    // --- Discard leaves every region into it invalid, forever -------------
    {
        graphics2d::Texture texture;
        assert(texture.create_rgba(2, 2, source,
               graphics2d::TextureRecoveryOptions::discard()));
        graphics2d::TextureRegion region{texture, 0, 0, 1, 1};
        assert(region.valid());

        texture.invalidate();
        assert(!texture.restore(false));
        assert(!region.valid());
        assert(region.stale());
        // and refresh cannot rescue it, because there is nothing to bind to
        assert(!region.refresh());
    }

    // --- a subregion inherits the generation ------------------------------
    {
        graphics2d::Texture texture;
        assert(texture.create_rgba(2, 2, source,
               graphics2d::TextureRecoveryOptions::retain_pixels()));
        graphics2d::TextureRegion whole{texture};
        graphics2d::TextureRegion half = whole.subregion(0, 0, 1, 2);
        assert(half.valid());
        assert(half.width() == 1 && half.height() == 2);

        texture.invalidate();
        assert(!half.valid() && half.stale());
    }

    // --- an unbacked region is invalid but never stale --------------------
    {
        graphics2d::TextureRegion logical{16, 16};
        assert(!logical.valid());
        assert(!logical.stale());        // it never had a texture to lose
        assert(logical.width() == 16);

        graphics2d::TextureRegion empty;
        assert(!empty.valid() && !empty.stale());
    }

    // --- rotated storage swaps the extents, not the logical size ----------
    {
        graphics2d::Texture texture;
        assert(texture.create_rgba(2, 2, source,
               graphics2d::TextureRecoveryOptions::retain_pixels()));
        graphics2d::TextureRegion upright{texture, 0, 0, 2, 1, false};
        graphics2d::TextureRegion turned{texture, 0, 0, 2, 1, true};
        assert(upright.width() == turned.width());
        assert(upright.height() == turned.height());
        assert(!upright.rotated_clockwise() && turned.rotated_clockwise());
        // storage extents differ even though the logical size does not
        assert(upright.u2() != turned.u2());
    }

    assert(!image.remove());
    assert(!fs.local("art").remove());
    assert(!fs.local("").remove());

    std::printf("texture: all assertions passed  sizeof(Texture)=%zu "
                "TextureRegion=%zu AtlasRegion=%zu\n",
                sizeof(graphics2d::Texture),
                sizeof(graphics2d::TextureRegion),
                sizeof(graphics2d::AtlasRegion));
    return 0;
}
