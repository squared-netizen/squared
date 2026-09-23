// TextureAtlas against a real libGDX atlas.
//
// The file below is the header and three regions of the stock libGDX default
// skin, verbatim - including a rotated region and a nine-patch with splits and
// pads, which are the two cases a hand-written fixture usually omits.

#include <squared/files/files.hpp>
#include <squared/graphics2d/atlas_region.hpp>
#include <squared/graphics2d/texture_atlas.hpp>
#include <squared/graphics2d/texture_filter.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO
#include <stb_image_write.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <span>
#include <string>
#include <vector>

using namespace sq;

namespace {

std::vector<unsigned char> g_png;

void collect(void*, void* data, int size)
{
    const auto* bytes = static_cast<const unsigned char*>(data);
    g_png.insert(g_png.end(), bytes, bytes + size);
}

constexpr const char* k_atlas = R"(
uiskin.png
size: 256,128
format: RGBA8888
filter: Nearest,Nearest
repeat: none
check-off
  rotate: false
  xy: 82, 78
  size: 13, 13
  orig: 13, 13
  offset: 0, 0
  index: -1
default-rect
  rotate: false
  xy: 2, 42
  size: 12, 12
  split: 1, 1, 1, 1
  pad: 2, 2, 2, 2
  orig: 12, 12
  offset: 0, 0
  index: -1
font-page
  rotate: true
  xy: 100, 2
  size: 20, 10
  orig: 20, 10
  offset: 0, 0
  index: 3
)";

}  // namespace

int main()
{
    const char* tmpdir = std::getenv("TMPDIR");
    const std::string base =
        std::string{tmpdir != nullptr ? tmpdir : "."} + "/sq_atlas_test";
    files::PosixFileSystem fs{files::PosixFileSystemRoots{
        .internal_root = base, .local_root = base, .external_root = {}}};

    // a 256x128 page, so the region coordinates above are inside it
    std::vector<unsigned char> pixels(256U * 128U * 4U, 200);
    assert(stbi_write_png_to_func(collect, nullptr, 256, 128, 4,
                                  pixels.data(), 256 * 4));
    assert(!fs.local("skin/uiskin.png")
                .write_bytes(std::as_bytes(std::span{g_png})));
    assert(!fs.local("skin/uiskin.atlas").write_string(k_atlas));

    files::FileHandle handle = fs.local("skin/uiskin.atlas");

    // --- parse ------------------------------------------------------------
    {
        graphics2d::TextureAtlas atlas;
        assert(atlas.load(handle));
        assert(atlas.valid());
        assert(atlas.page_count() == 1);
        assert(atlas.region_count() == 3);

        const graphics2d::AtlasRegion* check = atlas.find_region("check-off");
        assert(check != nullptr);
        assert(check->index() == -1);
        assert(check->region().valid());
        assert(check->region().width() == 13);
        assert(check->packed_width() == 13 && check->original_height() == 13);
        assert(!check->splits() && !check->pads());
        assert(!check->rotated_clockwise());

        // the nine-patch: splits and pads are what the GUI draws borders from
        const graphics2d::AtlasRegion* rect = atlas.find_region("default-rect");
        assert(rect != nullptr);
        assert(rect->splits());
        assert((*rect->splits())[0] == 1 && (*rect->splits())[3] == 1);
        assert(rect->pads());
        assert((*rect->pads())[1] == 2);

        // a rotated region keeps its logical size; the storage is swapped
        const graphics2d::AtlasRegion* page = atlas.find_region("font-page");
        assert(page != nullptr);
        assert(page->rotated_clockwise());
        assert(page->region().width() == 20 && page->region().height() == 10);
        assert(page->index() == 3);

        // index selects between same-named regions; -1 means any
        assert(atlas.find_region("font-page", 3) == page);
        assert(atlas.find_region("font-page", 9) == nullptr);
        assert(atlas.find_region("missing") == nullptr);
    }

    // --- the page's filter comes from the file ----------------------------
    {
        // Nearest,Nearest above: a pixel-art skin is ruined by anything else,
        // and nothing in the API lets a caller override it.
        graphics2d::TextureAtlas atlas;
        assert(atlas.load(handle));
        // the region is usable, which is what the filter affects at draw time
        assert(atlas.find_region("check-off")->region().valid());
    }

    // --- context loss: regions go invalid, then come back ------------------
    {
        graphics2d::TextureAtlas atlas;
        assert(atlas.load(handle));
        const graphics2d::AtlasRegion* check = atlas.find_region("check-off");
        assert(check->region().valid());

        atlas.invalidate();
        assert(!atlas.valid());
        assert(!check->region().valid());
        assert(check->region().stale());

        assert(atlas.restore());
        assert(atlas.valid());
        // refreshed on the atlas's behalf: same file, same layout
        assert(check->region().valid());
        assert(!check->region().stale());
    }

    // --- release keeps the recipe -----------------------------------------
    {
        graphics2d::TextureAtlas atlas;
        assert(atlas.load(handle));
        atlas.release();
        assert(!atlas.valid());
        assert(atlas.restore());
        assert(atlas.valid());
        assert(atlas.find_region("check-off")->region().valid());
    }

    // --- refusals ---------------------------------------------------------
    {
        graphics2d::TextureAtlas atlas;
        // Regenerate needs a callback there is nowhere to pass
        assert(!atlas.load(handle, graphics2d::TextureRecoveryPolicy::Regenerate));
        assert(!atlas.load(fs.local("skin/missing.atlas")));
        assert(!atlas.load(files::FileHandle{}));
        assert(atlas.page_count() == 0);

        // a page the atlas names but the filesystem lacks
        assert(!fs.local("broken/broken.atlas")
                    .write_string("\nnope.png\nsize: 4,4\nfilter: Linear,Linear\n"
                                  "repeat: none\nthing\n  xy: 0, 0\n  size: 2, 2\n"));
        assert(!atlas.load(fs.local("broken/broken.atlas")));
    }

    // --- Discard: regions stay listed and report themselves invalid --------
    {
        graphics2d::TextureAtlas atlas;
        assert(atlas.load(handle, graphics2d::TextureRecoveryPolicy::Discard));
        const std::size_t before = atlas.region_count();
        atlas.invalidate();
        assert(!atlas.restore());
        assert(atlas.region_count() == before);
        assert(!atlas.find_region("check-off")->region().valid());
    }

    assert(!fs.local("skin/uiskin.atlas").remove());
    assert(!fs.local("skin/uiskin.png").remove());
    assert(!fs.local("skin").remove());
    assert(!fs.local("broken/broken.atlas").remove());
    assert(!fs.local("broken").remove());
    assert(!fs.local("").remove());

    std::printf("texture atlas: all assertions passed  sizeof(AtlasRegion)=%zu\n",
                sizeof(graphics2d::AtlasRegion));
    return 0;
}
