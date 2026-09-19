#pragma once

#include <squared/data/json.hpp>
#include <squared/messaging/delivery_report.hpp>
#include <squared/messaging/dispatch_handle.hpp>
#include <squared/messaging/dispatch_result.hpp>
#include <squared/messaging/endpoint_id.hpp>
#include <squared/messaging/message_dispatcher_config.hpp>
#include <squared/messaging/message_id.hpp>
#include <squared/messaging/pending_restore_result.hpp>
#include <squared/messaging/pending_snapshot_result.hpp>
#include <squared/messaging/receipt_status.hpp>
#include <squared/messaging/subscription.hpp>
#include <squared/messaging/subscription_result.hpp>
#include <squared/messaging/telegram.hpp>
#include <squared/time/clock.hpp>
#include <squared/time/deadline_queue.hpp>
#include <squared/time/duration.hpp>
#include <squared/time/time_point.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

namespace sq::messaging {

class TelegramProvider;
class Telegraph;
struct PendingMessageView;

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
        const sq::time::Clock& clock,
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
        sq::time::Duration delay,
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
        const sq::data::JsonValue& snapshot
    );

private:
    struct PendingDelivery {
        Telegram telegram;
        std::optional<std::uint64_t> subscription_target;
    };

    [[nodiscard]] DispatchResult enqueue_at(
        sq::time::TimePoint due,
        Telegram telegram,
        std::optional<std::uint64_t> subscription_target =
            std::nullopt
    );
    [[nodiscard]] DeliveryReport deliver(
        const Telegram& telegram,
        sq::time::TimePoint captured_now,
        std::optional<std::uint64_t> subscription_target
    );
    [[nodiscard]] bool queue_receipt(
        const Telegram& original,
        ReceiptStatus status,
        sq::time::TimePoint captured_now
    );
    using ErasedPendingVisitor = void (*)(
        void*,
        const PendingMessageView&
    );
    void inspect_pending_erased(
        void* context,
        ErasedPendingVisitor visitor
    ) const;

    const sq::time::Clock& clock_;
    MessageDispatcherConfig config_;
    std::shared_ptr<Subscription::Registry> registry_;
    sq::time::DeadlineQueue<PendingDelivery> pending_;
};

}  // namespace sq::messaging
