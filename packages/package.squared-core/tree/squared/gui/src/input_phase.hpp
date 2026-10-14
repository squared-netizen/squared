#pragma once

#include <cstdint>

namespace sq::gui
{

/** @brief Dispatch phase of an event inside a Stage. */
enum class InputPhase : std::uint8_t { capture, target, bubble };

} // namespace sq::gui
