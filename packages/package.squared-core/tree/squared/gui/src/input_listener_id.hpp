#pragma once

#include <cstdint>

namespace sq::gui
{

/** @brief Stable actor-local listener identifier; insertions increase it. */
using InputListenerId = std::uint64_t;

} // namespace sq::gui
