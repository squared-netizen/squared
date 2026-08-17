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
    /** @brief Construct an inactive, empty subscription handle. */
    Subscription() noexcept = default;

    /** @brief Unregister from the dispatcher when still active. */
    ~Subscription();

    /**
     * @brief Move-construct from another handle.
     * @param other Handle to assume; left inactive afterward.
     */
    Subscription(Subscription&& other) noexcept;

    /**
     * @brief Move-assign from another handle.
     * @param other Handle to assume; left inactive afterward.
     * @return Reference to this handle.
     * @note Releases the current registration first, if any.
     */
    Subscription& operator=(Subscription&& other) noexcept;

    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;

    /** @brief Unregister from the dispatcher, if still active. */
    void reset() noexcept;

    /**
     * @brief Check registration state.
     * @return true while the subscription is still registered.
     */
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

/** @brief Outcome category of one registration operation. */
enum class SubscriptionStatus {
    Registered,
    InvalidId,
    AlreadyRegistered,
    CapacityReached,
    ProviderFailed,
    InitialStateQueueFull,
    HandleExhausted
};

/** @brief Owned result of one register/subscribe/provider operation. */
struct SubscriptionResult {
    /** @brief Outcome category of the operation. */
    SubscriptionStatus status{SubscriptionStatus::InvalidId};

    /** @brief Owned scoped registration; active only when Registered. */
    Subscription subscription;

    /** @brief Human-readable failure detail when not Registered. */
    std::string detail;

    /** @brief Return whether the operation registered successfully. */
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return status == SubscriptionStatus::Registered;
    }
};

/** @brief Stable handle for one queued or delayed Telegram. */
struct DispatchHandle {
    /** @brief Non-zero monotonic token; zero means invalid/empty. */
    std::uint64_t value{0};

    /** @brief Return whether the handle references a pending delivery. */
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return value != 0;
    }
};

/** @brief Outcome category of one dispatch operation. */
enum class DispatchStatus {
    Queued,
    InvalidTelegram,
    InvalidDelay,
    QueueFull,
    HandleExhausted
};

/** @brief Owned result of one send/schedule operation. */
struct DispatchResult {
    /** @brief Outcome category of the operation. */
    DispatchStatus status{DispatchStatus::InvalidTelegram};

    /** @brief Stable handle used for cancellation; valid when Queued. */
    DispatchHandle handle;

    /** @brief Return whether the Telegram was queued for delivery. */
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return status == DispatchStatus::Queued;
    }
};

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

/** @brief Capacity limits that bound every dispatcher queue. */
struct MessageDispatcherConfig {
    /** @brief Maximum pending Telegram count; must be at least one. */
    std::size_t pending_capacity{1024};

    /** @brief Maximum simultaneous producer registrations; at least one. */
    std::size_t subscription_capacity{256};

    /** @brief Maximum deliveries drained per update() call; at least one. */
    std::size_t maximum_deliveries_per_update{256};
};

/** @brief Non-owning view valid only during pending-message inspection. */
struct PendingMessageView {
    /** @brief Stable handle of the pending delivery. */
    DispatchHandle handle;

    /** @brief Remaining domain-time delay until delivery. */
    squared::time::Duration remaining_delay;

    /** @brief Reference to the pending Telegram; valid for the visit only. */
    const Telegram& telegram;

    /** @brief Whether the delivery targets an endpoint, not subscribers. */
    bool subscription_targeted{false};
};

/** @brief Outcome category of one pending-queue snapshot. */
enum class PendingSnapshotStatus {
    Ready,
    SubscriptionTargetedDelivery
};

/** @brief Owned result of one pending-queue snapshot. */
struct PendingSnapshotResult {
    /** @brief Outcome category of the snapshot. */
    PendingSnapshotStatus status{PendingSnapshotStatus::Ready};

    /** @brief Owned deterministic JSON snapshot; valid when Ready. */
    squared::data::JsonValue snapshot;

    /** @brief Human-readable failure detail when not Ready. */
    std::string detail;

    /** @brief Return whether a usable snapshot was produced. */
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return status == PendingSnapshotStatus::Ready;
    }
};

/** @brief Outcome category of one pending-queue restore. */
enum class PendingRestoreStatus {
    Restored,
    DispatcherNotEmpty,
    InvalidSnapshot,
    CapacityReached,
    TimeOverflow,
    HandleExhausted
};

/** @brief Owned result of one pending-queue restore. */
struct PendingRestoreResult {
    /** @brief Outcome category of the restore. */
    PendingRestoreStatus status{PendingRestoreStatus::InvalidSnapshot};

    /** @brief Number of pending deliveries restored when Restored. */
    std::size_t restored_count{0};

    /** @brief Human-readable failure detail when not Restored. */
    std::string detail;

    /** @brief Return whether the snapshot was atomically restored. */
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
    /**
     * @brief Construct a bounded router bound to one time domain.
     * @param clock Borrowed clock used for deadlines and queueing; must
     * outlive the dispatcher. Non-owning.
     * @param config Capacity and per-update delivery limits.
     */
    explicit MessageDispatcher(
        const squared::time::Clock& clock,
        MessageDispatcherConfig config = {}
    );

    /** @brief Release all registrations and pending deliveries. */
    ~MessageDispatcher();

    MessageDispatcher(const MessageDispatcher&) = delete;
    MessageDispatcher& operator=(const MessageDispatcher&) = delete;
    MessageDispatcher(MessageDispatcher&&) = delete;
    MessageDispatcher& operator=(MessageDispatcher&&) = delete;

    /**
     * @brief Attach a Telegraph to a directed endpoint address.
     * @param endpoint Candidate endpoint address, validated then required
     * to be unique.
     * @param telegraph Non-owning receiver; must outlive the subscription.
     * @return Registration result; keep the returned Subscription alive to
     * stay registered.
     * @note No Lua binding exists.
     */
    [[nodiscard]] SubscriptionResult register_endpoint(
        EndpointId endpoint,
        Telegraph& telegraph
    );

    /**
     * @brief Enroll a Telegraph as a broadcast subscriber of one message kind.
     * @param message Candidate message kind, validated before matching.
     * @param telegraph Non-owning receiver; must outlive the subscription.
     * @return Subscription result; keep the returned Subscription alive to
     * stay registered.
     */
    [[nodiscard]] SubscriptionResult subscribe(
        MessageId message,
        Telegraph& telegraph
    );

    /**
     * @brief Register the sole authoritative provider for one message kind.
     * @param message Message kind the provider covers.
     * @param provider_endpoint Endpoint receiving provider-generated current
     * state deliveries.
     * @param provider Non-owning generator invoked synchronously during
     * registration; must outlive the subscription.
     * @return Registration result; keep the returned Subscription alive to
     * stay registered.
     * @note No Lua binding exists.
     */
    [[nodiscard]] SubscriptionResult register_provider(
        MessageId message,
        EndpointId provider_endpoint,
        TelegramProvider& provider
    );

    /**
     * @brief Queue ordinary delivery at the current domain time.
     * @param telegram Envelope to deliver; copied into the pending queue.
     * @return Queued/results; use the handle for later cancellation.
     */
    [[nodiscard]] DispatchResult send(Telegram telegram);

    /**
     * @brief Queue delivery after a non-negative domain-time delay.
     * @param delay Non-negative delay in domain time.
     * @param telegram Envelope to deliver; copied into the pending queue.
     * @return Queued/results; use the handle for later cancellation.
     */
    [[nodiscard]] DispatchResult schedule(
        squared::time::Duration delay,
        Telegram telegram
    );

    /**
     * @brief Deliver synchronously for controlled internal use.
     *
     * Any requested receipt still enters the ordinary queue.
     * @param telegram Envelope to deliver using the current domain time.
     * @return Compressed delivery outcome.
     * @note Synchronous; bypasses the pending queue and delivery cap.
     */
    [[nodiscard]] DeliveryReport send_now(
        const Telegram& telegram
    );

    /**
     * @brief Cancel one pending delivery and queue its receipt if requested.
     * @param handle Stable handle returned by a prior send/schedule call.
     * @return Whether the delivery was found and cancelled.
     */
    [[nodiscard]] bool cancel(DispatchHandle handle);

    /**
     * @brief Deliver due Telegrams using one captured clock value.
     * @return Number of deliveries drained this call, capped by config.
     */
    std::size_t update();

    /**
     * @brief Count pending deliveries.
     * @return Number of currently pending deliveries, bounded by capacity.
     */
    [[nodiscard]] std::size_t pending_count() const noexcept;

    /**
     * @brief Report the pending delivery capacity.
     * @return Configured pending delivery capacity.
     */
    [[nodiscard]] std::size_t pending_capacity() const noexcept;

    /**
     * @brief Inspect pending deliveries in due-time and insertion order.
     *
     * This path copies no Telegram or payload. A view must not escape the
     * visitor call.
     * @tparam Visitor Callable accepting const PendingMessageView&.
     * @param visitor Invoked once per pending delivery; runs on the calling
     * thread.
     * @note No Lua binding exists.
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
     * @return Snapshot encoding every pending envelope with its remaining
     * delay; Ready unless a subscription-targeted delivery exists.
     * @note No Lua binding exists.
     */
    [[nodiscard]] PendingSnapshotResult snapshot_pending() const;

    /**
     * @brief Atomically restore a snapshot into an empty pending queue.
     * @param snapshot Snapshot produced by snapshot_pending().
     * @return Restored/count; the queue must be empty beforehand.
     * @note No Lua binding exists.
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
