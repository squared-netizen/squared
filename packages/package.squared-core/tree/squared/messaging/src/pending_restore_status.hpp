#pragma once

#include <cstdint>

namespace sq::messaging
{

/** @brief Outcome category of one pending-queue restore. */
enum class PendingRestoreStatus : std::uint8_t {
    Restored,
    DispatcherNotEmpty,
    InvalidSnapshot,
    CapacityReached,
    TimeOverflow,
    HandleExhausted
};

} // namespace sq::messaging
