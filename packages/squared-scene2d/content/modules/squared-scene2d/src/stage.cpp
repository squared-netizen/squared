#include <squared/scene2d/stage.hpp>

namespace squared::scene2d {

Stage::Stage(float width, float height) noexcept
{
    resize(width, height);
}

void Stage::resize(float width, float height) noexcept
{
    root_.set_bounds(0.0F, 0.0F, width, height);
}

void Stage::act(double delta_seconds)
{
    root_.act(delta_seconds);
}

Actor& Stage::add_actor(std::unique_ptr<Actor> actor)
{
    return root_.add_actor(std::move(actor));
}

Actor* Stage::hit(
    float stage_x,
    float stage_y,
    bool require_touchable
) noexcept
{
    return root_.hit(stage_x, stage_y, require_touchable);
}

} // namespace squared::scene2d
