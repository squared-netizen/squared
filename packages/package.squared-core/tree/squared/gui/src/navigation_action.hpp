#pragma once

#include <cstdint>

namespace sq::gui
{

/** @brief Semantic focus and navigation action from any supported device. */
enum class NavigationAction : std::uint8_t {
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

} // namespace sq::gui
