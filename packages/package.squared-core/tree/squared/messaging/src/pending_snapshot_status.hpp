#pragma once

#include <cstdint>

namespace sq::messaging
{

/** @brief Outcome category of one pending-queue snapshot. */
enum class PendingSnapshotStatus : std::uint8_t { Ready, SubscriptionTargetedDelivery };

} // namespace sq::messaging
