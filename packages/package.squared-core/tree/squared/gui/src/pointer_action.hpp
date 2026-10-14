#pragma once

#include <cstdint>

namespace sq::gui
{

/** @brief Pointer action delivered to widgets. */
enum class PointerAction : std::uint8_t { move, down, up, cancel };

} // namespace sq::gui
