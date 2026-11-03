export module sq.term:process_exit;

import std;

export namespace sq::term
{

/** @brief How a program ended: its exit code, or the signal that ended it. */
struct ProcessExit
{
        /** `exit(code)`; 127 when the program could not be run. Unused when `signal` is set. */
        int code = 0;
        /** The signal that ended it (2 for Ctrl-C's SIGINT), or 0. */
        int signal = 0;

        friend constexpr bool operator==( ProcessExit, ProcessExit ) noexcept = default;
};

} // namespace sq::term
