export module sq.term:terminal_events;

import std;

export namespace sq::term
{

/** @brief What happened since the owner last asked (`Terminal::take_events`). */
struct TerminalEvents
{
        /** BEL, outside a string. */
        bool bell : 1 = false;
        /** The title changed (OSC 0 or 2). */
        bool title : 1 = false;

        [[nodiscard]] explicit operator bool() const noexcept { return bell || title; }
};

} // namespace sq::term
