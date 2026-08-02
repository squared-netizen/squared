#include <squared/scene2d/group.hpp>

#include <algorithm>
#include <stdexcept>

namespace squared::scene2d {

Group::~Group()
{
    clear();
}

void Group::act(double delta_seconds)
{
    Actor::act(delta_seconds);
    for (const auto& child : children_) {
        child->act(delta_seconds);
    }
}

Actor* Group::hit(
    float local_x,
    float local_y,
    bool require_touchable
) noexcept
{
    if (!visible() || !contains(local_x, local_y)) return nullptr;

    for (auto iterator = children_.rbegin();
         iterator != children_.rend();
         ++iterator) {
        Actor& child = **iterator;
        Actor* result = child.hit(
            local_x - child.x(),
            local_y - child.y(),
            require_touchable
        );
        if (result) return result;
    }
    return Actor::hit(local_x, local_y, require_touchable);
}

Actor& Group::add_actor(std::unique_ptr<Actor> actor)
{
    if (!actor) {
        throw std::invalid_argument("Scene2D actor must not be null");
    }
    if (actor->parent()) {
        throw std::invalid_argument("Scene2D actor already has a parent");
    }
    Actor& reference = *actor;
    actor->set_parent(this);
    try {
        children_.push_back(std::move(actor));
    } catch (...) {
        reference.set_parent(nullptr);
        throw;
    }
    return reference;
}

std::unique_ptr<Actor> Group::remove_actor(Actor& actor) noexcept
{
    const auto iterator = std::find_if(
        children_.begin(),
        children_.end(),
        [&actor](const auto& candidate) {
            return candidate.get() == &actor;
        }
    );
    if (iterator == children_.end()) return {};

    std::unique_ptr<Actor> removed = std::move(*iterator);
    children_.erase(iterator);
    removed->set_parent(nullptr);
    return removed;
}

void Group::clear() noexcept
{
    for (const auto& child : children_) {
        child->set_parent(nullptr);
    }
    children_.clear();
}

std::size_t Group::child_count() const noexcept
{
    return children_.size();
}

Actor* Group::child_at(std::size_t index) noexcept
{
    return index < children_.size() ? children_[index].get() : nullptr;
}

const Actor* Group::child_at(std::size_t index) const noexcept
{
    return index < children_.size() ? children_[index].get() : nullptr;
}

} // namespace squared::scene2d
