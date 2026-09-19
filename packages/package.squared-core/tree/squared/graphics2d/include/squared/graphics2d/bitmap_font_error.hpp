#pragma once

#include <squared/graphics2d/bitmap_font_error_code.hpp>

#include <cstddef>
#include <string>

namespace sq::graphics2d {

/** @brief One owned BMFont parsing or glyph-layout failure. */
struct BitmapFontError final {
    BitmapFontErrorCode code{BitmapFontErrorCode::none};
    /** @brief One-based descriptor line, or zero for non-line failures. */
    std::size_t line{0};
    std::string message;

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return code != BitmapFontErrorCode::none;
    }
};

} // namespace sq::graphics2d
