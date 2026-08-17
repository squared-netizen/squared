#pragma once

#include <squared/scene2d/group.hpp>

#include <memory>

namespace squared::scene2d {

/** @brief Root owner for a translation-only two-dimensional actor hierarchy. */
class Stage {
public:
    /**
     * @brief Construct a stage with a logical viewport.
     * @param width Initial logical width.
     * @param height Initial logical height.
     */
    Stage(float width, float height) noexcept;

    /**
     * @brief Update the logical viewport dimensions.
     * @param width New logical width.
     * @param height New logical height.
     */
    void resize(float width, float height) noexcept;

    /**
     * @brief Advance the entire hierarchy by one frame.
     * @param delta_seconds Elapsed domain time in seconds.
     */
    void act(double delta_seconds);

    /**
     * @brief Adopt an actor into the root group.
     * @param actor The unparented actor to own.
     * @return Reference to the adopted actor.
     */
    [[nodiscard]] Actor& add_actor(std::unique_ptr<Actor> actor);

    /**
     * @brief Hit-test the hierarchy in stage coordinates.
     * @param stage_x Horizontal position in stage logical units.
     * @param stage_y Vertical position in stage logical units.
     * @param require_touchable Ignore actors whose touchable flag is false.
     * @return The deepest qualifying actor, or null.
     */
    [[nodiscard]] Actor* hit(
        float stage_x,
        float stage_y,
        bool require_touchable = true
    ) noexcept;

    /**
     * @brief Route input through capture, target, and bubble phases.
     *
     * When target is null, pointer events use stage hit testing. Other event
     * types require an explicit target.
     *
     * @param event The event to dispatch.
     * @param target Explicit dispatch target, or null for pointer hit testing.
     * @return `true` when the event was handled.
     */
    [[nodiscard]] bool dispatch_input(
        InputEvent& event,
        Actor* target = nullptr
    );

    /**
     * @brief Access the root group.
     * @return Reference to the root group owning every top-level actor.
     */
    [[nodiscard]] Group& root() noexcept { return root_; }

    /**
     * @brief Access the root group, read-only.
     * @return Reference to the root group owning every top-level actor.
     */
    [[nodiscard]] const Group& root() const noexcept { return root_; }

private:
    Group root_;
};

} // namespace squared::scene2d
