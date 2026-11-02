export module sq.gui:src.input_listener;

import std;

export namespace sq::gui
{

class InputEvent;

/** @brief Callback invoked for each phase during input dispatch. */
using InputListener = std::function<void( InputEvent & )>;

} // namespace sq::gui
