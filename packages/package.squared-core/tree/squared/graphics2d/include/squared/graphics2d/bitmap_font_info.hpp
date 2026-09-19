#pragma once

#include <string>

namespace sq::graphics2d {

/** @brief Portable BMFont face metadata. */
struct BitmapFontInfo final {
    std::string face;
    int size{0};
    bool bold{false};
    bool italic{false};
    bool unicode{false};
};

} // namespace sq::graphics2d
