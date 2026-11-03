export module sq.term:pty_config;

import std;

export namespace sq::term
{

/**
 * @brief The program a `Pty` starts, and the terminal it starts in.
 *
 * Only the views are held: what they name must outlive the `spawn` call,
 * not the `Pty`.
 */
struct PtyConfig
{
        /**
         * The program: a path (`/system/bin/sh`, `./run.sh`), or a name
         * (`zsh`) looked up on PATH, as a shell does: the environment's PATH
         * when `environment` is given, else this process's.
         */
        std::string_view program;
        /** The arguments after the program's name (its `argv[1]` on). */
        std::span<const std::string_view> arguments;
        /**
         * The environment, `NAME=value` each; empty: this process's own.
         * A terminal's program wants `TERM` among them.
         */
        std::span<const std::string_view> environment;
        /** Where it starts; empty: this process's current directory. */
        std::string_view directory;
        std::uint16_t columns = 80;
        std::uint16_t rows = 24;
};

} // namespace sq::term
