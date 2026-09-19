#pragma once

namespace sq::graphics2d {

/** @brief Stable failure categories for BMFont parsing and glyph layout. */
enum class BitmapFontErrorCode {
    none,
    invalid_argument,
    limit_exceeded,
    malformed,
    unsupported,
    duplicate,
    missing_required,
    invalid_utf8
};

} // namespace sq::graphics2d
