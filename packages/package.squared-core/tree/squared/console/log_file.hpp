#pragma once

#include <squared/console/console.hpp>

#include <cstddef>
#include <string_view>

namespace sq::console
{

/**
 * @brief Write the newest `count` lines of `console`'s scrollback to `path`,
 * one per line, so the next run can show how this one ended.
 *
 * Written to `path` with `.tmp` added, then renamed over `path`, so a reader
 * never finds half a file and a crash while writing leaves the last good one.
 * Allocates nothing: the lines go straight from the scrollback to the file.
 * Call it when the app may be about to go (paused, stopped, quitting); it costs
 * one small file write.
 *
 * ```cpp
 * sq::console::save_log( console, "/data/user/0/<package>/files/log.txt", 200 );
 * ```
 *
 * @return false if the file could not be written (the old one is kept), or
 * `path` is longer than 500 bytes.
 */
[[nodiscard]] bool save_log( const Console &console, const char *path, std::size_t count ) noexcept;

/**
 * @brief Move the log the last run saved at `path` to `previous_path`, at
 * start, before this run saves its own over it.
 * @return true if there was one.
 */
bool keep_previous_log( const char *path, const char *previous_path ) noexcept;

/**
 * @brief Print the file at `path` into `console`, a line at a time, after a
 * `title` line.
 * @return false, printing nothing, if there is no such file.
 */
bool show_log( Console &console, const char *path, std::string_view title ) noexcept;

/**
 * @brief Add the `log` command: `log previous` shows the end of the last run's
 * log, kept at `previous_path` by keep_previous_log(). `previous_path` is
 * kept, not copied: it must outlive the Console.
 */
[[nodiscard]] Commands::AddResult add_log_command( Console &console,
                                                   const char *previous_path ) noexcept;

} // namespace sq::console
