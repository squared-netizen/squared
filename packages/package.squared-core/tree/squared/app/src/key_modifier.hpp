#pragma once

#include <cstdint>

namespace sq::app {

/** @brief Platform-neutral modifier keys carried by keyboard events. */
enum class KeyModifier : std::uint8_t {
    shift = 1U << 0U,
    control = 1U << 1U,
    alt = 1U << 2U,
    meta = 1U << 3U
};

}  // namespace sq::app
