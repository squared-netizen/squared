#pragma once

#include <squared/scene2d/actor.hpp>

#include <cstddef>
#include <memory>
#include <vector>

namespace squared::scene2d {

/** @brief Actor that owns children in deterministic insertion order. */
class Group : public Actor {
public:
    Group() = default;
    ~Group() override;

    void act(double delta_seconds) override;
    [[nodiscard]] Actor* hit(
        float local_x,
        float local_y,
        bool require_touchable = true
    ) noexcept override;

    /** @brief Transfer ownership of one unparented actor into this group. */
    Actor& add_actor(std::unique_ptr<Actor> actor);

    /** @brief Remove an immediate child and return its ownership. */
    [[nodiscard]] std::unique_ptr<Actor> remove_actor(Actor& actor) noexcept;

    /** @brief Destroy all immediate children. */
    void clear() noexcept;

    [[nodiscard]] std::size_t child_count() const noexcept;
    [[nodiscard]] Actor* child_at(std::size_t index) noexcept;
    [[nodiscard]] const Actor* child_at(std::size_t index) const noexcept;

private:
    std::vector<std::unique_ptr<Actor>> children_;
};

} // namespace squared::scene2d
