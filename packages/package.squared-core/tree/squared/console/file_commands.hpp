#pragma once

#include <squared/console/bridge.hpp>
#include <squared/console/console.hpp>

#include <cstddef>

namespace sq::console
{

/**
 * @brief Add the commands for the app's own files:
 *
 * | Command | Does |
 * |---|---|
 * | `ls [path]` | the entries of a directory: type, size, name |
 * | `pull <path>` | send a file to Termux through `bridge` (`make console` saves it) |
 * | `push` | how to send one the other way (it is `make console`'s: `push <file> [name]`) |
 *
 * A relative path starts at `home` (the Android template: the app's data
 * directory, so `files/crash.txt`, `cache/`, `lib/`); an absolute one is used
 * as it is; `apk` is the installed APK. Everything the app may read, and only
 * that: these run as the app.
 *
 * `home` and `bridge` are kept, not copied; `bridge` may be null, and then
 * `pull` says it needs one. Nothing allocates.
 *
 * @return How many did not fit in the table (0 when all did).
 */
[[nodiscard]] std::size_t add_file_commands( Console &console, const char *home,
                                             Bridge *bridge ) noexcept;

} // namespace sq::console
