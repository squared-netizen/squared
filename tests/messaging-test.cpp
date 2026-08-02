#include <squared/messaging/message_dispatcher.hpp>

#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using namespace std::chrono_literals;
using squared::messaging::Telegram;

void require(bool condition, const char* message)
{
    if (condition) return;
    std::cerr << "messaging test failed: " << message << '\n';
    std::exit(1);
}

class Receiver final : public squared::messaging::Telegraph {
public:
    explicit Receiver(
        std::function<bool(const Telegram&)> handler
    )
        : handler_(std::move(handler))
    {
    }

    bool handle_message(const Telegram& telegram) override
    {
        return handler_(telegram);
    }

private:
    std::function<bool(const Telegram&)> handler_;
};

class StateProvider final
    : public squared::messaging::TelegramProvider {
public:
    explicit StateProvider(
        std::function<
            squared::messaging::ProviderResult(
                const squared::messaging::MessageId&
            )
        > provider
    )
        : provider_(std::move(provider))
    {
    }

    squared::messaging::ProviderResult provide(
        const squared::messaging::MessageId& message
    ) override
    {
        ++calls;
        return provider_(message);
    }

    int calls{0};

private:
    std::function<
        squared::messaging::ProviderResult(
            const squared::messaging::MessageId&
        )
    > provider_;
};

std::string receipt_status(const Telegram& telegram)
{
    const auto* status = telegram.payload().find("status");
    if (!status || !status->string_if()) return {};
    return *status->string_if();
}

}  // namespace

int main()
{
    using namespace squared::messaging;

    require(!MessageId("toggle").valid(), "bare message id rejected");
    require(
        MessageId("sample.toggle").valid(),
        "namespaced message id accepted"
    );
    require(
        !EndpointId("../sample").valid(),
        "invalid endpoint characters rejected"
    );

    squared::time::ManualTimepiece time;
    MessageDispatcher dispatcher(
        time,
        {
            .pending_capacity = 8,
            .subscription_capacity = 8,
            .maximum_deliveries_per_update = 8
        }
    );

    std::vector<std::string> delivery_order;
    Receiver directed([&delivery_order](const Telegram&) {
        delivery_order.emplace_back("directed");
        return true;
    });
    auto endpoint = dispatcher.register_endpoint(
        EndpointId("sample.receiver"),
        directed
    );
    require(static_cast<bool>(endpoint), "endpoint registers");
    require(endpoint.subscription.active(), "endpoint handle active");

    auto queued = dispatcher.send(Telegram(
        MessageId("sample.toggle"),
        {},
        EndpointId("sample.sender"),
        EndpointId("sample.receiver")
    ));
    require(static_cast<bool>(queued), "ordinary send queues");
    require(delivery_order.empty(), "queued send is not recursive");
    require(dispatcher.update() == 1, "queued send delivers on update");
    require(
        delivery_order == std::vector<std::string>{"directed"},
        "directed endpoint receives"
    );

    Receiver first([&delivery_order](const Telegram&) {
        delivery_order.emplace_back("first");
        return false;
    });
    Receiver second([&delivery_order](const Telegram&) {
        delivery_order.emplace_back("second");
        return true;
    });
    auto first_subscription = dispatcher.subscribe(
        MessageId("sample.broadcast"),
        first
    );
    auto second_subscription = dispatcher.subscribe(
        MessageId("sample.broadcast"),
        second
    );
    require(
        first_subscription && second_subscription,
        "broadcast subscriptions register"
    );
    const auto immediate = dispatcher.send_now(Telegram(
        MessageId("sample.broadcast")
    ));
    require(immediate.accepted, "immediate send accepted");
    require(immediate.receiver_count == 2, "broadcast reaches both");
    require(immediate.handled_count == 1, "handled count aggregates");
    require(
        delivery_order ==
            std::vector<std::string>({
                "directed",
                "first",
                "second"
            }),
        "broadcast preserves subscription order"
    );

    auto delayed = dispatcher.schedule(
        2s,
        Telegram(
            MessageId("sample.delayed"),
            {},
            EndpointId("sample.sender"),
            EndpointId("sample.receiver")
        )
    );
    require(static_cast<bool>(delayed), "delayed Telegram schedules");
    time.advance(1s);
    require(dispatcher.update() == 0, "delay uses domain time");
    time.advance(1s);
    require(dispatcher.update() == 1, "due Telegram delivers");

    std::vector<std::string> receipts;
    Receiver sender([&receipts](const Telegram& telegram) {
        if (telegram.message() == receipt_message_id()) {
            receipts.push_back(receipt_status(telegram));
            return true;
        }
        return false;
    });
    auto sender_endpoint = dispatcher.register_endpoint(
        EndpointId("sample.sender"),
        sender
    );
    require(
        static_cast<bool>(sender_endpoint),
        "receipt endpoint registers"
    );

    auto with_receipt = dispatcher.send(Telegram(
        MessageId("sample.receipted"),
        {},
        EndpointId("sample.sender"),
        EndpointId("sample.receiver"),
        41,
        true
    ));
    require(
        static_cast<bool>(with_receipt),
        "receipted Telegram queues"
    );
    require(
        dispatcher.update() == 2,
        "delivery and separate queued receipt dispatch"
    );
    require(
        receipts == std::vector<std::string>{"handled"},
        "handled receipt delivered"
    );

    auto cancelled = dispatcher.schedule(
        5s,
        Telegram(
            MessageId("sample.cancelled"),
            {},
            EndpointId("sample.sender"),
            EndpointId("sample.receiver"),
            42,
            true
        )
    );
    require(
        static_cast<bool>(cancelled),
        "cancellable Telegram schedules"
    );
    require(
        dispatcher.cancel(cancelled.handle),
        "pending Telegram cancels"
    );
    require(
        dispatcher.update() == 1,
        "cancel receipt is independently queued"
    );
    require(
        receipts ==
            std::vector<std::string>({"handled", "cancelled"}),
        "cancelled receipt delivered"
    );

    endpoint.subscription.reset();
    const auto unavailable = dispatcher.send_now(Telegram(
        MessageId("sample.missing"),
        {},
        std::nullopt,
        EndpointId("sample.receiver")
    ));
    require(
        unavailable.status == ReceiptStatus::ReceiverUnavailable,
        "missing endpoint is reported"
    );

    MessageDispatcher provider_dispatcher(
        time,
        {
            .pending_capacity = 8,
            .subscription_capacity = 8,
            .maximum_deliveries_per_update = 8
        }
    );
    std::uint64_t current_state = 7;
    StateProvider provider(
        [&current_state](const MessageId&) {
            return ProviderResult::provided(
                squared::data::JsonValue(current_state)
            );
        }
    );
    auto provider_registration =
        provider_dispatcher.register_provider(
            MessageId("sample.current-state"),
            EndpointId("sample.state-provider"),
            provider
        );
    require(
        static_cast<bool>(provider_registration),
        "authoritative provider registers"
    );
    require(
        provider_dispatcher.register_provider(
            MessageId("sample.current-state"),
            EndpointId("sample.other-provider"),
            provider
        ).status == SubscriptionStatus::AlreadyRegistered,
        "one provider owns a message kind"
    );

    std::vector<std::uint64_t> first_states;
    Receiver first_state(
        [&first_states](const Telegram& telegram) {
            const auto* value =
                telegram.payload().unsigned_integer_if();
            if (!value) return false;
            first_states.push_back(*value);
            return true;
        }
    );
    auto first_state_subscription =
        provider_dispatcher.subscribe(
            MessageId("sample.current-state"),
            first_state
        );
    require(
        static_cast<bool>(first_state_subscription),
        "subscriber with provider registers"
    );
    require(provider.calls == 1, "provider called at subscription");
    require(
        first_states.empty(),
        "provider state enters ordinary queue"
    );
    require(
        provider_dispatcher.update() == 1,
        "provider state delivers on update"
    );
    require(
        first_states == std::vector<std::uint64_t>{7},
        "first subscriber receives current state"
    );

    current_state = 9;
    std::vector<std::uint64_t> second_states;
    Receiver second_state(
        [&second_states](const Telegram& telegram) {
            const auto* value =
                telegram.payload().unsigned_integer_if();
            if (!value) return false;
            second_states.push_back(*value);
            return true;
        }
    );
    auto second_state_subscription =
        provider_dispatcher.subscribe(
            MessageId("sample.current-state"),
            second_state
        );
    require(
        static_cast<bool>(second_state_subscription),
        "second subscriber registers"
    );
    require(provider.calls == 2, "provider regenerates current state");
    require(
        provider_dispatcher.update() == 1,
        "second provider result delivers once"
    );
    require(
        first_states == std::vector<std::uint64_t>{7} &&
            second_states == std::vector<std::uint64_t>{9},
        "provider result targets only the new subscriber"
    );

    int transient_deliveries = 0;
    Receiver transient_state(
        [&transient_deliveries](const Telegram&) {
            ++transient_deliveries;
            return true;
        }
    );
    auto transient_subscription =
        provider_dispatcher.subscribe(
            MessageId("sample.current-state"),
            transient_state
        );
    require(
        static_cast<bool>(transient_subscription) &&
            provider.calls == 3,
        "provider creates state for transient subscriber"
    );
    transient_subscription.subscription.reset();
    require(
        provider_dispatcher.update() == 1 &&
            transient_deliveries == 0,
        "queued provider state observes subscription lifetime"
    );

    StateProvider no_state([](const MessageId&) {
        return ProviderResult::no_current_state();
    });
    auto no_state_registration =
        provider_dispatcher.register_provider(
            MessageId("sample.optional-state"),
            EndpointId("sample.optional-provider"),
            no_state
        );
    Receiver optional_receiver([](const Telegram&) {
        return true;
    });
    auto optional_subscription = provider_dispatcher.subscribe(
        MessageId("sample.optional-state"),
        optional_receiver
    );
    require(
        no_state_registration && optional_subscription,
        "no-current-state still permits subscription"
    );
    require(
        provider_dispatcher.pending_count() == 0,
        "no-current-state queues nothing"
    );

    StateProvider failing([](const MessageId&) {
        return ProviderResult::failed("state unavailable");
    });
    auto failing_registration =
        provider_dispatcher.register_provider(
            MessageId("sample.failed-state"),
            EndpointId("sample.failed-provider"),
            failing
        );
    Receiver failed_receiver([](const Telegram&) {
        return true;
    });
    const auto failed_subscription =
        provider_dispatcher.subscribe(
            MessageId("sample.failed-state"),
            failed_receiver
        );
    require(
        static_cast<bool>(failing_registration) &&
            failed_subscription.status ==
                SubscriptionStatus::ProviderFailed &&
            failed_subscription.detail == "state unavailable",
        "provider failure rejects subscription explicitly"
    );

    MessageDispatcher provider_full(
        time,
        {
            .pending_capacity = 1,
            .subscription_capacity = 4,
            .maximum_deliveries_per_update = 4
        }
    );
    StateProvider full_provider([](const MessageId&) {
        return ProviderResult::provided(
            squared::data::JsonValue(true)
        );
    });
    auto full_provider_registration =
        provider_full.register_provider(
            MessageId("sample.full-state"),
            EndpointId("sample.full-provider"),
            full_provider
        );
    require(
        full_provider_registration &&
            provider_full.send(
                Telegram(MessageId("sample.queue-occupant"))
            ),
        "provider queue-full fixture initializes"
    );
    Receiver full_receiver([](const Telegram&) {
        return true;
    });
    const auto full_subscription = provider_full.subscribe(
        MessageId("sample.full-state"),
        full_receiver
    );
    require(
        full_subscription.status ==
            SubscriptionStatus::InitialStateQueueFull &&
            !full_subscription.subscription.active(),
        "initial-state overflow rejects subscription atomically"
    );

    squared::time::ManualTimepiece snapshot_time;
    MessageDispatcher snapshot_source(
        snapshot_time,
        {
            .pending_capacity = 4,
            .subscription_capacity = 4,
            .maximum_deliveries_per_update = 4
        }
    );
    const auto later_snapshot_message = snapshot_source.schedule(
        3s,
        Telegram(
            MessageId("sample.snapshot.later"),
            squared::data::JsonValue("later"),
            EndpointId("sample.snapshot.sender"),
            EndpointId("sample.snapshot.receiver")
        )
    );
    const auto sooner_snapshot_message = snapshot_source.schedule(
        1s,
        Telegram(
            MessageId("sample.snapshot.sooner"),
            squared::data::JsonValue(std::uint64_t{42})
        )
    );
    require(
        later_snapshot_message && sooner_snapshot_message,
        "snapshot fixture queues"
    );
    snapshot_time.advance(500ms);
    std::vector<std::string> inspected;
    std::vector<squared::time::Duration> remaining;
    snapshot_source.inspect_pending(
        [&inspected, &remaining](
            const PendingMessageView& pending
        ) {
            inspected.emplace_back(
                pending.telegram.message().value()
            );
            remaining.push_back(pending.remaining_delay);
            require(
                static_cast<bool>(pending.handle),
                "inspection exposes stable handle"
            );
            require(
                !pending.subscription_targeted,
                "ordinary snapshot entries are not targeted"
            );
        }
    );
    require(
        inspected == std::vector<std::string>({
            "sample.snapshot.sooner",
            "sample.snapshot.later"
        }) &&
            remaining == std::vector<squared::time::Duration>({
                500ms,
                2500ms
            }),
        "inspection is ordered and uses remaining delay"
    );

    const auto pending_snapshot = snapshot_source.snapshot_pending();
    require(
        static_cast<bool>(pending_snapshot),
        "pending queue snapshots"
    );
    const auto encoded_snapshot =
        squared::data::write_json(pending_snapshot.snapshot);
    require(
        static_cast<bool>(encoded_snapshot) &&
            encoded_snapshot.text.find(
                "\"remainingNanoseconds\":500000000"
            ) != std::string::npos,
        "snapshot serializes deterministic relative time"
    );
    const auto decoded_snapshot =
        squared::data::parse_json(encoded_snapshot.text);
    require(
        static_cast<bool>(decoded_snapshot),
        "snapshot JSON parses"
    );

    MessageDispatcher restored_dispatcher(
        snapshot_time,
        {
            .pending_capacity = 4,
            .subscription_capacity = 4,
            .maximum_deliveries_per_update = 4
        }
    );
    std::vector<std::string> restored_delivery;
    Receiver restored_endpoint(
        [&restored_delivery](const Telegram& telegram) {
            restored_delivery.emplace_back(
                telegram.message().value()
            );
            return true;
        }
    );
    Receiver restored_broadcast(
        [&restored_delivery](const Telegram& telegram) {
            restored_delivery.emplace_back(
                telegram.message().value()
            );
            return true;
        }
    );
    auto restored_endpoint_registration =
        restored_dispatcher.register_endpoint(
            EndpointId("sample.snapshot.receiver"),
            restored_endpoint
        );
    auto restored_broadcast_subscription =
        restored_dispatcher.subscribe(
            MessageId("sample.snapshot.sooner"),
            restored_broadcast
        );
    const auto restore_result =
        restored_dispatcher.restore_pending(
            decoded_snapshot.value
        );
    require(
        restored_endpoint_registration &&
            restored_broadcast_subscription &&
            restore_result &&
            restore_result.restored_count == 2,
        "snapshot restores atomically"
    );
    require(
        restored_dispatcher.restore_pending(
            decoded_snapshot.value
        ).status == PendingRestoreStatus::DispatcherNotEmpty,
        "restoration refuses ambiguous append"
    );
    snapshot_time.advance(500ms);
    require(
        restored_dispatcher.update() == 1 &&
            restored_delivery ==
                std::vector<std::string>{
                    "sample.snapshot.sooner"
                },
        "restored nearer message preserves remaining delay"
    );
    snapshot_time.advance(2s);
    require(
        restored_dispatcher.update() == 1 &&
            restored_delivery ==
                std::vector<std::string>({
                    "sample.snapshot.sooner",
                    "sample.snapshot.later"
                }),
        "restored delivery order is deterministic"
    );

    MessageDispatcher too_small_restore(
        snapshot_time,
        {
            .pending_capacity = 1,
            .subscription_capacity = 1,
            .maximum_deliveries_per_update = 1
        }
    );
    require(
        too_small_restore.restore_pending(
            decoded_snapshot.value
        ).status == PendingRestoreStatus::CapacityReached &&
            too_small_restore.pending_count() == 0,
        "oversized snapshot changes no queue state"
    );

    auto malformed_snapshot = decoded_snapshot.value;
    auto* malformed_messages =
        malformed_snapshot.find("messages")->array_if();
    malformed_messages->at(1).object_if()->erase("message");
    MessageDispatcher malformed_restore(
        snapshot_time,
        {
            .pending_capacity = 4,
            .subscription_capacity = 1,
            .maximum_deliveries_per_update = 1
        }
    );
    require(
        malformed_restore.restore_pending(
            malformed_snapshot
        ).status == PendingRestoreStatus::InvalidSnapshot &&
            malformed_restore.pending_count() == 0,
        "invalid later entry cannot partially restore"
    );

    Receiver targeted_snapshot_receiver([](const Telegram&) {
        return true;
    });
    auto targeted_snapshot_subscription =
        provider_dispatcher.subscribe(
            MessageId("sample.current-state"),
            targeted_snapshot_receiver
        );
    require(
        targeted_snapshot_subscription &&
            provider_dispatcher.snapshot_pending().status ==
                PendingSnapshotStatus::
                    SubscriptionTargetedDelivery,
        "provider-targeted state is explicitly nonpersistent"
    );
    static_cast<void>(provider_dispatcher.update());

    MessageDispatcher bounded(
        time,
        {
            .pending_capacity = 1,
            .subscription_capacity = 1,
            .maximum_deliveries_per_update = 1
        }
    );
    require(
        static_cast<bool>(
            bounded.send(Telegram(MessageId("sample.one")))
        ),
        "bounded queue accepts capacity"
    );
    require(
        bounded.send(Telegram(MessageId("sample.two"))).status ==
            DispatchStatus::QueueFull,
        "bounded queue reports overflow"
    );
    require(
        bounded.schedule(
            -1ns,
            Telegram(MessageId("sample.invalid-delay"))
        ).status == DispatchStatus::InvalidDelay,
        "negative delay rejected"
    );

    std::cout
        << "Squared Telegram and Telegraph messaging: OK\n";
    return 0;
}
