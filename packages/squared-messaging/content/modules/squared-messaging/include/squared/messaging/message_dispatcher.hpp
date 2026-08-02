#pragma once

#include <squared/messaging/telegram.hpp>
#include <squared/messaging/telegraph.hpp>
#include <squared/messaging/telegram_provider.hpp>
#include <squared/time/deadline_queue.hpp>
#include <squared/time/timepiece.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

namespace squared::messaging {

class MessageDispatcher;

/**
 * @brief Move-only scoped endpoint registration or broadcast subscription.
 *
 * Destroying or resetting the handle unregisters it. A subscription may
 * safely outlive its dispatcher, but a referenced Telegraph or
 * TelegramProvider must remain alive while the subscription is active.
 */
class Subscription final {
public:
    Subscription() noexcept = default;
    ~Subscription();

    Subscription(Subscription&& other) noexcept;
    Subscription& operator=(Subscription&& other) noexcept;

    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;

    void reset() noexcept;
    [[nodiscard]] bool active() const noexcept;

private:
    struct Registry;

    Subscription(
        std::weak_ptr<Registry> registry,
        std::uint64_t token
    ) noexcept;

    std::weak_ptr<Registry> registry_;
    std::uint64_t token_{0};

    friend class MessageDispatcher;
};

enum class SubscriptionStatus {
    Registered,
    InvalidId,
    AlreadyRegistered,
    CapacityReached,
    ProviderFailed,
    InitialStateQueueFull,
    HandleExhausted
};

struct SubscriptionResult {
    SubscriptionStatus status{SubscriptionStatus::InvalidId};
    Subscription subscription;
    std::string detail;

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return status == SubscriptionStatus::Registered;
    }
};

/** @brief Stable handle for one queued or delayed Telegram. */
struct DispatchHandle {
    std::uint64_t value{0};

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return value != 0;
    }
};

enum class DispatchStatus {
    Queued,
    InvalidTelegram,
    InvalidDelay,
    QueueFull,
    HandleExhausted
};

struct DispatchResult {
    DispatchStatus status{DispatchStatus::InvalidTelegram};
    DispatchHandle handle;

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return status == DispatchStatus::Queued;
    }
};

struct DeliveryReport {
    bool accepted{false};
    ReceiptStatus status{ReceiptStatus::ReceiverUnavailable};
    std::size_t receiver_count{0};
    std::size_t handled_count{0};
    bool receipt_queued{false};
};

struct MessageDispatcherConfig {
    std::size_t pending_capacity{1024};
    std::size_t subscription_capacity{256};
    std::size_t maximum_deliveries_per_update{256};
};

/** @brief Non-owning view valid only during pending-message inspection. */
struct PendingMessageView {
    DispatchHandle handle;
    squared::time::Duration remaining_delay;
    const Telegram& telegram;
    bool subscription_targeted{false};
};

enum class PendingSnapshotStatus {
    Ready,
    SubscriptionTargetedDelivery
};

struct PendingSnapshotResult {
    PendingSnapshotStatus status{PendingSnapshotStatus::Ready};
    squared::data::JsonValue snapshot;
    std::string detail;

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return status == PendingSnapshotStatus::Ready;
    }
};

enum class PendingRestoreStatus {
    Restored,
    DispatcherNotEmpty,
    InvalidSnapshot,
    CapacityReached,
    TimeOverflow,
    HandleExhausted
};

struct PendingRestoreResult {
    PendingRestoreStatus status{PendingRestoreStatus::InvalidSnapshot};
    std::size_t restored_count{0};
    std::string detail;

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return status == PendingRestoreStatus::Restored;
    }
};

/**
 * @brief Bounded deterministic Telegram router for one time domain.
 *
 * The dispatcher borrows a Clock, owns no thread, and invokes Telegraphs on
 * the thread calling update() or send_now(). Ordinary send() is queued.
 */
class MessageDispatcher final {
public:
    explicit MessageDispatcher(
        const squared::time::Clock& clock,
        MessageDispatcherConfig config = {}
    );
    ~MessageDispatcher();

    MessageDispatcher(const MessageDispatcher&) = delete;
    MessageDispatcher& operator=(const MessageDispatcher&) = delete;
    MessageDispatcher(MessageDispatcher&&) = delete;
    MessageDispatcher& operator=(MessageDispatcher&&) = delete;

    [[nodiscard]] SubscriptionResult register_endpoint(
        EndpointId endpoint,
        Telegraph& telegraph
    );

    [[nodiscard]] SubscriptionResult subscribe(
        MessageId message,
        Telegraph& telegraph
    );

    /**
     * @brief Register the sole authoritative provider for one message kind.
     */
    [[nodiscard]] SubscriptionResult register_provider(
        MessageId message,
        EndpointId provider_endpoint,
        TelegramProvider& provider
    );

    /** @brief Queue ordinary delivery at the current domain time. */
    [[nodiscard]] DispatchResult send(Telegram telegram);

    /** @brief Queue delivery after a non-negative domain-time delay. */
    [[nodiscard]] DispatchResult schedule(
        squared::time::Duration delay,
        Telegram telegram
    );

    /**
     * @brief Deliver synchronously for controlled internal use.
     *
     * Any requested receipt still enters the ordinary queue.
     */
    [[nodiscard]] DeliveryReport send_now(
        const Telegram& telegram
    );

    /** @brief Cancel one pending delivery and queue its receipt if requested. */
    [[nodiscard]] bool cancel(DispatchHandle handle);

    /** @brief Deliver due Telegrams using one captured clock value. */
    std::size_t update();

    [[nodiscard]] std::size_t pending_count() const noexcept;
    [[nodiscard]] std::size_t pending_capacity() const noexcept;

    /**
     * @brief Inspect pending deliveries in due-time and insertion order.
     *
     * This path copies no Telegram or payload. A view must not escape the
     * visitor call.
     */
    template<typename Visitor>
    void inspect_pending(Visitor&& visitor) const
    {
        using VisitorType = std::remove_reference_t<Visitor>;
        VisitorType* visitor_pointer = &visitor;
        inspect_pending_erased(
            visitor_pointer,
            [](void* context, const PendingMessageView& view) {
                (*static_cast<VisitorType*>(context))(view);
            }
        );
    }

    /**
     * @brief Create deterministic versioned JSON using remaining delays.
     */
    [[nodiscard]] PendingSnapshotResult snapshot_pending() const;

    /**
     * @brief Atomically restore a snapshot into an empty pending queue.
     */
    [[nodiscard]] PendingRestoreResult restore_pending(
        const squared::data::JsonValue& snapshot
    );

private:
    struct PendingDelivery {
        Telegram telegram;
        std::optional<std::uint64_t> subscription_target;
    };

    [[nodiscard]] DispatchResult enqueue_at(
        squared::time::TimePoint due,
        Telegram telegram,
        std::optional<std::uint64_t> subscription_target =
            std::nullopt
    );
    [[nodiscard]] DeliveryReport deliver(
        const Telegram& telegram,
        squared::time::TimePoint captured_now,
        std::optional<std::uint64_t> subscription_target
    );
    [[nodiscard]] bool queue_receipt(
        const Telegram& original,
        ReceiptStatus status,
        squared::time::TimePoint captured_now
    );
    using ErasedPendingVisitor = void (*)(
        void*,
        const PendingMessageView&
    );
    void inspect_pending_erased(
        void* context,
        ErasedPendingVisitor visitor
    ) const;

    const squared::time::Clock& clock_;
    MessageDispatcherConfig config_;
    std::shared_ptr<Subscription::Registry> registry_;
    squared::time::DeadlineQueue<PendingDelivery> pending_;
};

}  // namespace squared::messaging
