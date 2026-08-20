#include <squared/graphics/context.hpp>
#include <squared/graphics2d/bitmap_font.hpp>
#include <squared/graphics2d/orthographic_camera.hpp>
#include <squared/graphics2d/sprite_batch.hpp>
#include <squared/graphics2d/texture.hpp>
#include <squared/graphics2d/texture_atlas.hpp>
#include <squared/graphics2d/texture_region.hpp>

#include <cmath>
#include <concepts>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <type_traits>

namespace {

void require(bool condition, const char* message)
{
    if (condition) return;
    std::cerr << "graphics2d portable test failed: " << message << '\n';
    std::exit(1);
}

bool regenerate_texture(
    void*,
    squared::graphics2d::TextureRecoveryTarget&
) noexcept
{
    return false;
}

}  // namespace

static_assert(!std::is_copy_constructible_v<squared::graphics::Context>);
static_assert(!std::is_copy_constructible_v<squared::graphics2d::Texture>);
static_assert(std::is_move_constructible_v<squared::graphics2d::Texture>);
static_assert(!std::is_copy_constructible_v<squared::graphics2d::SpriteBatch>);
static_assert(requires(squared::graphics2d::Texture& texture) {
    texture.release();
    { texture.restore(false) } -> std::same_as<bool>;
    { texture.restorable() } -> std::same_as<bool>;
    { texture.recovery_policy() } ->
        std::same_as<squared::graphics2d::TextureRecoveryPolicy>;
    { texture.retained_recovery_bytes() } -> std::same_as<std::size_t>;
});
static_assert(requires(
    squared::graphics2d::Texture& texture,
    const std::uint8_t* pixels
) {
    { texture.create_rgba(
        1,
        1,
        pixels,
        squared::graphics2d::TextureRecoveryOptions::discard()
    ) } -> std::same_as<bool>;
});
static_assert(requires(squared::graphics2d::TextureAtlas& atlas) {
    atlas.release();
    { atlas.restore(false) } -> std::same_as<bool>;
});
static_assert(requires(squared::graphics2d::SpriteBatch& batch) {
    batch.release();
    { batch.restore(false) } -> std::same_as<bool>;
});

int main()
{
    using squared::graphics2d::TextureRecoveryOptions;
    using squared::graphics2d::TextureRecoveryPolicy;
    require(
        TextureRecoveryOptions::reload_from_asset().policy ==
            TextureRecoveryPolicy::ReloadFromAsset,
        "asset reload policy is portable"
    );
    require(
        TextureRecoveryOptions::retain_pixels().policy ==
            TextureRecoveryPolicy::RetainPixels,
        "pixel retention policy is portable"
    );
    const auto regenerate = TextureRecoveryOptions::regenerate(
        &regenerate_texture,
        nullptr
    );
    require(
        regenerate.policy == TextureRecoveryPolicy::Regenerate &&
            regenerate.callback == &regenerate_texture,
        "callback regeneration policy is portable"
    );
    require(
        TextureRecoveryOptions::discard().policy ==
            TextureRecoveryPolicy::Discard,
        "discard policy is portable"
    );

    const squared::graphics2d::TextureRegion region(100, 80);
    const auto subsection = region.subregion(10, 20, 30, 40);
    require(subsection.width() == 30 && subsection.height() == 40,
            "logical texture subregion preserves dimensions");
    require(std::abs(subsection.u1() - 0.1F) < 0.0001F &&
                std::abs(subsection.v1() - 0.25F) < 0.0001F &&
                std::abs(subsection.u2() - 0.4F) < 0.0001F &&
                std::abs(subsection.v2() - 0.75F) < 0.0001F,
            "logical texture subregion derives normalized coordinates");
    const squared::graphics2d::TextureRegion rotated(100, 80, true);
    const auto rotated_subsection = rotated.subregion(10, 20, 30, 40);
    require(rotated_subsection.rotated_clockwise(),
            "subregion preserves clockwise atlas rotation");
    require(std::abs(rotated_subsection.u1() - 0.25F) < 0.0001F &&
                std::abs(rotated_subsection.v1() - 0.1F) < 0.0001F &&
                std::abs(rotated_subsection.u2() - 0.75F) < 0.0001F &&
                std::abs(rotated_subsection.v2() - 0.4F) < 0.0001F,
            "rotated subregion maps logical axes into atlas storage");
    require(region.subregion(90, 70, 20, 20).width() == 0,
            "out-of-bounds subregion is empty");

    squared::graphics2d::OrthographicCamera camera(320.0F, 180.0F);
    require(
        std::abs(camera.position().x - 160.0F) < 0.0001F,
        "camera starts horizontally centered"
    );
    require(
        std::abs(camera.position().y - 90.0F) < 0.0001F,
        "camera starts vertically centered"
    );
    camera.set_position(40.0F, 50.0F);
    camera.set_zoom(2.0F);
    camera.update();
    require(
        std::abs(camera.position().x - 40.0F) < 0.0001F,
        "camera pan remains portable"
    );
    require(
        std::abs(camera.zoom() - 2.0F) < 0.0001F,
        "camera zoom remains portable"
    );

    constexpr std::string_view font_descriptor = R"FONT(info face="Test Face" size=20 bold=0 italic=0 charset="" unicode=1
common lineHeight=20 base=15 scaleW=64 scaleH=64 pages=1 packed=0
page id=0 file="fonts/test.png"
chars count=4
char id=32 x=0 y=0 width=0 height=0 xoffset=0 yoffset=0 xadvance=5 page=0 chnl=15
char id=63 x=0 y=0 width=7 height=10 xoffset=1 yoffset=2 xadvance=9 page=0 chnl=15
char id=65 x=8 y=0 width=8 height=10 xoffset=1 yoffset=2 xadvance=10 page=0 chnl=15
char id=86 x=16 y=0 width=8 height=10 xoffset=0 yoffset=2 xadvance=10 page=0 chnl=15
char id=937 x=24 y=0 width=10 height=10 xoffset=0 yoffset=2 xadvance=10 page=0 chnl=15
kernings count=1
kerning first=65 second=86 amount=-2
)FONT";

    squared::graphics2d::BitmapFont font;
    squared::graphics2d::BitmapFontError font_error;
    require(font.load(font_descriptor, font_error),
            "text BMFont descriptor loads");
    require(font.valid() && font.info().face == "Test Face",
            "font metadata is retained");
    require(font.pages().size() == 1 &&
                font.pages().front().file == "fonts/test.png",
            "font keeps a safe relative texture-page reference");
    require(font.glyph(U'\u03A9') != nullptr && font.kerning(U'A', U'V') == -2,
            "Unicode glyphs and kerning pairs are indexed");

    squared::graphics2d::GlyphLayout layout;
    squared::graphics2d::GlyphLayoutOptions layout_options;
    layout_options.target_width = 30.0F;
    layout_options.alignment = squared::graphics2d::GlyphAlignment::center;
    require(layout.set_text(font, "AV\n\xCE\xA9", layout_options, font_error),
            "UTF-8 glyph layout succeeds");
    require(layout.lines().size() == 2 && layout.glyphs().size() == 3,
            "explicit newlines create distinct glyph lines");
    require(std::abs(layout.lines()[0].width - 18.0F) < 0.0001F &&
                std::abs(layout.lines()[0].x_offset - 6.0F) < 0.0001F,
            "kerning affects width before center alignment");
    require(std::abs(layout.glyphs()[1].x - 14.0F) < 0.0001F &&
                std::abs(layout.glyphs()[2].x - 10.0F) < 0.0001F,
            "aligned placements retain kerning and per-line offsets");
    require(std::abs(layout.width() - 30.0F) < 0.0001F &&
                std::abs(layout.height() - 40.0F) < 0.0001F,
            "layout exposes target width and multiline height");

    const auto prior_glyph_count = layout.glyphs().size();
    require(!layout.set_text(font, std::string_view{"\xFF", 1}, font_error) &&
                font_error.code ==
                    squared::graphics2d::BitmapFontErrorCode::invalid_utf8,
            "strict layout rejects malformed UTF-8");
    require(layout.glyphs().size() == prior_glyph_count,
            "failed layout preserves the previous glyph run");
    layout_options.reject_invalid_utf8 = false;
    layout_options.target_width = 0.0F;
    require(layout.set_text(font, std::string_view{"\xFF", 1}, layout_options,
                            font_error) &&
                layout.glyphs().size() == 1 &&
                layout.glyphs().front().codepoint == U'?',
            "permissive layout substitutes malformed UTF-8");

    constexpr std::string_view unsafe_descriptor = R"FONT(info face="Bad" size=20 bold=0 italic=0 unicode=1
common lineHeight=20 base=15 scaleW=64 scaleH=64 pages=1
page id=0 file="../escape.png"
chars count=1
char id=65 x=0 y=0 width=8 height=10 xoffset=0 yoffset=0 xadvance=8 page=0
)FONT";
    require(!font.load(unsafe_descriptor, font_error) &&
                font_error.code ==
                    squared::graphics2d::BitmapFontErrorCode::malformed,
            "unsafe texture-page paths are rejected");
    require(font.valid() && font.glyph(U'\u03A9') != nullptr,
            "failed BMFont load preserves the previous resource");

    std::cout << "Squared Graphics2D portable boundary: OK\n";
    return 0;
}
