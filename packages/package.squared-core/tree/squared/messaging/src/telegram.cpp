#include <squared/messaging/telegram.hpp>

#include <squared/data/json.hpp>
#include <squared/messaging/correlation_id.hpp>
#include <squared/messaging/endpoint_id.hpp>
#include <squared/messaging/message_id.hpp>

#include <optional>
#include <utility>

namespace sq::messaging {

Telegram::Telegram(
    MessageId message,
    sq::data::JsonValue payload,
    std::optional<EndpointId> sender,
    std::optional<EndpointId> receiver,
    CorrelationId correlation,
    bool receipt_requested
)
    : message_(std::move(message)),
      payload_(std::move(payload)),
      sender_(std::move(sender)),
      receiver_(std::move(receiver)),
      correlation_(correlation),
      receipt_requested_(receipt_requested)
{
}

const MessageId& Telegram::message() const noexcept
{
    return message_;
}

const sq::data::JsonValue& Telegram::payload() const noexcept
{
    return payload_;
}

const std::optional<EndpointId>& Telegram::sender() const noexcept
{
    return sender_;
}

const std::optional<EndpointId>& Telegram::receiver() const noexcept
{
    return receiver_;
}

CorrelationId Telegram::correlation() const noexcept
{
    return correlation_;
}

bool Telegram::receipt_requested() const noexcept
{
    return receipt_requested_;
}

bool Telegram::directed() const noexcept
{
    return receiver_.has_value();
}

bool Telegram::valid() const noexcept
{
    if (!message_.valid()) return false;
    if (sender_ && !sender_->valid()) return false;
    if (receiver_ && !receiver_->valid()) return false;
    if (receipt_requested_ &&
        (!sender_ || correlation_ == 0)) {
        return false;
    }
    return true;
}

}  // namespace sq::messaging
