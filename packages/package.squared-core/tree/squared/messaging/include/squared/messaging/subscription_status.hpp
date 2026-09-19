#pragma once

namespace sq::messaging {

/** @brief Outcome category of one registration operation. */
enum class SubscriptionStatus {
    Registered,
    InvalidId,
    AlreadyRegistered,
    CapacityReached,
    ProviderFailed,
    InitialStateQueueFull,
    HandleExhausted
};

}  // namespace sq::messaging
