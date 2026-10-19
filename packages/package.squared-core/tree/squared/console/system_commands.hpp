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
 *
 * Each reads into the stack and prints through the Console: nothing is
 * allocated, nothing is kept.
 *
 * @return How many did not fit in the command table: 0 when all were added.
 */
[[nodiscard]] std::size_t add_system_commands( Console &console ) noexcept;

} // namespace sq::console
