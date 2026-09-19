#pragma once

#include <squared/data/json.hpp>
#include <squared/messaging/correlation_id.hpp>
#include <squared/messaging/endpoint_id.hpp>
#include <squared/messaging/message_id.hpp>

#include <optional>

namespace sq::messaging {

/**
 * @brief Owned immutable-access message envelope.
 *
 * A receiver identifies directed delivery. An absent receiver requests
 * broadcast delivery to subscribers of message(). Payloads are owned JSON
 * values and therefore never contain process-local pointers.
 */
class Telegram final {
public:
    /**
     * @brief Construct an owned immutable-access message envelope.
     * @param message Message kind used for subscription matching.
     * @param payload Application data carried by the envelope.
     * @param sender Origin endpoint, when directed.
     * @param receiver Destination endpoint for directed delivery; an absent
     * receiver requests broadcast delivery to subscribers.
     * @param correlation Application-owned correlation tag.
     * @param receipt_requested Whether a queued return receipt is requested.
     */
    Telegram(
        MessageId message,
        sq::data::JsonValue payload = {},
        std::optional<EndpointId> sender = std::nullopt,
        std::optional<EndpointId> receiver = std::nullopt,
        CorrelationId correlation = 0,
        bool receipt_requested = false
    );

    /**
     * @brief Access the message kind.
     * @return Message kind used for subscription matching.
     */
    [[nodiscard]] const MessageId& message() const noexcept;

    /**
     * @brief Access the owned application payload.
     * @return Reference to the stored payload; invalidated by destruction.
     */
    [[nodiscard]] const sq::data::JsonValue& payload() const noexcept;

    /**
     * @brief Access the origin endpoint.
     * @return Sender endpoint when set, otherwise std::nullopt.
     */
    [[nodiscard]] const std::optional<EndpointId>& sender() const noexcept;

    /**
     * @brief Access the destination endpoint.
     * @return Receiver endpoint for directed delivery, else broadcast.
     */
    [[nodiscard]] const std::optional<EndpointId>& receiver() const noexcept;

    /**
     * @brief Access the correlation tag.
     * @return Application-owned correlation tag.
     */
    [[nodiscard]] CorrelationId correlation() const noexcept;

    /**
     * @brief Report whether a return receipt was requested.
     * @return true when a queued return receipt was requested.
     */
    [[nodiscard]] bool receipt_requested() const noexcept;

    /**
     * @brief Report directed delivery.
     * @return true for directed, false for broadcast delivery.
     */
    [[nodiscard]] bool directed() const noexcept;

    /**
     * @brief Validate the envelope.
     * @return true when message and any endpoint identifiers are valid.
     */
    [[nodiscard]] bool valid() const noexcept;

private:
    MessageId message_;
    sq::data::JsonValue payload_;
    std::optional<EndpointId> sender_;
    std::optional<EndpointId> receiver_;
    CorrelationId correlation_{0};
    bool receipt_requested_{false};
};

}  // namespace sq::messaging
