#include <squared/messaging/src/receipt.hpp>

#include <squared/messaging/src/message_id.hpp>
#include <squared/messaging/src/receipt_status.hpp>

#include <string_view>

namespace sq::messaging {

const MessageId& receipt_message_id()
{
    static const MessageId value("squared.messaging.receipt");
    return value;
}

std::string_view receipt_status_name(ReceiptStatus status) noexcept
{
    switch (status) {
    case ReceiptStatus::Handled:
        return "handled";
    case ReceiptStatus::Unhandled:
        return "unhandled";
    case ReceiptStatus::ReceiverUnavailable:
        return "receiver_unavailable";
    case ReceiptStatus::Cancelled:
        return "cancelled";
    }
    return "unhandled";
}

}  // namespace sq::messaging
