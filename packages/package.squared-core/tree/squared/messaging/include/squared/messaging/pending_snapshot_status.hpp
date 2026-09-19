#pragma once

namespace sq::messaging {

/** @brief Outcome category of one pending-queue snapshot. */
enum class PendingSnapshotStatus {
    Ready,
    SubscriptionTargetedDelivery
};

}  // namespace sq::messaging
