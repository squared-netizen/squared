#include <squared/messaging/message_dispatcher.hpp>

#include "detail/messaging_detail.hpp"
#include <squared/data/json.hpp>
#include <squared/messaging/delivery_report.hpp>
#include <squared/messaging/dispatch_handle.hpp>
#include <squared/messaging/dispatch_result.hpp>
#include <squared/messaging/dispatch_status.hpp>
#include <squared/messaging/endpoint_id.hpp>
#include <squared/messaging/message_dispatcher_config.hpp>
#include <squared/messaging/message_id.hpp>
#include <squared/messaging/pending_message_view.hpp>
#include <squared/messaging/pending_restore_result.hpp>
#include <squared/messaging/pending_restore_status.hpp>
#include <squared/messaging/pending_snapshot_result.hpp>
#include <squared/messaging/pending_snapshot_status.hpp>
#include <squared/messaging/provider_result.hpp>
#include <squared/messaging/provider_status.hpp>
#include <squared/messaging/receipt.hpp>
#include <squared/messaging/receipt_status.hpp>
#include <squared/messaging/subscription.hpp>
#include <squared/messaging/subscription_result.hpp>
#include <squared/messaging/subscription_status.hpp>
#include <squared/messaging/telegram.hpp>
#include <squared/messaging/telegram_provider.hpp>
#include <squared/messaging/telegraph.hpp>
#include <squared/time/clock.hpp>
#include <squared/time/deadline_queue.hpp>
#include <squared/time/duration.hpp>
#include <squared/time/time_point.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace sq::messaging {

namespace {

const EndpointId& dispatcher_endpoint_id()
{
    static const EndpointId value("squared.messaging.dispatcher");
    return value;
}

}  // namespace

MessageDispatcher::MessageDispatcher(
    const sq::time::Clock& clock,
    MessageDispatcherConfig config
)
    : clock_(clock),
      config_(config),
      registry_(
          std::make_shared<Subscription::Registry>(
              config.subscription_capacity
          )
      ),
      pending_(config.pending_capacity)
{
}

MessageDispatcher::~MessageDispatcher() = default;

SubscriptionResult MessageDispatcher::register_endpoint(
    EndpointId endpoint,
    Telegraph& telegraph
)
{
    if (!endpoint.valid()) {
        return {SubscriptionStatus::InvalidId, {}, {}};
    }
    for (const auto& binding : registry_->bindings) {
        if (binding.kind ==
                Subscription::Registry::Kind::Endpoint &&
            binding.endpoint == endpoint) {
            return {
                SubscriptionStatus::AlreadyRegistered,
                {},
                {}
            };
        }
    }
    if (registry_->bindings.size() >= registry_->capacity) {
        return {SubscriptionStatus::CapacityReached, {}, {}};
    }
    if (registry_->next_token ==
        std::numeric_limits<std::uint64_t>::max()) {
        return {SubscriptionStatus::HandleExhausted, {}, {}};
    }

    const std::uint64_t token = ++registry_->next_token;
    registry_->bindings.push_back({
        token,
        Subscription::Registry::Kind::Endpoint,
        std::move(endpoint),
        MessageId{},
        &telegraph,
        nullptr
    });
    return {
        SubscriptionStatus::Registered,
        Subscription(registry_, token),
        {}
    };
}

SubscriptionResult MessageDispatcher::subscribe(
    MessageId message,
    Telegraph& telegraph
)
{
    if (!message.valid()) {
        return {SubscriptionStatus::InvalidId, {}, {}};
    }
    for (const auto& binding : registry_->bindings) {
        if (binding.kind ==
                Subscription::Registry::Kind::Broadcast &&
            binding.message == message &&
            binding.telegraph == &telegraph) {
            return {
                SubscriptionStatus::AlreadyRegistered,
                {},
                {}
            };
        }
    }
    if (registry_->bindings.size() >= registry_->capacity) {
        return {SubscriptionStatus::CapacityReached, {}, {}};
    }
    if (registry_->next_token ==
        std::numeric_limits<std::uint64_t>::max()) {
        return {SubscriptionStatus::HandleExhausted, {}, {}};
    }

    TelegramProvider* provider = nullptr;
    EndpointId provider_endpoint;
    for (const auto& binding : registry_->bindings) {
        if (binding.kind ==
                Subscription::Registry::Kind::Provider &&
            binding.message == message) {
            provider = binding.provider;
            provider_endpoint = binding.endpoint;
            break;
        }
    }

    ProviderResult provided;
    if (provider) {
        provided = provider->provide(message);
        if (provided.status == ProviderStatus::Failed) {
            return {
                SubscriptionStatus::ProviderFailed,
                {},
                std::move(provided.detail)
            };
        }
    }

    // A provider executes synchronously and may use the dispatcher itself.
    // Recheck mutable registry constraints before committing the subscriber.
    for (const auto& binding : registry_->bindings) {
        if (binding.kind ==
                Subscription::Registry::Kind::Broadcast &&
            binding.message == message &&
            binding.telegraph == &telegraph) {
            return {
                SubscriptionStatus::AlreadyRegistered,
                {},
                {}
            };
        }
    }
    if (registry_->bindings.size() >= registry_->capacity) {
        return {SubscriptionStatus::CapacityReached, {}, {}};
    }
    if (registry_->next_token ==
        std::numeric_limits<std::uint64_t>::max()) {
        return {SubscriptionStatus::HandleExhausted, {}, {}};
    }

    const std::uint64_t token = ++registry_->next_token;
    registry_->bindings.push_back({
        token,
        Subscription::Registry::Kind::Broadcast,
        EndpointId{},
        std::move(message),
        &telegraph,
        nullptr
    });

    if (provider &&
        provided.status == ProviderStatus::Provided) {
        const MessageId state_message =
            registry_->bindings.back().message;
        const auto queued = enqueue_at(
            clock_.now(),
            Telegram(
                state_message,
                std::move(provided.payload),
                provider_endpoint
            ),
            token
        );
        if (!queued) {
            registry_->remove(token);
            const SubscriptionStatus status =
                queued.status == DispatchStatus::QueueFull
                    ? SubscriptionStatus::InitialStateQueueFull
                    : (
                        queued.status ==
                            DispatchStatus::HandleExhausted
                            ? SubscriptionStatus::HandleExhausted
                            : SubscriptionStatus::ProviderFailed
                    );
            return {
                status,
                {},
                "authoritative initial state could not be queued"
            };
        }
    }

    return {
        SubscriptionStatus::Registered,
        Subscription(registry_, token),
        {}
    };
}

SubscriptionResult MessageDispatcher::register_provider(
    MessageId message,
    EndpointId provider_endpoint,
    TelegramProvider& provider
)
{
    if (!message.valid() || !provider_endpoint.valid()) {
        return {SubscriptionStatus::InvalidId, {}, {}};
    }
    for (const auto& binding : registry_->bindings) {
        if (binding.kind ==
                Subscription::Registry::Kind::Provider &&
            binding.message == message) {
            return {
                SubscriptionStatus::AlreadyRegistered,
                {},
                {}
            };
        }
    }
    if (registry_->bindings.size() >= registry_->capacity) {
        return {SubscriptionStatus::CapacityReached, {}, {}};
    }
    if (registry_->next_token ==
        std::numeric_limits<std::uint64_t>::max()) {
        return {SubscriptionStatus::HandleExhausted, {}, {}};
    }

    const std::uint64_t token = ++registry_->next_token;
    registry_->bindings.push_back({
        token,
        Subscription::Registry::Kind::Provider,
        std::move(provider_endpoint),
        std::move(message),
        nullptr,
        &provider
    });
    return {
        SubscriptionStatus::Registered,
        Subscription(registry_, token),
        {}
    };
}

DispatchResult MessageDispatcher::send(Telegram telegram)
{
    return enqueue_at(clock_.now(), std::move(telegram));
}

DispatchResult MessageDispatcher::schedule(
    sq::time::Duration delay,
    Telegram telegram
)
{
    if (delay.count() < 0) {
        return {DispatchStatus::InvalidDelay, {}};
    }
    const auto current = clock_.now();
    const auto maximum =
        std::numeric_limits<sq::time::Duration::rep>::max();
    if (current.count() < 0 ||
        delay.count() > maximum - current.count()) {
        return {DispatchStatus::InvalidDelay, {}};
    }
    return enqueue_at(current + delay, std::move(telegram));
}

DispatchResult MessageDispatcher::enqueue_at(
    sq::time::TimePoint due,
    Telegram telegram,
    std::optional<std::uint64_t> subscription_target
)
{
    if (!telegram.valid()) {
        return {DispatchStatus::InvalidTelegram, {}};
    }
    sq::time::DeadlineQueue<PendingDelivery>::Ticket ticket;
    const auto result = pending_.schedule_at(
        due,
        PendingDelivery{
            std::move(telegram),
            subscription_target
        },
        &ticket
    );
    using QueueResult =
        sq::time::DeadlineQueue<
            PendingDelivery
        >::ScheduleResult;
    switch (result) {
    case QueueResult::Scheduled:
        return {
            DispatchStatus::Queued,
            DispatchHandle{ticket.value}
        };
    case QueueResult::QueueFull:
        return {DispatchStatus::QueueFull, {}};
    case QueueResult::InvalidTime:
        return {DispatchStatus::InvalidDelay, {}};
    case QueueResult::TicketExhausted:
        return {DispatchStatus::HandleExhausted, {}};
    }
    return {DispatchStatus::InvalidTelegram, {}};
}

DeliveryReport MessageDispatcher::send_now(
    const Telegram& telegram
)
{
    if (!telegram.valid()) return {};
    return deliver(telegram, clock_.now(), std::nullopt);
}

bool MessageDispatcher::cancel(DispatchHandle handle)
{
    const auto captured_now = clock_.now();
    return pending_.cancel(
        {handle.value},
        [this, captured_now](
            const PendingDelivery& delivery
        ) {
            static_cast<void>(
                queue_receipt(
                    delivery.telegram,
                    ReceiptStatus::Cancelled,
                    captured_now
                )
            );
        }
    );
}

std::size_t MessageDispatcher::update()
{
    const auto captured_now = clock_.now();
    return pending_.poll_due(
        captured_now,
        config_.maximum_deliveries_per_update,
        [this, captured_now](PendingDelivery delivery) {
            static_cast<void>(
                deliver(
                    delivery.telegram,
                    captured_now,
                    delivery.subscription_target
                )
            );
        }
    );
}

std::size_t MessageDispatcher::pending_count() const noexcept
{
    return pending_.size();
}

std::size_t MessageDispatcher::pending_capacity() const noexcept
{
    return pending_.capacity();
}

void MessageDispatcher::inspect_pending_erased(
    void* context,
    ErasedPendingVisitor visitor
) const
{
    if (!visitor) return;
    const auto captured_now = clock_.now();
    pending_.visit_ordered(
        [context, visitor, captured_now](
            sq::time::TimePoint due,
            sq::time::DeadlineQueue<
                PendingDelivery
            >::Ticket ticket,
            const PendingDelivery& delivery
        ) {
            const auto remaining = due > captured_now
                ? due - captured_now
                : sq::time::Duration::zero();
            visitor(
                context,
                PendingMessageView{
                    DispatchHandle{ticket.value},
                    remaining,
                    delivery.telegram,
                    delivery.subscription_target.has_value()
                }
            );
        }
    );
}

PendingSnapshotResult MessageDispatcher::snapshot_pending() const
{
    sq::data::JsonValue::Array messages;
    messages.reserve(pending_.size());
    bool targeted = false;
    inspect_pending(
        [&messages, &targeted](const PendingMessageView& pending) {
            if (pending.subscription_targeted) {
                targeted = true;
                return;
            }
            sq::data::JsonValue::Object entry;
            entry.emplace(
                "remainingNanoseconds",
                sq::data::JsonValue(
                    static_cast<std::uint64_t>(
                        pending.remaining_delay.count()
                    )
                )
            );
            entry.emplace(
                "message",
                sq::data::JsonValue(
                    pending.telegram.message().value()
                )
            );
            entry.emplace(
                "payload",
                pending.telegram.payload()
            );
            entry.emplace(
                "sender",
                pending.telegram.sender()
                    ? sq::data::JsonValue(
                        pending.telegram.sender()->value()
                    )
                    : sq::data::JsonValue()
            );
            entry.emplace(
                "receiver",
                pending.telegram.receiver()
                    ? sq::data::JsonValue(
                        pending.telegram.receiver()->value()
                    )
                    : sq::data::JsonValue()
            );
            entry.emplace(
                "correlation",
                sq::data::JsonValue(
                    pending.telegram.correlation()
                )
            );
            entry.emplace(
                "receiptRequested",
                sq::data::JsonValue(
                    pending.telegram.receipt_requested()
                )
            );
            messages.emplace_back(std::move(entry));
        }
    );
    if (targeted) {
        return {
            PendingSnapshotStatus::SubscriptionTargetedDelivery,
            {},
            "subscription-targeted provider state must be delivered "
            "before snapshot"
        };
    }

    sq::data::JsonValue::Object root;
    root.emplace(
        "schema",
        sq::data::JsonValue(
            "squared.messaging.pending"
        )
    );
    root.emplace(
        "version",
        sq::data::JsonValue(std::uint64_t{1})
    );
    root.emplace(
        "messages",
        sq::data::JsonValue(std::move(messages))
    );
    return {
        PendingSnapshotStatus::Ready,
        sq::data::JsonValue(std::move(root)),
        {}
    };
}

PendingRestoreResult MessageDispatcher::restore_pending(
    const sq::data::JsonValue& snapshot
)
{
    if (!pending_.empty()) {
        return {
            PendingRestoreStatus::DispatcherNotEmpty,
            0,
            "pending queue must be empty before restoration"
        };
    }

    const auto* root = snapshot.object_if();
    const auto* schema_value = snapshot.find("schema");
    const auto* version_value = snapshot.find("version");
    const auto* messages_value = snapshot.find("messages");
    const auto* schema = schema_value
        ? schema_value->string_if()
        : nullptr;
    const auto* version = version_value
        ? version_value->unsigned_integer_if()
        : nullptr;
    const auto* messages = messages_value
        ? messages_value->array_if()
        : nullptr;
    if (!root || root->size() != 3 ||
        !schema || *schema != "squared.messaging.pending" ||
        !version || *version != 1 || !messages) {
        return {
            PendingRestoreStatus::InvalidSnapshot,
            0,
            "snapshot root or version is invalid"
        };
    }
    if (messages->size() > pending_.capacity()) {
        return {
            PendingRestoreStatus::CapacityReached,
            0,
            "snapshot exceeds pending queue capacity"
        };
    }

    struct RestoredEntry {
        sq::time::Duration delay;
        Telegram telegram;
    };
    std::vector<RestoredEntry> restored;
    restored.reserve(messages->size());
    const auto captured_now = clock_.now();
    const auto maximum =
        std::numeric_limits<sq::time::Duration::rep>::max();

    for (const auto& encoded : *messages) {
        const auto* object = encoded.object_if();
        const auto* delay_value =
            encoded.find("remainingNanoseconds");
        const auto* message_value = encoded.find("message");
        const auto* payload_value = encoded.find("payload");
        const auto* sender_value = encoded.find("sender");
        const auto* receiver_value = encoded.find("receiver");
        const auto* correlation_value =
            encoded.find("correlation");
        const auto* receipt_value =
            encoded.find("receiptRequested");
        const auto* delay = delay_value
            ? delay_value->unsigned_integer_if()
            : nullptr;
        const auto* message = message_value
            ? message_value->string_if()
            : nullptr;
        const auto* correlation = correlation_value
            ? correlation_value->unsigned_integer_if()
            : nullptr;
        const auto* receipt = receipt_value
            ? receipt_value->boolean_if()
            : nullptr;
        if (!object || object->size() != 7 ||
            !delay || !message || !payload_value ||
            !sender_value || !receiver_value ||
            !correlation || !receipt) {
            return {
                PendingRestoreStatus::InvalidSnapshot,
                0,
                "snapshot message schema is invalid"
            };
        }
        if (*delay >
                static_cast<std::uint64_t>(maximum) ||
            captured_now.count() < 0 ||
            static_cast<sq::time::Duration::rep>(*delay) >
                maximum - captured_now.count()) {
            return {
                PendingRestoreStatus::TimeOverflow,
                0,
                "remaining delay exceeds the time domain"
            };
        }

        const auto decode_endpoint = [](
            const sq::data::JsonValue& value,
            std::optional<EndpointId>& output
        ) {
            if (value.is_null()) {
                output.reset();
                return true;
            }
            const auto* text = value.string_if();
            if (!text) return false;
            EndpointId endpoint(*text);
            if (!endpoint.valid()) return false;
            output = std::move(endpoint);
            return true;
        };
        std::optional<EndpointId> sender;
        std::optional<EndpointId> receiver;
        if (!decode_endpoint(*sender_value, sender) ||
            !decode_endpoint(*receiver_value, receiver)) {
            return {
                PendingRestoreStatus::InvalidSnapshot,
                0,
                "snapshot endpoint is invalid"
            };
        }
        Telegram telegram(
            MessageId(*message),
            *payload_value,
            std::move(sender),
            std::move(receiver),
            *correlation,
            *receipt
        );
        if (!telegram.valid()) {
            return {
                PendingRestoreStatus::InvalidSnapshot,
                0,
                "snapshot Telegram is invalid"
            };
        }
        restored.push_back({
            sq::time::Duration(
                static_cast<sq::time::Duration::rep>(
                    *delay
                )
            ),
            std::move(telegram)
        });
    }

    std::vector<
        sq::time::DeadlineQueue<PendingDelivery>::Ticket
    > committed;
    committed.reserve(restored.size());
    for (auto& entry : restored) {
        sq::time::DeadlineQueue<
            PendingDelivery
        >::Ticket ticket;
        const auto result = pending_.schedule_at(
            captured_now + entry.delay,
            PendingDelivery{
                std::move(entry.telegram),
                std::nullopt
            },
            &ticket
        );
        using QueueResult =
            sq::time::DeadlineQueue<
                PendingDelivery
            >::ScheduleResult;
        if (result != QueueResult::Scheduled) {
            for (const auto committed_ticket : committed) {
                static_cast<void>(
                    pending_.cancel(committed_ticket)
                );
            }
            const auto status =
                result == QueueResult::TicketExhausted
                    ? PendingRestoreStatus::HandleExhausted
                    : (
                        result == QueueResult::QueueFull
                            ? PendingRestoreStatus::CapacityReached
                            : PendingRestoreStatus::TimeOverflow
                    );
            return {
                status,
                0,
                "snapshot restoration could not be committed"
            };
        }
        committed.push_back(ticket);
    }
    return {
        PendingRestoreStatus::Restored,
        committed.size(),
        {}
    };
}

DeliveryReport MessageDispatcher::deliver(
    const Telegram& telegram,
    sq::time::TimePoint captured_now,
    std::optional<std::uint64_t> subscription_target
)
{
    DeliveryReport report;
    if (!telegram.valid()) return report;
    report.accepted = true;

    std::vector<Telegraph*> receivers;
    receivers.reserve(registry_->bindings.size());
    if (subscription_target) {
        for (const auto& binding : registry_->bindings) {
            if (binding.token == *subscription_target &&
                binding.kind ==
                    Subscription::Registry::Kind::Broadcast &&
                binding.message == telegram.message()) {
                receivers.push_back(binding.telegraph);
                break;
            }
        }
    } else if (telegram.receiver()) {
        for (const auto& binding : registry_->bindings) {
            if (binding.kind ==
                    Subscription::Registry::Kind::Endpoint &&
                binding.endpoint == *telegram.receiver()) {
                receivers.push_back(binding.telegraph);
                break;
            }
        }
    } else {
        for (const auto& binding : registry_->bindings) {
            if (binding.kind ==
                    Subscription::Registry::Kind::Broadcast &&
                binding.message == telegram.message()) {
                receivers.push_back(binding.telegraph);
            }
        }
    }

    report.receiver_count = receivers.size();
    for (Telegraph* receiver : receivers) {
        if (receiver && receiver->handle_message(telegram)) {
            ++report.handled_count;
        }
    }

    if (receivers.empty()) {
        report.status = ReceiptStatus::ReceiverUnavailable;
    } else if (report.handled_count > 0) {
        report.status = ReceiptStatus::Handled;
    } else {
        report.status = ReceiptStatus::Unhandled;
    }
    report.receipt_queued = queue_receipt(
        telegram,
        report.status,
        captured_now
    );
    return report;
}

bool MessageDispatcher::queue_receipt(
    const Telegram& original,
    ReceiptStatus status,
    sq::time::TimePoint captured_now
)
{
    if (!original.receipt_requested() ||
        !original.sender() ||
        original.correlation() == 0) {
        return false;
    }

    sq::data::JsonValue::Object payload;
    payload.emplace(
        "correlation",
        sq::data::JsonValue(original.correlation())
    );
    payload.emplace(
        "message",
        sq::data::JsonValue(
            std::string(original.message().value())
        )
    );
    payload.emplace(
        "status",
        sq::data::JsonValue(
            std::string(receipt_status_name(status))
        )
    );
    if (original.receiver()) {
        payload.emplace(
            "receiver",
            sq::data::JsonValue(
                std::string(original.receiver()->value())
            )
        );
    }

    Telegram receipt(
        receipt_message_id(),
        sq::data::JsonValue(std::move(payload)),
        dispatcher_endpoint_id(),
        *original.sender()
    );
    return enqueue_at(
        captured_now,
        std::move(receipt)
    ).status == DispatchStatus::Queued;
}

}  // namespace sq::messaging
