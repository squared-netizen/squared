#include <squared/scene2d/stage.hpp>

#include <vector>

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

bool Stage::dispatch_input(InputEvent& event, Actor* target)
{
    const bool pointer_event =
        event.type == InputType::pointer_move ||
        event.type == InputType::pointer_down ||
        event.type == InputType::pointer_up ||
        event.type == InputType::pointer_cancel;
    if (!target && pointer_event) target = hit(event.stage_x, event.stage_y, true);
    if (!target) return false;

    std::vector<Actor*> path;
    for (Actor* current = target; current; current = current->parent()) {
        path.push_back(current);
        if (current == &root_) break;
    }
    if (path.empty() || path.back() != &root_) return false;

    event.current_target_ = nullptr;
    event.handled_by_ = nullptr;
    event.handled_ = false;
    event.stopped_ = false;
    event.cancelled_ = false;
    event.target_ = target;
    auto select_current = [&event](Actor& actor, InputPhase phase) {
        event.current_target_ = &actor;
        event.phase_ = phase;
        float origin_x = 0.0F;
        float origin_y = 0.0F;
        for (const Actor* current = &actor; current;
             current = current->parent()) {
            origin_x += current->x();
            origin_y += current->y();
        }
        event.local_x_ = event.stage_x - origin_x;
        event.local_y_ = event.stage_y - origin_y;
    };

    // Root-to-parent capture.
    for (std::size_t index = path.size(); index > 1; --index) {
        Actor& current = *path[index - 1];
        select_current(current, InputPhase::capture);
        current.notify_input(event, true);
        if (event.stopped()) return event.handled();
    }

    select_current(*target, InputPhase::target);
    target->notify_input(event, true);
    if (!event.stopped()) target->notify_input(event, false);
    if (event.stopped()) return event.handled();

    // Parent-to-root bubble.
    for (std::size_t index = 1; index < path.size(); ++index) {
        Actor& current = *path[index];
        select_current(current, InputPhase::bubble);
        current.notify_input(event, false);
        if (event.stopped()) break;
    }
    return event.handled();
}

} // namespace squared::scene2d
