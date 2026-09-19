#pragma once

#include <squared/data/json.hpp>
#include <squared/messaging/pending_snapshot_status.hpp>

#include <string>

namespace sq::messaging {

/** @brief Owned result of one pending-queue snapshot. */
struct PendingSnapshotResult {
    /** @brief Outcome category of the snapshot. */
    PendingSnapshotStatus status{PendingSnapshotStatus::Ready};

    /** @brief Owned deterministic JSON snapshot; valid when Ready. */
    sq::data::JsonValue snapshot;

    /** @brief Human-readable failure detail when not Ready. */
    std::string detail;

    /** @brief Return whether a usable snapshot was produced. */
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return status == PendingSnapshotStatus::Ready;
    }
};

}  // namespace sq::messaging
