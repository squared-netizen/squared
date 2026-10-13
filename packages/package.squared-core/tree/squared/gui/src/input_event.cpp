#include <squared/gui/src/input_event.hpp>

#include <squared/gui/actor.hpp>

namespace sq::gui {

void InputEvent::handle() noexcept
{
    handled_ = true;
    if (!handled_by_) handled_by_ = current_target_;
}

} // namespace sq::gui
