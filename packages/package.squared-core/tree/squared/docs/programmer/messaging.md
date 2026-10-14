# messaging

Addressed, queued message passing between decoupled parts of a program:
endpoints register with a dispatcher, messages are delivered immediately or on
a deadline, and pending traffic can be snapshotted and restored.

Every type below lives in its own header. Include exactly the ones a
translation unit names; `squared/messaging/messaging.hpp` pulls in all of them
and exists for convenience, not for use inside headers of your own.

Developer counterpart: [../developer/messaging.md](../developer/messaging.md)

## Envelope

| Type | Header | Purpose |
|---|---|---|
| `MessageId` | `squared/messaging/src/message_id.hpp` | Stable namespaced identifier for one Telegram kind |
| `EndpointId` | `squared/messaging/src/endpoint_id.hpp` | Stable namespaced address for one directed Telegraph endpoint |
| `CorrelationId` | `squared/messaging/src/correlation_id.hpp` | Application-owned correlation tag echoed by queued receipts |
| `Telegram` | `squared/messaging/telegram.hpp` | Owned immutable-access message envelope |
| `ReceiptStatus` | `squared/messaging/src/receipt_status.hpp` | Outcome of delivering one Telegram to its subscribers |

## Receiving

| Type | Header | Purpose |
|---|---|---|
| `Telegraph` | `squared/messaging/telegraph.hpp` | Receiver of Telegram values on the dispatcher's calling thread |
| `TelegramProvider` | `squared/messaging/src/telegram_provider.hpp` | Generates authoritative current state for a new subscriber |
| `ProviderStatus` | `squared/messaging/src/provider_status.hpp` | Outcome category of one authoritative current-state request |
| `ProviderResult` | `squared/messaging/src/provider_result.hpp` | Owned result of one authoritative current-state request |

## Subscribing

| Type | Header | Purpose |
|---|---|---|
| `Subscription` | `squared/messaging/subscription.hpp` | Move-only scoped endpoint registration or broadcast subscription |
| `SubscriptionStatus` | `squared/messaging/src/subscription_status.hpp` | Outcome category of one registration operation |
| `SubscriptionResult` | `squared/messaging/src/subscription_result.hpp` | Owned result of one register/subscribe/provider operation |

## Dispatching

| Type | Header | Purpose |
|---|---|---|
| `DispatchHandle` | `squared/messaging/src/dispatch_handle.hpp` | Stable handle for one queued or delayed Telegram |
| `DispatchStatus` | `squared/messaging/src/dispatch_status.hpp` | Outcome category of one dispatch operation |
| `DispatchResult` | `squared/messaging/src/dispatch_result.hpp` | Owned result of one send/schedule operation |
| `DeliveryReport` | `squared/messaging/src/delivery_report.hpp` | Compressed outcome describing one immediate delivery pass |
| `MessageDispatcherConfig` | `squared/messaging/src/message_dispatcher_config.hpp` | Capacity limits that bound every dispatcher queue |
| `MessageDispatcher` | `squared/messaging/message_dispatcher.hpp` | Bounded deterministic Telegram router for one time domain |

## Snapshot and restore

| Type | Header | Purpose |
|---|---|---|
| `PendingMessageView` | `squared/messaging/src/pending_message_view.hpp` | Non-owning view valid only during pending-message inspection |
| `PendingSnapshotStatus` | `squared/messaging/src/pending_snapshot_status.hpp` | Outcome category of one pending-queue snapshot |
| `PendingSnapshotResult` | `squared/messaging/src/pending_snapshot_result.hpp` | Owned result of one pending-queue snapshot |
| `PendingRestoreStatus` | `squared/messaging/src/pending_restore_status.hpp` | Outcome category of one pending-queue restore |
| `PendingRestoreResult` | `squared/messaging/src/pending_restore_result.hpp` | Owned result of one pending-queue restore |

## Free functions

`squared/messaging/src/receipt.hpp` declares the two framework constants used for
queued return receipts:

| Function | Returns |
|---|---|
| `receipt_message_id()` | the `MessageId` the framework uses for receipts |
| `receipt_status_name(ReceiptStatus)` | the stable lowercase name used in JSON payloads |

## Dependencies

`sq::time` for the deadline queue and clock domain, and `sq::data`
for the `JsonValue` message payload. `sq::data` lives outside this
archive; see [building.md](building.md).
