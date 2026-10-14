#pragma once

#include <cstdint>

namespace sq::messaging
{

/** @brief Outcome category of one registration operation. */
enum class SubscriptionStatus : std::uint8_t {
    Registered,
    InvalidId,
    AlreadyRegistered,
    CapacityReached,
    ProviderFailed,
    InitialStateQueueFull,
    HandleExhausted
};

} // namespace sq::messaging
