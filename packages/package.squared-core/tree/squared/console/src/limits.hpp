#pragma once

#include <cstddef>

// The console's sizes, fixed at compile time. The Android template sets them
// from make: `make CONSOLE=app CONSOLE_SCROLLBACK=1048576` (see
// docs/programmer/console.md). A build without the console compiles none of
// this.
#ifndef SQUARED_CONSOLE_SCROLLBACK
#define SQUARED_CONSOLE_SCROLLBACK 65536
#endif
#ifndef SQUARED_CONSOLE_COMMANDS
#define SQUARED_CONSOLE_COMMANDS 64
#endif

namespace sq::console
{

/** @brief Bytes of scrollback text and line headers: about 1,000 lines at the default. */
inline constexpr std::size_t scrollback_bytes = SQUARED_CONSOLE_SCROLLBACK;

/** @brief Registered commands the table holds. */
inline constexpr std::size_t command_slots = SQUARED_CONSOLE_COMMANDS;

/** @brief Bytes in the input line; a longer line is cut at a character boundary. */
inline constexpr std::size_t input_bytes = 256;

/** @brief Bytes of input history: about 50 earlier lines. */
inline constexpr std::size_t history_bytes = 2048;

/** @brief Words a command line is split into, the command's name included. */
inline constexpr std::size_t max_words = 16;

/** @brief Bytes one printf() line may format to; longer is cut. */
inline constexpr std::size_t format_bytes = 512;

static_assert( scrollback_bytes >= 1024 && scrollback_bytes % 4 == 0,
               "SQUARED_CONSOLE_SCROLLBACK: at least 1024, and a multiple of 4" );
static_assert( scrollback_bytes <= 0x7FFFFFFF, "SQUARED_CONSOLE_SCROLLBACK: below 2 GB" );
static_assert( command_slots >= 1 && command_slots <= 1024,
               "SQUARED_CONSOLE_COMMANDS: between 1 and 1024" );

} // namespace sq::console
