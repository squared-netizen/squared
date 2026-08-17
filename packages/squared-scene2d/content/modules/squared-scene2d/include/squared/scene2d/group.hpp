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

    /**
     * @brief Transfer ownership of one unparented actor into this group.
     * @param actor The actor to adopt; must not already have a parent.
     * @return Reference to the adopted actor.
     */
    Actor& add_actor(std::unique_ptr<Actor> actor);

    /**
     * @brief Remove an immediate child and return its ownership.
     * @param actor An immediate child actor to detach.
     * @return The detached actor, or null when it is not an immediate child.
     */
    [[nodiscard]] std::unique_ptr<Actor> remove_actor(Actor& actor) noexcept;

    /** @brief Destroy all immediate children. */
    void clear() noexcept;

    /**
     * @brief Count immediate children.
     * @return Number of immediate children owned by this group.
     */
    [[nodiscard]] std::size_t child_count() const noexcept;

    /**
     * @brief Return the child at an insertion-order index.
     * @param index Zero-based child index.
     * @return The child, or null when the index is out of range.
     */
    [[nodiscard]] Actor* child_at(std::size_t index) noexcept;

    /**
     * @brief Return the child at an insertion-order index, read-only.
     * @param index Zero-based child index.
     * @return The child, or null when the index is out of range.
     */
    [[nodiscard]] const Actor* child_at(std::size_t index) const noexcept;

private:
    std::vector<std::unique_ptr<Actor>> children_;
};

} // namespace squared::scene2d
