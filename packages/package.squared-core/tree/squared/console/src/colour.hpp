#pragma once

#include <cstddef>
#include <cstdint>

namespace sq::console
{

/** @brief What a console quad is drawn in: an index into the renderer's palette. */
enum class Colour : std::uint8_t {
    text,       // an info line
    warning,    // a warning line
    error,      // an error line
    dim,        // the prompt
    background, // behind everything, translucent
    cursor,     // the block under the cursor
    title       // the title row and the close button
};

/** @brief How many colours the palette holds. */
inline constexpr std::size_t colour_count = 7;

} // namespace sq::console
