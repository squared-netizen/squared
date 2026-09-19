#pragma once

#include <cstddef>
#include <string>

namespace sq::graphics2d {

/** @brief One texture-page dependency declared by a BMFont. */
struct BitmapFontPage final {
    std::size_t id{0};
    /** @brief Safe relative asset path resolved by the application. */
    std::string file;
};

} // namespace sq::graphics2d
