#pragma once

#include <squared/scene2d/actor_interface_id.hpp>
#include <squared/scene2d/input_listener.hpp>
#include <squared/scene2d/input_listener_id.hpp>

#include <vector>

namespace sq::scene2d {

class Group;
class InputEvent;

/**
 * @brief Base node with parent-relative bounds and frame traversal.
 *
 * Coordinates supplied to hit() are local to the actor. Position is expressed
 * in the parent coordinate system. The first Scene2D slice intentionally has
 * translation-only hierarchy semantics; transforms and rendering arrive in a
 * later compatible layer.
 */
class Actor {
public:
    Actor() = default;
    virtual ~Actor() = default;

    Actor(const Actor&) = delete;
    Actor& operator=(const Actor&) = delete;
    Actor(Actor&&) = delete;
    Actor& operator=(Actor&&) = delete;

    /** @brief Advance this actor by one frame.
     *  @param delta_seconds Elapsed domain time in seconds.
     */
    virtual void act(double delta_seconds);

    /**
     * @brief Return the deepest eligible actor at a local coordinate.
     * @param local_x Horizontal coordinate local to this actor.
     * @param local_y Vertical coordinate local to this actor.
     * @param require_touchable Ignore actors whose touchable flag is false.
     * @return The actor at the coordinate, or null when nothing qualifies.
     */
    [[nodiscard]] virtual Actor* hit(
        float local_x,
        float local_y,
        bool require_touchable = true
    ) noexcept;

    /**
     * @brief Set position and size in parent-local logical units.
     * @param x Horizontal position.
     * @param y Vertical position.
     * @param width New width; non-negative.
     * @param height New height; non-negative.
     */
    void set_bounds(float x, float y, float width, float height) noexcept;

    /**
     * @brief Set position in parent-local logical units.
     * @param x Horizontal position.
     * @param y Vertical position.
     */
    void set_position(float x, float y) noexcept;

    /**
     * @brief Set the extent in logical units.
     * @param width New width; non-negative.
     * @param height New height; non-negative.
     */
    void set_size(float width, float height) noexcept;

    /**
     * @brief Read the horizontal position.
     * @return Horizontal position in parent-local units.
     */
    [[nodiscard]] float x() const noexcept { return x_; }

    /**
     * @brief Read the vertical position.
     * @return Vertical position in parent-local units.
     */
    [[nodiscard]] float y() const noexcept { return y_; }

    /**
     * @brief Read the width.
     * @return Width in logical units.
     */
    [[nodiscard]] float width() const noexcept { return width_; }

    /**
     * @brief Read the height.
     * @return Height in logical units.
     */
    [[nodiscard]] float height() const noexcept { return height_; }

    /**
     * @brief Control whether the actor participates in frames and hit tests.
     * @param visible `false` hides the actor and excludes it from hit tests.
     */
    void set_visible(bool visible) noexcept { visible_ = visible; }

    /**
     * @brief Read the visibility state.
     * @return true while the actor participates in frames and hit tests.
     */
    [[nodiscard]] bool visible() const noexcept { return visible_; }

    /**
     * @brief Control whether the actor can receive pointer hit tests.
     * @param touchable `false` skips the actor during hit testing.
     */
    void set_touchable(bool touchable) noexcept { touchable_ = touchable; }

    /**
     * @brief Read the touchable state.
     * @return true while the actor participates in hit testing.
     */
    [[nodiscard]] bool touchable() const noexcept { return touchable_; }

    /**
     * @brief Access the owning group.
     * @return Owning group, or null for a root-owned actor.
     */
    [[nodiscard]] Group* parent() noexcept { return parent_; }

    /**
     * @brief Access the owning group, read-only.
     * @return Owning group, or null for a root-owned actor.
     */
    [[nodiscard]] const Group* parent() const noexcept { return parent_; }

    /**
     * @brief Add a regular or capture listener owned by this actor.
     * @param listener Callback invoked during input dispatch.
     * @param capture `true` registers the listener for the capture phase.
     * @return A stable actor-local identifier usable with
     * remove_input_listener.
     */
    InputListenerId add_input_listener(
        InputListener listener,
        bool capture = false
    );

    /**
     * @brief Remove one listener by its stable actor-local identifier.
     * @param id Identifier returned by add_input_listener.
     * @return `true` when the identifier matched a live listener.
     */
    [[nodiscard]] bool remove_input_listener(InputListenerId id) noexcept;

    /**
     * @brief Recover a derived interface published by this actor.
     * @param id Identifier of the requested interface.
     * @return Pointer to the requested interface, or null when this actor does
     * not publish it. The base implementation publishes nothing.
     * @note Prefer actor_cast(), which supplies the identifier and restores the
     * pointer type. Overrides must answer their own identifier and otherwise
     * delegate to their direct base, so an interface published anywhere in the
     * chain stays reachable.
     */
    [[nodiscard]] virtual void* actor_interface(ActorInterfaceId id) noexcept
    {
        static_cast<void>(id);
        return nullptr;
    }

    /**
     * @brief Recover a derived interface published by this actor, read-only.
     * @param id Identifier of the requested interface.
     * @return Pointer to the requested interface, or null when this actor does
     * not publish it.
     */
    [[nodiscard]] virtual const void* actor_interface(
        ActorInterfaceId id
    ) const noexcept
    {
        static_cast<void>(id);
        return nullptr;
    }

protected:
    /**
     * @brief Test whether a local coordinate falls inside this actor.
     * @param local_x Horizontal coordinate local to this actor.
     * @param local_y Vertical coordinate local to this actor.
     * @return true when the point lies within the actor's rectangle.
     */
    [[nodiscard]] bool contains(float local_x, float local_y) const noexcept;

    /**
     * @brief Handle target/bubble input before regular external listeners.
     * @param event The event currently in the target or bubble phase.
     */
    virtual void input_event(InputEvent& event);

private:
    friend class Group;
    friend class Stage;
    void set_parent(Group* parent) noexcept { parent_ = parent; }
    void notify_input(InputEvent& event, bool capture_only);

    struct ListenerEntry final {
        InputListenerId id{0};
        bool capture{false};
        InputListener listener;
    };

    Group* parent_{nullptr};
    float x_{0.0F};
    float y_{0.0F};
    float width_{0.0F};
    float height_{0.0F};
    bool visible_{true};
    bool touchable_{true};
    InputListenerId next_listener_id_{1};
    std::vector<ListenerEntry> listeners_;
};

/**
 * @brief Downcast an actor to a derived interface without RTTI.
 * @tparam Interface Type publishing a static `actor_interface_id` constant and
 * overriding Actor::actor_interface().
 * @param actor Actor to inspect; null is accepted and yields null.
 * @return Pointer to the interface, or null when the actor does not publish it.
 * @note One virtual call. This replaces dynamic_cast on every Scene2D and GUI
 * traversal, so the framework builds with `-fno-rtti`.
 */
template <typename Interface>
[[nodiscard]] Interface* actor_cast(Actor* actor) noexcept
{
    if (actor == nullptr) return nullptr;
    return static_cast<Interface*>(
        actor->actor_interface(Interface::actor_interface_id)
    );
}

/**
 * @brief Downcast a read-only actor to a derived interface without RTTI.
 * @tparam Interface Type publishing a static `actor_interface_id` constant.
 * @param actor Actor to inspect; null is accepted and yields null.
 * @return Pointer to the interface, or null when the actor does not publish it.
 */
template <typename Interface>
[[nodiscard]] const Interface* actor_cast(const Actor* actor) noexcept
{
    if (actor == nullptr) return nullptr;
    return static_cast<const Interface*>(
        actor->actor_interface(Interface::actor_interface_id)
    );
}

} // namespace sq::scene2d
