#include <squared/graphics/color.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {

void require(bool condition, const char* message)
{
    if (condition) return;
    std::cerr << "graphics test failed: " << message << '\n';
    std::exit(1);
}

bool near(float left, float right)
{
    return std::abs(left - right) < 0.000001F;
}

}  // namespace

int main()
{
    const auto white = squared::graphics::Color::white();
    require(
        near(white.red, 1.0F) &&
            near(white.green, 1.0F) &&
            near(white.blue, 1.0F) &&
            near(white.alpha, 1.0F),
        "white is opaque"
    );

    const auto transparent = squared::graphics::Color::transparent();
    require(
        near(transparent.red, 0.0F) &&
            near(transparent.green, 0.0F) &&
            near(transparent.blue, 0.0F) &&
            near(transparent.alpha, 0.0F),
        "transparent is clear black"
    );

    const auto converted =
        squared::graphics::Color::from_rgba8(255, 128, 0, 64);
    require(near(converted.red, 1.0F), "RGBA8 red conversion");
    require(near(converted.green, 128.0F / 255.0F), "RGBA8 green conversion");
    require(near(converted.blue, 0.0F), "RGBA8 blue conversion");
    require(near(converted.alpha, 64.0F / 255.0F), "RGBA8 alpha conversion");

    const auto clamped = squared::graphics::Color{
        -1.0F,
        0.25F,
        2.0F,
        1.5F
    }.clamped();
    require(near(clamped.red, 0.0F), "negative component clamps");
    require(near(clamped.green, 0.25F), "safe component remains");
    require(near(clamped.blue, 1.0F), "high component clamps");
    require(near(clamped.alpha, 1.0F), "high alpha clamps");

    std::cout << "Squared graphics color values: OK\n";
    return 0;
}
