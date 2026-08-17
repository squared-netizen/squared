#pragma once

#include <squared/data/json.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace squared::messaging {

/**
 * @brief Stable namespaced identifier for one Telegram kind.
 *
 * Valid identifiers contain 1-128 ASCII letters, digits, dots, dashes,
 * underscores, slashes, or colons and include at least one namespace
 * separator (`.`, `/`, or `:`).
 */
class MessageId final {
public:
    /** @brief Construct an empty, invalid identifier. */
    MessageId() noexcept = default;

    /**
     * @brief Construct from a candidate namespace string.
     * @param value Candidate identifier; validated on construction.
     */
    explicit MessageId(std::string value);

    /**
     * @brief Validate the stored identifier.
     * @return true when the stored value satisfied the identifier rules.
     */
    [[nodiscard]] bool valid() const noexcept;

    /**
     * @brief Access the stored identifier text.
     * @return Text of the candidate identifier.
     */
    [[nodiscard]] std::string_view value() const noexcept;

    /** @brief Return whether two identifiers are textually equal. */
    friend bool operator==(
        const MessageId&,
        const MessageId&
    ) = default;

private:
    std::string value_;
    bool valid_{false};
};

/** @brief Stable namespaced address for one directed Telegraph endpoint. */
class EndpointId final {
public:
    /** @brief Construct an empty, invalid endpoint address. */
    EndpointId() noexcept = default;

    /**
     * @brief Construct from a candidate endpoint string.
     * @param value Candidate endpoint; validated on construction.
     */
    explicit EndpointId(std::string value);

    /**
     * @brief Validate the stored endpoint address.
     * @return true when the stored value satisfied the address rules.
     */
    [[nodiscard]] bool valid() const noexcept;

    /**
     * @brief Access the stored endpoint text.
     * @return Text of the candidate endpoint.
     */
    [[nodiscard]] std::string_view value() const noexcept;

    /** @brief Return whether two endpoints are textually equal. */
    friend bool operator==(
        const EndpointId&,
        const EndpointId&
    ) = default;

private:
    std::string value_;
    bool valid_{false};
};

/** @brief Application-owned correlation tag echoed by queued receipts. */
using CorrelationId = std::uint64_t;

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
        squared::data::JsonValue payload = {},
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
    [[nodiscard]] const squared::data::JsonValue& payload() const noexcept;

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
    squared::data::JsonValue payload_;
    std::optional<EndpointId> sender_;
    std::optional<EndpointId> receiver_;
    CorrelationId correlation_{0};
    bool receipt_requested_{false};
};

/** @brief Outcome of delivering one Telegram to its subscribers. */
enum class ReceiptStatus {
    Handled,
    Unhandled,
    ReceiverUnavailable,
    Cancelled
};

/** @brief Framework message kind used for queued return receipts. */
[[nodiscard]] const MessageId& receipt_message_id();

/** @brief Stable lowercase receipt status name used in JSON payloads. */
[[nodiscard]] std::string_view receipt_status_name(
    ReceiptStatus status
) noexcept;

}  // namespace squared::messaging
