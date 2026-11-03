export module sq.term:pty_error;

import std;

export namespace sq::term
{

/** @brief Which step of starting a program failed. */
enum class PtyErrorCode : std::uint8_t {
    none,
    /** This `Pty` runs a program already. */
    already_running,
    /** `PtyConfig::program` is empty. */
    no_program,
    /** The pseudo-terminal could not be made (`posix_openpt`, `grantpt`, `unlockpt`, `ptsname_r`). */
    open_master,
    /** Its other end could not be opened or set up. */
    open_slave,
    /** No process could be made (`fork`). */
    fork,
};

/**
 * @brief Why `Pty::spawn` failed: true when it did.
 *
 * Owns no memory: the text is fixed, `system_error` is the `errno` of the
 * call that failed. A program that cannot be run is not one of these: the
 * process was made, and it says so in the terminal and exits with 127.
 */
struct PtyError
{
        PtyErrorCode code = PtyErrorCode::none;
        int system_error = 0;

        [[nodiscard]] explicit operator bool() const noexcept { return code != PtyErrorCode::none; }

        /** The step that failed, in a few words. */
        [[nodiscard]] constexpr std::string_view what() const noexcept
        {
            switch( code ) {
                case PtyErrorCode::none:
                    return "no error";
                case PtyErrorCode::already_running:
                    return "a program is running in this pty already";
                case PtyErrorCode::no_program:
                    return "no program given";
                case PtyErrorCode::open_master:
                    return "could not make a pseudo-terminal";
                case PtyErrorCode::open_slave:
                    return "could not open the pseudo-terminal's other end";
                case PtyErrorCode::fork:
                    return "could not make a process";
            }
            return "unknown";
        }
};

} // namespace sq::term
