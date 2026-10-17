#pragma once

#include <squared/console/console.hpp>
#include <squared/console/src/clipboard_writer.hpp>

namespace sq::console
{

/**
 * @brief Add the `copy` command, which writes lines of the scrollback to
 * `writer`, one per line, separated by newlines:
 *
 * | Typed | Copies |
 * |---|---|
 * | `copy` | every line |
 * | `copy errors` | the error lines only |
 * | `copy 20` | the last 20 lines |
 *
 * The command's own echo (`> copy`) is not copied. It prints how many lines
 * and bytes it copied, or that the clipboard is not available. `writer` is
 * kept by address: it must outlive the Console. Nothing is allocated.
 */
[[nodiscard]] Commands::AddResult add_copy_command( Console &console,
                                                    const ClipboardWriter &writer ) noexcept;

} // namespace sq::console
