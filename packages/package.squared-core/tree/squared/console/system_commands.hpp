#pragma once

#include <squared/console/console.hpp>

#include <cstddef>

namespace sq::console
{

/**
 * @brief Add the built-in commands that read the running system, through
 * Linux interfaces that work on Android too:
 *
 * | Command | Prints |
 * |---|---|
 * | `mem` | resident and peak memory (`/proc/self/status`), and what the system has available (`/proc/meminfo`) |
 * | `heap` | the allocator's in-use, free and mapped bytes (`mallinfo`) |
 * | `threads` | the process's thread count |
 * | `fds` | the open file descriptors |
 * | `device` | maker, model, Android version, API level and ABI (system properties), and the kernel (`uname`) |
 * | `smaps` | memory by kind (this program, native heap, GPU driver, Java runtime, stacks, other libraries, files, anonymous): PSS, RSS and private dirty, from `/proc/self/smaps` |
 * | `libs` | every shared library the process has mapped, with the address space it takes (`/proc/self/maps`) |
 *
 * Each reads into the stack and prints through the Console: nothing is
 * allocated, nothing is kept. Two groups for `dump` come with them: `sys`
 * (all seven) and `memory` (`mem`, `heap`, `smaps`).
 *
 * @return How many did not fit in the command or group tables: 0 when all were added.
 */
[[nodiscard]] std::size_t add_system_commands( Console &console ) noexcept;

/**
 * @brief Run a program and print what it writes (stdout and stderr), a line
 * at a time, at most `max_lines` of them; then how many more there were.
 * Waits for it to end, so for programs that do (`logcat -d`, not `logcat`).
 *
 * `argv` is the program's path, then its arguments, then nullptr; nothing is
 * searched for on a PATH. It runs as this process does, so it reads what the
 * app may read (on Android, logcat shows only the app's own lines). fork and
 * exec, a pipe, a 2 KB buffer on the stack: nothing is allocated here.
 *
 * ```cpp
 * const char *const logcat[] = { "/system/bin/logcat", "-d", "-t", "200", nullptr };
 * sq::console::run_program( console, logcat, 200 );
 * ```
 *
 * @return false when it could not run (no such program, no process).
 */
bool run_program( Console &console, const char *const *argv, std::size_t max_lines ) noexcept;

} // namespace sq::console
