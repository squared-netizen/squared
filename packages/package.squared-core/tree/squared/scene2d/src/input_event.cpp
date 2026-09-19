#include <squared/scene2d/input_event.hpp>

#include <squared/scene2d/actor.hpp>

namespace sq::scene2d {

void InputEvent::handle() noexcept
{
    handled_ = true;
    if (!handled_by_) handled_by_ = current_target_;
}

} // namespace sq::scene2d
