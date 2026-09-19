#pragma once

#include <squared/messaging/pending_restore_status.hpp>

#include <cstddef>
#include <string>

namespace sq::messaging {

/** @brief Owned result of one pending-queue restore. */
struct PendingRestoreResult {
    /** @brief Outcome category of the restore. */
    PendingRestoreStatus status{PendingRestoreStatus::InvalidSnapshot};

    /** @brief Number of pending deliveries restored when Restored. */
    std::size_t restored_count{0};

    /** @brief Human-readable failure detail when not Restored. */
    std::string detail;

    /** @brief Return whether the snapshot was atomically restored. */
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return status == PendingRestoreStatus::Restored;
    }
};

}  // namespace sq::messaging
