#pragma once

#include <cstdint>

namespace sq::graphics
{

/** @brief Stable failure categories for BMFont parsing and glyph layout. */
enum class BitmapFontErrorCode : std::uint8_t {
    none,
    invalid_argument,
    limit_exceeded,
    malformed,
    unsupported,
    duplicate,
    missing_required,
    invalid_utf8
};

} // namespace sq::graphics
