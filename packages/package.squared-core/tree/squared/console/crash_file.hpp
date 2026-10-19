#pragma once

#include <squared/console/console.hpp>

#include <cstddef>

namespace sq::console
{

/**
 * @brief Write a report to `path` when the process dies of a fatal signal
 * (SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL, SIGTRAP): the crash trace
 * (`sq::dev::write_crash_trace`), then the newest `log_lines` lines of
 * `console`'s scrollback, so the report says what led up to it.
 *
 * ```text
 * CRASH SIGSEGV (11) code 1 addr 0x0
 *   #0 pc 0x4a5f0 libdemo.so
 *   ...
 * END
 * log: the last 40 lines
 *   show landing: ...
 * ```
 *
 * The file is created at the crash, never before, so an existing report is
 * left alone until then; read it first (show_crash_file). The signal is then
 * handed on to whatever handled it before (the dev log's, in a dev build, then
 * Android's own crash report), so nothing else is lost. The handler allocates
 * nothing and takes no lock; it runs on an alternate stack on this thread, so
 * a stack overflow here is reported too.
 *
 * `path` is copied (at most 511 bytes); `console` must stay alive until
 * remove_crash_file() or the end of the process. One installation at a time.
 *
 * @return false if `path` is too long or a handler could not be installed.
 */
[[nodiscard]] bool install_crash_file( const char *path, const Console *console,
                                       std::size_t log_lines = 40 ) noexcept;

/// Put back the handlers install_crash_file() replaced. Before the Console goes away.
void remove_crash_file() noexcept;

/**
 * @brief Print the report an earlier run left at `path` into `console`, a line
 * at a time as errors, then move it to `previous_path` so it is shown once.
 * @return true if there was a report.
 */
bool show_crash_file( Console &console, const char *path, const char *previous_path ) noexcept;

/**
 * @brief Add the `crash` command: the report kept at `previous_path`, again;
 * `crash segv` and `crash abort` crash the process on purpose, to see a report
 * made. `previous_path` is kept, not copied: it must outlive the Console.
 */
[[nodiscard]] Commands::AddResult add_crash_command( Console &console,
                                                     const char *previous_path ) noexcept;

} // namespace sq::console
