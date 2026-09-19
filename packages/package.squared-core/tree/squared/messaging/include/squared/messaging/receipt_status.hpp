#pragma once

namespace sq::messaging {

/** @brief Outcome of delivering one Telegram to its subscribers. */
enum class ReceiptStatus {
    Handled,
    Unhandled,
    ReceiverUnavailable,
    Cancelled
};

}  // namespace sq::messaging
