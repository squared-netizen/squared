#pragma once

#include <squared/messaging/receipt_status.hpp>

#include <cstddef>

namespace sq::messaging {

/** @brief Compressed outcome describing one immediate delivery pass. */
struct DeliveryReport {
    /** @brief Whether the Telegram passed validation for delivery. */
    bool accepted{false};

    /** @brief Aggregate outcome across targeted receivers. */
    ReceiptStatus status{ReceiptStatus::ReceiverUnavailable};

    /** @brief Number of dispatch-mode targets consulted. */
    std::size_t receiver_count{0};

    /** @brief Number of targets that reported the Telegram as handled. */
    std::size_t handled_count{0};

    /** @brief Whether a return receipt entered the ordinary queue. */
    bool receipt_queued{false};
};

}  // namespace sq::messaging
