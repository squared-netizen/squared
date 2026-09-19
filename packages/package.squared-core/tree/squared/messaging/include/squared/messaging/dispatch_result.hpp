#pragma once

#include <squared/messaging/dispatch_handle.hpp>
#include <squared/messaging/dispatch_status.hpp>

namespace sq::messaging {

/** @brief Owned result of one send/schedule operation. */
struct DispatchResult {
    /** @brief Outcome category of the operation. */
    DispatchStatus status{DispatchStatus::InvalidTelegram};

    /** @brief Stable handle used for cancellation; valid when Queued. */
    DispatchHandle handle;

    /** @brief Return whether the Telegram was queued for delivery. */
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return status == DispatchStatus::Queued;
    }
};

}  // namespace sq::messaging
