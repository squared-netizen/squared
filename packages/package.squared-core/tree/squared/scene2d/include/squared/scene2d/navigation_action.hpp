#pragma once

namespace sq::scene2d {

/** @brief Semantic focus and navigation action from any supported device. */
enum class NavigationAction {
    unknown,
    left,
    right,
    up,
    down,
    next,
    previous,
    activate,
    cancel
};

} // namespace sq::scene2d
