#pragma once

#include <squared/console/console.hpp>
#include <squared/messaging/telegraph.hpp>

#include <cstddef>

namespace sq::console
{

/**
 * @brief The Telegraph's commands, on the UI's end of a line:
 *
 * | Command | Prints |
 * |---|---|
 * | `telegraph` | each capacity and its highest fill since start: both rings (telegrams and data), the cache (entries, text, evictions, drops), the states held back, the lists, and how often the model waited for room or was refused |
 * | `telegraph state` | every value the UI holds from the model, one line each in the text form (`state player.hp 42`), ordered by address |
 *
 * and their groups for `dump`: `telegraph` and `state`. On the UI thread,
 * as the Console is; `ui` must outlive the commands. Nothing is allocated.
 *
 * @return How many did not fit in the command or group tables: 0 when all were added.
 */
[[nodiscard]] std::size_t add_telegraph_commands( Console &console,
                                                  messaging::TelegraphUi &ui ) noexcept;

} // namespace sq::console
