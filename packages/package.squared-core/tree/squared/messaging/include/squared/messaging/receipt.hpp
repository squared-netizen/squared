#pragma once

// Framework constants for queued return receipts. Declared apart
// from Telegram and ReceiptStatus because they belong to neither.

#include <squared/messaging/receipt.hpp>
#include <squared/messaging/receipt_status.hpp>

#include <string_view>

namespace sq::messaging {

class MessageId;

/** @brief Framework message kind used for queued return receipts. */
[[nodiscard]] const MessageId& receipt_message_id();

/** @brief Stable lowercase receipt status name used in JSON payloads. */
[[nodiscard]] std::string_view receipt_status_name(
    ReceiptStatus status
) noexcept;

}  // namespace sq::messaging
