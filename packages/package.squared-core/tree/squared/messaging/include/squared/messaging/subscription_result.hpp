#pragma once

#include <squared/messaging/subscription.hpp>
#include <squared/messaging/subscription_status.hpp>

#include <string>

namespace sq::messaging {

/** @brief Owned result of one register/subscribe/provider operation. */
struct SubscriptionResult {
    /** @brief Outcome category of the operation. */
    SubscriptionStatus status{SubscriptionStatus::InvalidId};

    /** @brief Owned scoped registration; active only when Registered. */
    Subscription subscription;

    /** @brief Human-readable failure detail when not Registered. */
    std::string detail;

    /** @brief Return whether the operation registered successfully. */
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return status == SubscriptionStatus::Registered;
    }
};

}  // namespace sq::messaging
