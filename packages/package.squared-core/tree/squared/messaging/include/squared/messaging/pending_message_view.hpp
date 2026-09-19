#pragma once

#include <squared/messaging/dispatch_handle.hpp>
#include <squared/time/duration.hpp>

namespace sq::messaging {

class Telegram;

/** @brief Non-owning view valid only during pending-message inspection. */
struct PendingMessageView {
    /** @brief Stable handle of the pending delivery. */
    DispatchHandle handle;

    /** @brief Remaining domain-time delay until delivery. */
    sq::time::Duration remaining_delay;

    /** @brief Reference to the pending Telegram; valid for the visit only. */
    const Telegram& telegram;

    /** @brief Whether the delivery targets an endpoint, not subscribers. */
    bool subscription_targeted{false};
};

}  // namespace sq::messaging
