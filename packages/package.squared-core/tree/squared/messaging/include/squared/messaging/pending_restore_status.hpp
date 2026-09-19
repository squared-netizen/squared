#pragma once

namespace sq::messaging {

/** @brief Outcome category of one pending-queue restore. */
enum class PendingRestoreStatus {
    Restored,
    DispatcherNotEmpty,
    InvalidSnapshot,
    CapacityReached,
    TimeOverflow,
    HandleExhausted
};

}  // namespace sq::messaging
