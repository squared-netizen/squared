#pragma once

#include <cstdint>

namespace sq::messaging
{

/** @brief Outcome category of one authoritative current-state request. */
enum class ProviderStatus : std::uint8_t { Provided, NoCurrentState, Failed };

} // namespace sq::messaging
