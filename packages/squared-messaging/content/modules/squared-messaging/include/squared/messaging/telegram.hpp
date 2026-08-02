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
    MessageId() noexcept = default;
    explicit MessageId(std::string value);

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] std::string_view value() const noexcept;

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
    EndpointId() noexcept = default;
    explicit EndpointId(std::string value);

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] std::string_view value() const noexcept;

    friend bool operator==(
        const EndpointId&,
        const EndpointId&
    ) = default;

private:
    std::string value_;
    bool valid_{false};
};

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
    Telegram(
        MessageId message,
        squared::data::JsonValue payload = {},
        std::optional<EndpointId> sender = std::nullopt,
        std::optional<EndpointId> receiver = std::nullopt,
        CorrelationId correlation = 0,
        bool receipt_requested = false
    );

    [[nodiscard]] const MessageId& message() const noexcept;
    [[nodiscard]] const squared::data::JsonValue& payload() const noexcept;
    [[nodiscard]] const std::optional<EndpointId>& sender() const noexcept;
    [[nodiscard]] const std::optional<EndpointId>& receiver() const noexcept;
    [[nodiscard]] CorrelationId correlation() const noexcept;
    [[nodiscard]] bool receipt_requested() const noexcept;
    [[nodiscard]] bool directed() const noexcept;
    [[nodiscard]] bool valid() const noexcept;

private:
    MessageId message_;
    squared::data::JsonValue payload_;
    std::optional<EndpointId> sender_;
    std::optional<EndpointId> receiver_;
    CorrelationId correlation_{0};
    bool receipt_requested_{false};
};

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
