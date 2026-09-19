#pragma once

#include <cstddef>

namespace sq::graphics2d {

/** @brief Explicit resource limits for one text BMFont descriptor. */
struct BitmapFontParseLimits final {
    std::size_t maximum_bytes{1024U * 1024U};
    std::size_t maximum_line_bytes{4096};
    std::size_t maximum_pages{64};
    std::size_t maximum_glyphs{65536};
    std::size_t maximum_kernings{262144};
    std::size_t maximum_name_bytes{512};
};

} // namespace sq::graphics2d
