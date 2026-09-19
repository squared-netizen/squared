#pragma once

namespace sq::gui {

/** @brief Scissor amounts used by nine-patch scaling. */
struct NinePatchSplits {
    /** @brief Left corner width in source pixels. */
    int left{0};
    /** @brief Top corner height in source pixels. */
    int top{0};
    /** @brief Right corner width in source pixels. */
    int right{0};
    /** @brief Bottom corner height in source pixels. */
    int bottom{0};
};

} // namespace sq::gui
