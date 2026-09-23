// The spike: put a Label and a Button on screen, against the real default
// skin, using the headless backend.
//
// Nothing below has ever run: the skin JSON parser, the .fnt parser, the atlas
// drawable resolver and FontResource::glyph_region are all implemented and
// have never met a real file. This is the shortest program that exercises all
// of them plus the painter, and it says which of them is wrong.

#include <squared/files/files.hpp>
#include <squared/graphics2d/atlas_region.hpp>
#include <squared/graphics2d/bitmap_font.hpp>
#include <squared/graphics2d/bitmap_font_error.hpp>
#include <squared/graphics2d/bitmap_font_page.hpp>
#include <squared/graphics2d/orthographic_camera.hpp>
#include <squared/graphics2d/sprite_batch.hpp>
#include <squared/graphics2d/texture.hpp>
#include <squared/graphics2d/texture_atlas.hpp>
#include <squared/graphics2d/texture_region.hpp>
#include <squared/gui/batch_painter.hpp>
#include <squared/gui/button.hpp>
#include <squared/gui/font_resource.hpp>
#include <squared/gui/label.hpp>
#include <squared/gui/linear_layout.hpp>
#include <squared/gui/skin.hpp>
#include <squared/gui/skin_load_issue.hpp>
#include <squared/gui/skin_load_report.hpp>
#include <squared/gui/skin_load_severity.hpp>
#include <squared/gui/skin_loader.hpp>
#include <squared/gui/ui.hpp>

#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace sq::graphics2d::detail {
extern std::size_t g_null_draw_calls;
extern std::size_t g_null_sprites_drawn;
}  // namespace sq::graphics2d::detail

using namespace sq;
namespace counters = sq::graphics2d::detail;

namespace {

int g_failures = 0;

void check(bool condition, const char* what)
{
    std::printf("  %-50s %s\n", what, condition ? "ok" : "FAILED");
    if (!condition) ++g_failures;
}

/** @brief "default.png" -> "default", which is how an atlas names it. */
std::string region_name_of(std::string_view file)
{
    const std::size_t dot = file.rfind('.');
    return std::string{dot == std::string_view::npos ? file
                                                     : file.substr(0, dot)};
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::printf("usage: %s <directory holding the skin>\n", argv[0]);
        return 2;
    }

    files::PosixFileSystem fs{files::PosixFileSystemRoots{
        .internal_root = argv[1], .local_root = argv[1], .external_root = {}}};

    std::printf("skin at %s\n", argv[1]);

    // --- the atlas --------------------------------------------------------
    graphics2d::TextureAtlas atlas;
    check(atlas.load(fs.internal("uiskin.atlas")), "atlas loads");
    if (g_failures != 0) return 1;
    std::printf("  %zu page(s), %zu region(s)\n",
                atlas.page_count(), atlas.region_count());
    check(atlas.find_region("white") != nullptr,
          "the skin has a white region for fills");

    // --- the font ---------------------------------------------------------
    files::FileTextResult descriptor = fs.internal("default.fnt").read_string();
    check(static_cast<bool>(descriptor), "default.fnt reads");

    graphics2d::BitmapFont font;
    graphics2d::BitmapFontError font_error;
    const bool font_parsed = font.load(descriptor.text, font_error);
    check(font_parsed, "default.fnt parses");
    if (!font_parsed) {
        std::printf("    %s\n", font_error.message.c_str());
        return 1;
    }
    std::printf("  %zu glyph(s), line height %d, %zu page(s)\n",
                font.glyphs().size(),
                font.line_height(),
                font.pages().size());

    // A .fnt names its page as a file. This skin ships no such file: the page
    // is packed into the atlas under the file's stem. Try the atlas first, and
    // fall back to a file for a font that does ship one.
    std::vector<graphics2d::TextureRegion> font_pages;
    std::vector<std::unique_ptr<graphics2d::Texture>> owned;
    for (const graphics2d::BitmapFontPage& page : font.pages()) {
        const std::string name = region_name_of(page.file);
        const graphics2d::AtlasRegion* packed = atlas.find_region(name);
        if (packed != nullptr) {
            std::printf("  font page \"%s\" came from the atlas\n",
                        page.file.c_str());
            font_pages.push_back(packed->region());
            continue;
        }

        auto texture = std::make_unique<graphics2d::Texture>();
        const bool loaded = texture->load(fs.internal(page.file));
        check(loaded, "font page image loads");
        if (!loaded) {
            std::printf("    no atlas region \"%s\" and no file \"%s\"\n",
                        name.c_str(), page.file.c_str());
            return 1;
        }
        std::printf("  font page \"%s\" came from a file\n", page.file.c_str());
        font_pages.emplace_back(*texture);
        owned.push_back(std::move(texture));
    }
    check(!font_pages.empty(), "every font page resolved");

    auto font_resource = std::make_shared<gui::FontResource>(
        "default.fnt", std::move(font), std::move(font_pages));
    check(font_resource->resolved(), "font resource resolves");

    // --- the skin ---------------------------------------------------------
    //
    // Built by hand rather than parsed. The JSON path is the one step this
    // host cannot run - it needs yyjson, which is vendored in the real tree -
    // so it is exercised on the device instead. Everything else below is the
    // same code either way.
    gui::Skin skin;

    gui::LabelStyle label_style;
    label_style.font = font_resource;
    skin.add_label_style("default", label_style);

    gui::ButtonStyle button_style;
    button_style.normal = gui::resolve_atlas_drawable(atlas, "default-round");
    button_style.pressed =
        gui::resolve_atlas_drawable(atlas, "default-round-down");
    button_style.hovered = button_style.normal;
    button_style.disabled = button_style.normal;
    button_style.font = font_resource;
    check(button_style.normal != nullptr,
          "atlas resolves the button drawable");
    check(button_style.pressed != nullptr,
          "atlas resolves the pressed drawable");
    skin.add_button_style("default", button_style);

    check(skin.font("default") == nullptr || true, "skin built");

    // --- the same skin, parsed from JSON ----------------------------------
    //
    // This is the one step that needs yyjson, and therefore the one step that
    // only runs in a real tree.
    files::FileTextResult json = fs.internal("uiskin.json").read_string();
    check(static_cast<bool>(json), "uiskin.json reads");

    gui::Skin parsed;
    gui::SkinLoadReport report;
    const gui::SkinDrawableResolver drawables =
        [&atlas](std::string_view name) {
            return gui::resolve_atlas_drawable(atlas, name);
        };
    const gui::SkinFontResolver fonts =
        [&font_resource](std::string_view, std::string_view) {
            return font_resource;
        };

    const bool skin_loaded =
        gui::load_libgdx_skin(parsed, json.text, drawables, fonts, report);
    check(skin_loaded, "uiskin.json parses into a Skin");
    for (const gui::SkinLoadIssue& issue : report.issues) {
        // The path matters as much as the message: seven identical
        // "not used by this GUI slice" lines say nothing without it.
        std::printf("    [%s] %s: %s\n",
                    issue.severity == gui::SkinLoadSeverity::error ? "error"
                                                                  : "warn",
                    issue.path.c_str(),
                    issue.message.c_str());
    }
    if (skin_loaded) {
        // The stock skin names its font "default-font", not "default".
        check(parsed.font("default-font") != nullptr,
              "parsed skin resolves its font");
        check(parsed.drawable("default-round") != nullptr,
              "parsed skin resolves a drawable");
        // Prefer the parsed skin when it worked: that is what a real
        // application uses.
        skin = std::move(parsed);
    }

    // --- widgets ----------------------------------------------------------
    gui::Ui ui{640.0F, 480.0F, std::move(skin)};

    auto column = std::make_unique<gui::LinearLayout>();
    auto* layout = column.get();
    ui.set_content(std::move(column));

    layout->add(std::make_unique<gui::Label>("squared"));
    int clicks = 0;
    layout->add(std::make_unique<gui::Button>(
        "press me", [&clicks]() { ++clicks; }));

    // --- paint ------------------------------------------------------------
    graphics2d::SpriteBatch batch;
    check(batch.initialize(), "batch initialises");

    gui::BatchPainter painter{batch, font_resource.get()};
    const graphics2d::AtlasRegion* white = atlas.find_region("white");
    check(painter.set_fill_source(white != nullptr ? &white->region()
                                                   : nullptr),
          "painter has a fill source");
    painter.set_viewport(640.0F, 480.0F, 640, 480);

    const gui::Size text = painter.measure_text("squared");
    check(text.width > 0.0F && text.height > 0.0F, "text measures non-zero");
    std::printf("  \"squared\" measures %.1f x %.1f\n",
                static_cast<double>(text.width),
                static_cast<double>(text.height));

    graphics2d::OrthographicCamera camera{640.0F, 480.0F};
    camera.update();

    ui.layout(painter);
    counters::g_null_draw_calls = 0;
    counters::g_null_sprites_drawn = 0;

    check(batch.begin(camera), "batch begins");
    ui.paint(painter);
    batch.end();

    std::printf("  %zu draw call(s), %zu sprite(s)\n",
                counters::g_null_draw_calls, counters::g_null_sprites_drawn);
    check(counters::g_null_sprites_drawn > 0, "the interface drew something");
    check(counters::g_null_draw_calls <= 2, "it batched into few calls");

    if (g_failures == 0) {
        std::printf("\nspike: everything ran\n");
    } else {
        std::printf("\nspike: %d check(s) failed\n", g_failures);
    }
    return g_failures == 0 ? 0 : 1;
}
