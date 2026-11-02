export module sq.gui:src.input_listener_id;

import std;

export namespace sq::gui
{

/** @brief Stable actor-local listener identifier; insertions increase it. */
using InputListenerId = std::uint64_t;

} // namespace sq::gui
