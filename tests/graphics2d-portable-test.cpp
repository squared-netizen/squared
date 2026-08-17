#include <squared/graphics/context.hpp>
#include <squared/graphics2d/orthographic_camera.hpp>
#include <squared/graphics2d/sprite_batch.hpp>
#include <squared/graphics2d/texture.hpp>
#include <squared/graphics2d/texture_atlas.hpp>
#include <squared/graphics2d/texture_region.hpp>

#include <cmath>
#include <concepts>
#include <cstdlib>
#include <iostream>
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

    std::cout << "Squared Graphics2D portable boundary: OK\n";
    return 0;
}
