#include <squared/scene2d/actor.hpp>

#include <algorithm>

namespace squared::scene2d {

void Actor::act(double) {}

Actor* Actor::hit(
    float local_x,
    float local_y,
    bool require_touchable
) noexcept
{
    if (!visible_ || (require_touchable && !touchable_)) return nullptr;
    return contains(local_x, local_y) ? this : nullptr;
}

void Actor::set_bounds(
    float x,
    float y,
    float width,
    float height
) noexcept
{
    set_position(x, y);
    set_size(width, height);
}

void Actor::set_position(float x, float y) noexcept
{
    x_ = x;
    y_ = y;
}

void Actor::set_size(float width, float height) noexcept
{
    width_ = std::max(0.0F, width);
    height_ = std::max(0.0F, height);
}

bool Actor::contains(float local_x, float local_y) const noexcept
{
    return local_x >= 0.0F && local_y >= 0.0F &&
        local_x < width_ && local_y < height_;
}

} // namespace squared::scene2d
