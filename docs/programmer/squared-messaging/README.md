# Squared Messaging — Programmer Guide

Squared Messaging provides deterministic Telegram and Telegraph messaging in
the style of libGDX-AI. A `MessageDispatcher` routes owned `Telegram`
envelopes to registered `Telegraph` receivers, supports immediate, queued, and
delayed delivery, broadcast and directed addressing, return receipts,
cancellation, pending inspection, and deterministic JSON snapshot and restore
of the pending queue. Delivery order and timing come from one explicit
Squared Time domain; JSON encoding comes from Squared Data.

## Package availability

| Module | Version | Requires |
| --- | --- | --- |
| `dev.squarednetizen.squared.messaging` | `0.6.0-dev.3` | `dev.squarednetizen.squared.data` `0.6.0-dev.2`, `dev.squarednetizen.squared.time` `0.6.0-dev.2` |

The CMake target is `squared_messaging`. There is no global dispatcher; each
`MessageDispatcher` borrows one read-only `squared::time::Clock`, so separate
application, simulation, UI, or editor-preview domains use separate
dispatchers.

## Public API overview

| Type | Header | Purpose |
| --- | --- | --- |
| `squared::messaging::MessageId` | `squared/messaging/telegram.hpp` | Validated namespaced message kind. |
| `squared::messaging::EndpointId` | `squared/messaging/telegram.hpp` | Validated namespaced endpoint address. |
| `squared::messaging::CorrelationId` | `squared/messaging/telegram.hpp` | Application-owned `std::uint64_t` correlation tag. |
| `squared::messaging::Telegram` | `squared/messaging/telegram.hpp` | Owned immutable-access message envelope. |
| `squared::messaging::ReceiptStatus` | `squared/messaging/telegram.hpp` | Delivery outcome category. |
| `receipt_message_id` / `receipt_status_name` | `squared/messaging/telegram.hpp` | Framework receipt message kind and status text. |
| `squared::messaging::Telegraph` | `squared/messaging/telegraph.hpp` | Receiver interface; `handle_message`. |
| `squared::messaging::TelegramProvider` | `squared/messaging/telegram_provider.hpp` | Authoritative current-state generator. |
| `squared::messaging::MessageDispatcher` | `squared/messaging/message_dispatcher.hpp` | Registration, routing, queuing, scheduling, cancellation, snapshot/restore. |
| `squared::messaging::Subscription` | `squared/messaging/message_dispatcher.hpp` | Move-only scoped registration handle. |
| `squared::messaging::SubscriptionResult` / `SubscriptionStatus` | `squared/messaging/message_dispatcher.hpp` | Registration outcome. |
| `squared::messaging::DispatchHandle` / `DispatchResult` / `DispatchStatus` | `squared/messaging/message_dispatcher.hpp` | Queue and schedule outcome. |
| `squared::messaging::DeliveryReport` | `squared/messaging/message_dispatcher.hpp` | Compressed immediate-delivery outcome. |
| `squared::messaging::MessageDispatcherConfig` | `squared/messaging/message_dispatcher.hpp` | Capacity and per-update delivery limits. |
| `squared::messaging::PendingMessageView` | `squared/messaging/message_dispatcher.hpp` | Non-owning pending-message inspection view. |
| `squared::messaging::PendingSnapshotResult` / `PendingRestoreResult` | `squared/messaging/message_dispatcher.hpp` | Snapshot and restore outcomes. |

## Identities and envelopes

`MessageId` and `EndpointId` are validated namespaced strings (not pointers or
process-local numeric addresses). A valid ID is 1-128 ASCII characters that
begin and end with a letter or digit, use only letters, digits, `.`, `/`, `:`,
`-`, and `_`, and contain at least one namespace separator (`.`, `/`, or
`:`). Construct with a `std::string`; `valid()` reports acceptance and
`value()` returns the text.

`Telegram` is the owned message envelope:

- `message()` — the `MessageId` kind used for subscription matching.
- `payload()` — an owned `squared::data::JsonValue`; receivers get immutable
  access only, so delivery never exposes a borrowed pointer.
- `sender()` / `receiver()` — optional `EndpointId` values. An absent
  receiver requests broadcast to subscribers of the message kind; a present
  receiver requests directed delivery to that endpoint.
- `correlation()` — an application-owned `std::uint64_t` echoed by receipts.
- `receipt_requested()` — requests a queued return receipt (requires a sender
  and a non-zero correlation to be valid).
- `directed()` — `true` for directed, `false` for broadcast delivery.
- `valid()` — `true` when the message kind and any endpoint IDs are valid and
  a receipt request carries a sender and non-zero correlation.

## Registration and dispatch

A `Telegraph` implements `handle_message(const Telegram&)` and returns `true`
when it handled the envelope. A `Subscription` is a move-only scoped handle:
destroying or `reset()`-ing it unregisters. The referenced `Telegraph` (or
`TelegramProvider`) must outlive its subscription.

```cpp
#include <squared/messaging/message_dispatcher.hpp>
#include <squared/time/timepiece.hpp>

#include <chrono>

using namespace std::chrono_literals;

int main()
{
    squared::time::Timepiece clock;
    squared::messaging::MessageDispatcher dispatcher(clock);

    struct Receiver final : squared::messaging::Telegraph {
        int handled{0};
        bool handle_message(const squared::messaging::Telegram&) override
        {
            ++handled;
            return true;
        }
    } receiver;

    auto endpoint = dispatcher.register_endpoint(
        squared::messaging::EndpointId("sample.receiver"), receiver
    );
    if (!endpoint) {
        return 1;  // InvalidId, AlreadyRegistered, CapacityReached, ...
    }

    auto queued = dispatcher.send(squared::messaging::Telegram(
        squared::messaging::MessageId("sample.toggle"),
        {},
        squared::messaging::EndpointId("sample.sender"),
        squared::messaging::EndpointId("sample.receiver")
    ));
    if (!queued) {
        return 2;  // InvalidTelegram, QueueFull, HandleExhausted
    }
    // Ordinary send queues; delivery happens on update(), never recursively.
    dispatcher.update();   // returns 1
    return 0;
}
```

- `send(Telegram)` queues delivery at the current domain time.
- `schedule(Duration delay, Telegram)` queues after a non-negative domain-time
  delay; the clock is read once to compute the deadline.
- `send_now(const Telegram&)` delivers synchronously for controlled internal
  use, bypassing the queue and the per-update cap.
- `update()` delivers due messages using one captured `clock.now()` value,
  draining at most `maximum_deliveries_per_update`, and returns the count
  delivered.
- Both `send` and `schedule` return `DispatchResult` with a `handle` that
  `cancel(DispatchHandle)` uses. `DispatchStatus` reports `Queued`,
  `InvalidTelegram`, `InvalidDelay`, `QueueFull`, or `HandleExhausted`.

### Broadcast and cancellation

A Telegram without a receiver is broadcast to all subscribers of its message
kind in subscription order. Cancelling a pending Telegram immediately
reclaims its queue slot; if the cancelled Telegram requested a receipt, a
`cancelled` receipt is queued separately.

```cpp
#include <squared/messaging/message_dispatcher.hpp>
#include <squared/time/timepiece.hpp>

#include <chrono>

using namespace std::chrono_literals;

int main()
{
    squared::time::Timepiece clock;
    squared::messaging::MessageDispatcher dispatcher(clock);

    struct Receiver final : squared::messaging::Telegraph {
        int calls{0};
        bool handle_message(const squared::messaging::Telegram&) override
        {
            ++calls;
            return true;
        }
    } first;

    auto broadcast = dispatcher.subscribe(
        squared::messaging::MessageId("sample.event"), first
    );
    if (!broadcast) {
        return 1;
    }

    auto pending = dispatcher.schedule(
        5s,
        squared::messaging::Telegram(
            squared::messaging::MessageId("sample.event")
        )
    );
    if (pending && dispatcher.cancel(pending.handle)) {
        // cancelled before delivery; queue slot reclaimed
    }
    dispatcher.send(squared::messaging::Telegram(
        squared::messaging::MessageId("sample.event")
    ));
    dispatcher.update();
    return 0;
}
```

## Receipts

A `Telegram` may request a return receipt by supplying a sender endpoint and a
non-zero correlation. The dispatcher queues a separate
`squared.messaging.receipt` `Telegram` addressed back to the sender whose
owned JSON payload carries `correlation`, `message`, `status`, and (for
directed mail) `receiver`. Status is one of `handled`, `unhandled`,
`receiver_unavailable`, or `cancelled`. Receipts never mutate or redispatch
the original envelope.

```cpp
#include <squared/messaging/message_dispatcher.hpp>
#include <squared/time/timepiece.hpp>

#include <string>

int main()
{
    squared::time::Timepiece clock;
    squared::messaging::MessageDispatcher dispatcher(clock);

    struct Receiver final : squared::messaging::Telegraph {
        explicit Receiver(std::string& status) : status_(status) {}
        bool handle_message(const squared::messaging::Telegram& telegram) override
        {
            if (telegram.message() ==
                squared::messaging::receipt_message_id()) {
                if (const auto* value =
                        telegram.payload().find("status")) {
                    status_ = *value->string_if();
                }
            }
            return true;
        }
    private:
        std::string& status_;
    };

    std::string observed_status;
    Receiver receiver{observed_status};
    Receiver sender{observed_status};

    auto endpoint = dispatcher.register_endpoint(
        squared::messaging::EndpointId("sample.receiver"), receiver
    );
    auto receipt_target = dispatcher.register_endpoint(
        squared::messaging::EndpointId("sample.sender"), sender
    );

    auto sent = dispatcher.send(squared::messaging::Telegram(
        squared::messaging::MessageId("sample.receipted"),
        {},
        squared::messaging::EndpointId("sample.sender"),
        squared::messaging::EndpointId("sample.receiver"),
        41,                 // correlation
        true                // receipt_requested
    ));
    dispatcher.update();    // delivery and the queued receipt both dispatch
    // observed_status == "handled"
    return 0;
}
```

The receipt is delivered to the endpoint registered for the original sender
address; register a `Telegraph` there to observe the `handled`/`unhandled`/
`receiver_unavailable`/`cancelled` status from the payload.

## Authoritative state on subscription

`register_provider(message, provider_endpoint, provider)` installs at most
one `TelegramProvider` per message kind. When a new broadcast subscriber
registers for that kind, the provider is called synchronously and returns one
of `ProviderResult::provided(value)`, `ProviderResult::no_current_state()`, or
`ProviderResult::failed(reason)`:

- `Provided` — the owned JSON value becomes a fresh queued `Telegram`
  addressed only to the new subscriber, and it follows ordinary queue ordering
  and delivery limits.
- `NoCurrentState` — the subscription proceeds with no initial delivery.
- `Failed` — the subscription is rejected atomically with `detail` as the
  diagnostic.

Provider failure, exhausted subscription handles, or a full initial-state
queue reject the subscription atomically: no partially active subscriber is
left behind. The provider object must remain alive while its registration
handle is active. Existing subscribers never receive the fresh provider state.

## Configuration

`MessageDispatcherConfig` bounds every dispatcher queue:

| Entry | Default | Meaning |
| --- | --- | --- |
| `pending_capacity` | 1024 | Maximum queued Telegrams; must be at least one. |
| `subscription_capacity` | 256 | Maximum simultaneous registrations; at least one. |
| `maximum_deliveries_per_update` | 256 | Maximum deliveries drained per `update()`; at least one. |

Overflow and invalid input are reported by result status, never silently
dropped. `pending_count()` and `pending_capacity()` report the current load.

## Pending inspection and snapshot/restore

`inspect_pending(visitor)` visits pending entries in delivery order without
copying Telegrams or payloads. Each `PendingMessageView` exposes the stable
`handle`, the remaining domain-time `remaining_delay`, the immutable
`telegram`, and whether the delivery targets a specific live subscription
(`subscription_targeted`). Views are valid only for the visitor call.

`snapshot_pending()` produces deterministic versioned JSON (schema
`squared.messaging.pending`, version 1) encoding each pending entry's
remaining nanoseconds, stable message and endpoint names, owned payload,
correlation, and receipt request. It stores no time points, pointers,
handles, or subscription tokens. It refuses (`PendingSnapshotStatus::
SubscriptionTargetedDelivery`) when a subscription-targeted provider delivery
is pending, because that state is not portable; run `update()` once to
deliver such startup state before checkpointing.

`restore_pending(snapshot)` is transactional and accepts only an empty queue.
The complete document is validated against the version-1 schema and the
queue capacity before anything is committed; equal deadlines retain snapshot
array order. Failures (`InvalidSnapshot`, `CapacityReached`, `TimeOverflow`,
`HandleExhausted`, `DispatcherNotEmpty`) leave the queue unchanged.

```cpp
#include <squared/messaging/message_dispatcher.hpp>
#include <squared/time/timepiece.hpp>

int main()
{
    squared::time::ManualTimepiece time;
    squared::messaging::MessageDispatcher source(time);

    source.schedule(
        std::chrono::seconds{3},
        squared::messaging::Telegram(
            squared::messaging::MessageId("sample.later"),
            squared::data::JsonValue{"payload"}
        )
    );
    const auto snapshot = source.snapshot_pending();
    if (!snapshot) {
        return 1;
    }

    const auto encoded = squared::data::write_json(snapshot.snapshot);
    const auto decoded = squared::data::parse_json(encoded.text);

    squared::messaging::MessageDispatcher restored(time);
    const auto result = restored.restore_pending(decoded.value);
    if (!result || result.restored_count != 1) {
        return 2;
    }
    time.advance(std::chrono::seconds{3});
    restored.update();   // delivers the restored Telegram
    return 0;
}
```

## Errors and failure behavior

- Registration (`register_endpoint`, `subscribe`, `register_provider`)
  returns `SubscriptionResult` with `SubscriptionStatus`: `Registered`,
  `InvalidId`, `AlreadyRegistered`, `CapacityReached`, `ProviderFailed`,
  `InitialStateQueueFull`, or `HandleExhausted`.
- Sending returns `DispatchResult` with `DispatchStatus`: `Queued`,
  `InvalidTelegram`, `InvalidDelay`, `QueueFull`, or `HandleExhausted`.
- `send_now` returns `DeliveryReport` (`accepted`, `status`, counts,
  `receipt_queued`); an unvalidated Telegram reports `accepted == false`.
- `cancel(handle)` returns `false` when the handle does not reference a
  pending delivery.
- Snapshot and restore return dedicated `PendingSnapshotResult` and
  `PendingRestoreResult` structs. A `Telegram` that is invalid (bad message
  kind, invalid endpoint, receipt request without sender/correlation) is
  refused at dispatch time.

## Ownership and lifetime

- `Telegram` and payloads are owned values (Squared Data `JsonValue`);
  delivery never shares or borrows payload storage.
- The dispatcher borrows its `Clock`; the clock must outlive the dispatcher.
- The dispatcher is non-copyable and non-movable; it owns its registry and
  pending queue.
- A `Subscription` may safely outlive its dispatcher (it unregisters via a
  weak registry reference), but the referenced `Telegraph`/`TelegramProvider`
  must remain alive while the subscription is active.
- `DispatchHandle` and `Ticket` values are valid until the entry fires or is
  cancelled; cancelled handles become invalid.

## Threading

The dispatcher owns no thread. Handlers run synchronously on the thread that
calls `update()` or `send_now()`. `MessageDispatcher` is not thread-safe:
confine each dispatcher to one thread or guard all calls with a single
external lock. Handlers must not invoke dispatcher methods recursively.

## Lua bindings

None of the types or functions in this package have a Lua 5.4 binding.

## Related documentation

- Package payload: [Messaging.md](../../../packages/squared-messaging/content/docs/Messaging.md)
- Implementation details: [Squared Messaging — Developer Guide](../developer/squared-messaging/README.md)
- Supporting modules: [Squared Data](../squared-data/README.md), [Squared Time](../squared-time/README.md)
- Documentation index: [Programmer documentation](../README.md)
