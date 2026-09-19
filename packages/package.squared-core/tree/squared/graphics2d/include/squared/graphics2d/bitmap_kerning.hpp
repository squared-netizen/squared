#pragma once

namespace sq::graphics2d {

/** @brief Pair-specific horizontal advance adjustment in source pixels. */
struct BitmapKerning final {
    char32_t first{0};
    char32_t second{0};
    int amount{0};
};

} // namespace sq::graphics2d
