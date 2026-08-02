#include <squared/math/matrix4.hpp>
#include <squared/math/vector2.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {

void require(bool condition, const char* message)
{
    if (condition) return;
    std::cerr << "math test failed: " << message << '\n';
    std::exit(1);
}

bool near(float left, float right)
{
    return std::abs(left - right) < 0.000001F;
}

}  // namespace

int main()
{
    const squared::math::Vector2 vector{3.0F, -2.0F};
    require(near(vector.x, 3.0F), "Vector2 stores x");
    require(near(vector.y, -2.0F), "Vector2 stores y");

    const squared::math::Matrix4 identity;
    const float* identity_data = identity.data();
    for (int index = 0; index < 16; ++index) {
        const float expected = index % 5 == 0 ? 1.0F : 0.0F;
        require(
            near(identity_data[index], expected),
            "Matrix4 defaults to identity"
        );
    }

    const auto projection = squared::math::Matrix4::orthographic(
        0.0F,
        200.0F,
        100.0F,
        0.0F
    );
    const float* values = projection.data();
    require(near(values[0], 0.01F), "orthographic x scale");
    require(near(values[5], -0.02F), "orthographic y scale");
    require(near(values[10], -1.0F), "orthographic depth scale");
    require(near(values[12], -1.0F), "orthographic x translation");
    require(near(values[13], 1.0F), "orthographic y translation");
    require(near(values[15], 1.0F), "orthographic homogeneous value");

    const auto degenerate = squared::math::Matrix4::orthographic(
        1.0F,
        1.0F,
        0.0F,
        100.0F
    );
    require(
        near(degenerate.data()[0], 1.0F) &&
            near(degenerate.data()[5], 1.0F) &&
            near(degenerate.data()[10], 1.0F) &&
            near(degenerate.data()[15], 1.0F),
        "degenerate projection returns identity"
    );

    std::cout << "Squared math primitives: OK\n";
    return 0;
}
