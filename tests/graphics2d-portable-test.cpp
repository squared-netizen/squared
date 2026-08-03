#include <squared/graphics/context.hpp>
#include <squared/graphics2d/orthographic_camera.hpp>
#include <squared/graphics2d/sprite_batch.hpp>
#include <squared/graphics2d/texture.hpp>

#include <cmath>
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

}  // namespace

static_assert(!std::is_copy_constructible_v<squared::graphics::Context>);
static_assert(!std::is_copy_constructible_v<squared::graphics2d::Texture>);
static_assert(std::is_move_constructible_v<squared::graphics2d::Texture>);
static_assert(!std::is_copy_constructible_v<squared::graphics2d::SpriteBatch>);

int main()
{
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
