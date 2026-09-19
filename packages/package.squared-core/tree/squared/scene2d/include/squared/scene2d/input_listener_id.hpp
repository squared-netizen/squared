#pragma once

#include <cstdint>

namespace sq::scene2d {

/** @brief Stable actor-local listener identifier; insertions increase it. */
using InputListenerId = std::uint64_t;

} // namespace sq::scene2d
