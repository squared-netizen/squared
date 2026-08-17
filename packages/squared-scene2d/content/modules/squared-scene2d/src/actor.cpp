#include <squared/scene2d/actor.hpp>

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace squared::scene2d {

void InputEvent::handle() noexcept
{
    handled_ = true;
    if (!handled_by_) handled_by_ = current_target_;
}

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

InputListenerId Actor::add_input_listener(
    InputListener listener,
    bool capture
)
{
    if (!listener) {
        throw std::invalid_argument("Scene2D input listener must not be empty");
    }
    const InputListenerId id = next_listener_id_++;
    listeners_.push_back({id, capture, std::move(listener)});
    return id;
}

bool Actor::remove_input_listener(InputListenerId id) noexcept
{
    const auto found = std::find_if(
        listeners_.begin(), listeners_.end(),
        [id](const ListenerEntry& entry) { return entry.id == id; }
    );
    if (found == listeners_.end()) return false;
    listeners_.erase(found);
    return true;
}

void Actor::input_event(InputEvent&) {}

void Actor::notify_input(InputEvent& event, bool capture_only)
{
    if (!capture_only) {
        input_event(event);
        if (event.stopped()) return;
    }

    // Copy matching callbacks so listeners may add or remove listeners during
    // notification without invalidating this iteration.
    std::vector<InputListener> callbacks;
    callbacks.reserve(listeners_.size());
    for (const ListenerEntry& entry : listeners_) {
        if (entry.capture == capture_only) callbacks.push_back(entry.listener);
    }
    for (InputListener& callback : callbacks) {
        callback(event);
        if (event.stopped()) return;
    }
}

bool Actor::contains(float local_x, float local_y) const noexcept
{
    return local_x >= 0.0F && local_y >= 0.0F &&
        local_x < width_ && local_y < height_;
}

} // namespace squared::scene2d
