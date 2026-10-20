#pragma once

#include <squared/messaging/name_table.hpp>
#include <squared/messaging/telegram.hpp>
#include <squared/messaging/telegram_data.hpp>

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

namespace sq::messaging
{

/**
 * @brief A telegram as its one canonical line of text: what the dev log
 * prints, and what replay files and saves hold.
 *
 * ```text
 * set settings.sound false
 * do file.open "notes.txt" @12
 * state item[17].name "fur coat"
 * state player.pos (1.5 2)
 * event game.over @12
 * ```
 *
 * `<verb> <address> <value>`, then ` @<correlation>` when it is not 0. The
 * value: `true`/`false`; an integer in decimal; a double in the shortest form
 * that reads back to the same bits, always with a `.` or an exponent so it is
 * not an integer (`2.0`, `1e+21`, `inf`); two to four floats in parentheses;
 * text in double quotes with `\\`, `\"`, `\n`, `\t` and `\xNN` for any other
 * byte below 0x20 or 0x7F (other bytes as they are); uids in brackets. The
 * same telegram always gives the same line, on every device: no locale.
 *
 * @param data The data area the telegram's text or uid list is in.
 * @return The length written, or 0 when `out` is too small, the telegram is
 * not valid, or its data is outside `data`.
 */
[[nodiscard]] std::size_t write_telegram( const Telegram &telegram, std::span<const std::byte> data,
                                          const NameTable &names, std::span<char> out ) noexcept;

/**
 * @brief A telegram from its line, with no table (names are hashed). Text and
 * uid lists go into `data`.
 *
 * Reads more than the canonical form (more spaces, `[0]`, `1.50`), and
 * writing what it read gives the canonical line.
 * @return The telegram, or nothing when the line is not one, or `data` is full.
 */
[[nodiscard]] std::optional<Telegram> read_telegram( std::string_view line,
                                                     TelegramData &data ) noexcept;

/** @brief The text of a verb: `set`, `do`, `state`, `event`. */
[[nodiscard]] std::string_view verb_text( Verb verb ) noexcept;

} // namespace sq::messaging
