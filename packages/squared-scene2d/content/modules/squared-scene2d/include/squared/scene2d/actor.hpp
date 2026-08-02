#pragma once

namespace squared::scene2d {

class Group;

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

    /** @brief Advance this actor by one frame. */
    virtual void act(double delta_seconds);

    /**
     * @brief Return the deepest eligible actor at a local coordinate.
     * @param local_x Horizontal coordinate local to this actor.
     * @param local_y Vertical coordinate local to this actor.
     * @param require_touchable Ignore actors whose touchable flag is false.
     */
    [[nodiscard]] virtual Actor* hit(
        float local_x,
        float local_y,
        bool require_touchable = true
    ) noexcept;

    void set_bounds(float x, float y, float width, float height) noexcept;
    void set_position(float x, float y) noexcept;
    void set_size(float width, float height) noexcept;

    [[nodiscard]] float x() const noexcept { return x_; }
    [[nodiscard]] float y() const noexcept { return y_; }
    [[nodiscard]] float width() const noexcept { return width_; }
    [[nodiscard]] float height() const noexcept { return height_; }

    void set_visible(bool visible) noexcept { visible_ = visible; }
    [[nodiscard]] bool visible() const noexcept { return visible_; }

    void set_touchable(bool touchable) noexcept { touchable_ = touchable; }
    [[nodiscard]] bool touchable() const noexcept { return touchable_; }

    [[nodiscard]] Group* parent() noexcept { return parent_; }
    [[nodiscard]] const Group* parent() const noexcept { return parent_; }

protected:
    [[nodiscard]] bool contains(float local_x, float local_y) const noexcept;

private:
    friend class Group;
    void set_parent(Group* parent) noexcept { parent_ = parent; }

    Group* parent_{nullptr};
    float x_{0.0F};
    float y_{0.0F};
    float width_{0.0F};
    float height_{0.0F};
    bool visible_{true};
    bool touchable_{true};
};

} // namespace squared::scene2d
