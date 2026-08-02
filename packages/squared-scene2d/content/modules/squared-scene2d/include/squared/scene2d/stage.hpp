#pragma once

#include <squared/scene2d/group.hpp>

#include <memory>

namespace squared::scene2d {

/** @brief Root owner for a translation-only two-dimensional actor hierarchy. */
class Stage {
public:
    Stage(float width, float height) noexcept;

    void resize(float width, float height) noexcept;
    void act(double delta_seconds);

    [[nodiscard]] Actor& add_actor(std::unique_ptr<Actor> actor);
    [[nodiscard]] Actor* hit(
        float stage_x,
        float stage_y,
        bool require_touchable = true
    ) noexcept;

    [[nodiscard]] Group& root() noexcept { return root_; }
    [[nodiscard]] const Group& root() const noexcept { return root_; }

private:
    Group root_;
};

} // namespace squared::scene2d
