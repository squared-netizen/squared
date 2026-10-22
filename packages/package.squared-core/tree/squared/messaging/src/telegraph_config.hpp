#pragma once

#include <cstddef>

// The Telegraph's capacities: compile-time knobs, like the Console's, set
// with -D (`make TELEGRAPH_RING=512` in a project passes
// -DSQUARED_TELEGRAPH_RING=512 to the framework and the app alike). The
// defaults suit a small app; a large one raises them, from what the
// Console's `telegraph` command shows of the highest fill.

#ifndef SQUARED_TELEGRAPH_RING
#define SQUARED_TELEGRAPH_RING 256 // telegrams each way
#endif
#ifndef SQUARED_TELEGRAPH_DATA
#define SQUARED_TELEGRAPH_DATA 8192 // bytes of text and uid lists each way
#endif
#ifndef SQUARED_TELEGRAPH_PENDING
#define SQUARED_TELEGRAPH_PENDING 256 // model-side states waiting for ring room
#endif
#ifndef SQUARED_TELEGRAPH_CACHE
#define SQUARED_TELEGRAPH_CACHE 512 // latest values on the UI side
#endif
#ifndef SQUARED_TELEGRAPH_CACHE_TEXT
#define SQUARED_TELEGRAPH_CACHE_TEXT 8192 // their text over 16 bytes
#endif
#ifndef SQUARED_TELEGRAPH_LISTS
#define SQUARED_TELEGRAPH_LISTS 16 // virtualized lists a model may declare
#endif
#ifndef SQUARED_TELEGRAPH_LIST_VIEW
#define SQUARED_TELEGRAPH_LIST_VIEW 128 // the most rows a list's view holds
#endif
#ifndef SQUARED_TELEGRAPH_NAMES
#define SQUARED_TELEGRAPH_NAMES 8192 // the dev tools' name table, for the text form
#endif
#ifndef SQUARED_TELEGRAPH_WAIT_MS
#define SQUARED_TELEGRAPH_WAIT_MS 100 // how long the model waits for room, then `full`
#endif

namespace sq::messaging::telegraph_size
{

inline constexpr std::size_t ring = SQUARED_TELEGRAPH_RING;
inline constexpr std::size_t data = SQUARED_TELEGRAPH_DATA;
inline constexpr std::size_t pending = SQUARED_TELEGRAPH_PENDING;
// Pending text is as large as a ring's data, so whatever a ring takes, the
// pending table can hold (both refuse a value over a quarter of it).
inline constexpr std::size_t pending_text = SQUARED_TELEGRAPH_DATA;
inline constexpr std::size_t cache = SQUARED_TELEGRAPH_CACHE;
inline constexpr std::size_t cache_text = SQUARED_TELEGRAPH_CACHE_TEXT;
inline constexpr std::size_t lists = SQUARED_TELEGRAPH_LISTS;
inline constexpr std::size_t list_view = SQUARED_TELEGRAPH_LIST_VIEW;
inline constexpr int wait_ms = SQUARED_TELEGRAPH_WAIT_MS;
// In the dev tools only (console::TelegraphTools), not in the line: a plain
// build holds no name table. About 250 names at the default.
inline constexpr std::size_t names = SQUARED_TELEGRAPH_NAMES;

[[nodiscard]] constexpr bool power_of_two( std::size_t n ) noexcept
{
    return n != 0 && ( n & ( n - 1 ) ) == 0;
}

static_assert( power_of_two( ring ) && ring >= 2, "TELEGRAPH_RING: a power of two, 2 or more" );
static_assert( power_of_two( data ) && data >= 16, "TELEGRAPH_DATA: a power of two, 16 or more" );
static_assert( power_of_two( pending ) && pending >= 2 && pending <= 32768,
               "TELEGRAPH_PENDING: a power of two, 2 to 32768" );
static_assert( power_of_two( cache ) && cache >= 2 && cache <= 32768,
               "TELEGRAPH_CACHE: a power of two, 2 to 32768" );
static_assert( cache_text <= std::size_t{ 4 } * 0xFFFF && pending_text <= std::size_t{ 4 } * 0xFFFF,
               "TELEGRAPH_CACHE_TEXT and TELEGRAPH_DATA: at most 262140 bytes" );
static_assert( lists >= 1 && list_view >= 1, "TELEGRAPH_LISTS and TELEGRAPH_LIST_VIEW: 1 or more" );
static_assert( wait_ms >= 0, "TELEGRAPH_WAIT_MS: 0 or more (0: never wait)" );
static_assert( names >= 64 && names <= std::size_t{ 1 } << 24,
               "TELEGRAPH_NAMES: 64 bytes to 16 MB" );

} // namespace sq::messaging::telegraph_size
