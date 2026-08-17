# Squared Messaging — Developer Guide

Squared Messaging is the framework's deterministic Telegram/Telegraph
router, modeled on libGDX-AI messaging concepts but with behavior confirmed
by the module's own tests. A `MessageDispatcher` owns a subscription registry
and a bounded `DeadlineQueue` of pending deliveries, and it integrates two
Squared modules: Time (for deadlines and deterministic ordering) and Data
(for owned JSON payloads and deterministic snapshot/restore).

- Programmer counterpart: [Squared Messaging — Programmer Guide](../programmer/squared-messaging/README.md)
- Package payload: [Messaging.md](../../packages/squared-messaging/content/docs/Messaging.md)
- Documentation index: [Developer documentation](../README.md)
- Diagrams:
  - [Telegram dispatch decision flow](dispatch-decision.dot)
  - [Subscriber and provider delivery](subscriber-delivery.dot)
  - [Delayed queue and Time integration](delayed-time-integration.dot)
  - [Snapshot and restore](snapshot-restore.dot)

## Dependency boundary

The manifest declares exactly two requirements:
`dev.squarednetizen.squared.data` `0.6.0-dev.2` and
`dev.squarednetizen.squared.time` `0.6.0-dev.2`. The CMake target
`squared_messaging` is a `STATIC` library linking `squared_data` and
`squared_time` `PUBLIC` (their headers are part of the messaging public
surface). `message_dispatcher.hpp` includes `deadline_queue.hpp` and
`timepiece.hpp` from Time and `json.hpp` from Data; `telegram.hpp` and
`telegram_provider.hpp` include only `json.hpp`. There is no dependency on
GUI, Scene2D, Application, or any third-party library.

## Architecture

| Component | Source | Responsibility |
| --- | --- | --- |
| `MessageId`, `EndpointId` | `src/message_dispatcher.cpp` | Stable validated namespaced identifiers. |
| `Telegram` | `src/message_dispatcher.cpp` | Owned immutable-access envelope. |
| `Telegraph` | `include/squared/messaging/telegraph.hpp` | Receiver interface (`handle_message`). |
| `TelegramProvider` | `include/squared/messaging/telegram_provider.hpp` | Authoritative current-state generator. |
| `Subscription::Registry` | `src/message_dispatcher.cpp` | Shared binding table and token allocation. |
| `MessageDispatcher` | `include/squared/messaging/message_dispatcher.hpp`, `src/message_dispatcher.cpp` | Routing, queuing, delivery, receipts, snapshot/restore. |

## Ownership and threading

- The dispatcher borrows its `Clock` (`clock_`) and is non-copyable,
  non-movable. It owns `pending_` (a `DeadlineQueue<PendingDelivery>`) and a
  `shared_ptr<Subscription::Registry>`.
- The registry is heap-shared so that `Subscription` handles can outlive the
  dispatcher: a handle keeps a `weak_ptr` and unregisters itself on
  destruction/reset only while the registry is still alive. The registry owns
  the binding records, which hold non-owning `Telegraph*`/`TelegramProvider*`
  pointers; those objects must outlive their active subscriptions.
- `PendingDelivery` stores the owned `Telegram` plus an optional
  `subscription_target` token for provider-generated, subscriber-targeted
  state. Ownership moves through the queue; delivery visits it by value.
- The dispatcher owns no thread. Handlers run synchronously on the thread
  calling `update()` or `send_now()`; the dispatcher is not thread-safe.
  Handlers must not call back into the dispatcher recursively (the
  `subscribe` path deliberately rechecks registry constraints after a
  provider runs because a provider may itself use the dispatcher).

## Invariants and failure behavior

- Identifiers: non-empty, at most 128 ASCII bytes, alphanumeric first/last
  byte, restricted character set, at least one `.`/`/`/`:` separator. A
  `MessageId`/`EndpointId` constructed from invalid text stores the text but
  reports `valid() == false`, and every registration/dispatch path rejects it
  with `InvalidId`/`InvalidTelegram` before any registry mutation.
- Capacity: `pending_` is bounded by `pending_capacity`; the registry by
  `subscription_capacity`; each `update()` drains at most
  `maximum_deliveries_per_update`. Overflow is a reported status, never a
  silent drop.
- One provider per message kind: a second `register_provider` for the same
  kind returns `AlreadyRegistered`.
- A receipt request is valid only with a sender endpoint and non-zero
  correlation; `Telegram::valid()` enforces this.
- Subscription is atomic: provider failure, exhausted handles, or a full
  initial-state queue leave no partially registered subscriber (the token is
  removed again when provider state cannot be queued).
- Provider-generated state is queued at the current time targeted at the new
  subscriber's token; it is intentionally nonpersistent — `snapshot_pending`
  reports `SubscriptionTargetedDelivery` rather than broadening it into a
  broadcast.
- Snapshot/restore round-trip is deterministic: remaining delays (not
  absolute time points), stable names, owned payloads, and no handles or
  tokens. `restore_pending` validates the full version-1 schema and capacity
  before committing, and rolls back committed entries if a later schedule
  fails.
- Cancellation reclaims queue capacity immediately and, for a receipted
  Telegram, queues a separate `cancelled` receipt.

## Data structures and complexity

- The registry is a `std::vector<Binding>` scanned linearly: registration
  duplicate checks and provider lookup are O(n) in subscription count; the
  fixed `subscription_capacity` bounds n. Delivery selection likewise scans
  the registry once to collect receivers (O(n)) — directed lookup could be a
  map, but the capacity cap keeps the scan acceptable and preserves
  subscription order.
- `pending_` is the Time `DeadlineQueue<PendingDelivery>`: a binary heap with
  O(log n) schedule/poll and O(n) cancel. Delivery order is `(due,
  insertion sequence)`, giving deterministic FIFO among equal deadlines.
- `inspect_pending` delegates to `DeadlineQueue::visit_ordered`, which sorts
  entry pointers (O(n log n)) and hands out non-owning views valid only for
  the call.
- `snapshot_pending` builds one `JsonValue::Object` per pending entry plus a
  root document: O(n) JSON nodes. `restore_pending` decodes into a temporary
  vector, validates everything, then commits and rolls back on failure —
  O(n) with the rollback bounded by the number already committed.

## Algorithms and execution order

`send`/`schedule`:

1. `schedule` validates `delay >= 0` and `current + delay` against the
   `TimePoint` range; `send` uses `clock_.now()` directly.
2. `enqueue_at` rejects an invalid `Telegram`, then calls
   `pending_.schedule_at(due, PendingDelivery{...})`, mapping the queue's
   `ScheduleResult` to `DispatchStatus` (`QueueFull`, `InvalidDelay` for
   `InvalidTime`, `HandleExhausted` for `TicketExhausted`). The returned
   ticket becomes the `DispatchHandle`.

`update`:

1. Capture `clock_.now()` exactly once.
2. `pending_.poll_due(captured_now, maximum_deliveries_per_update, visitor)`
   delivers at most `limit` due entries; each is handed to `deliver`.
3. Return the delivered count.

`deliver(telegram, captured_now, subscription_target)`:

1. Validate the envelope (an unvalidated envelope reports `accepted = false`).
2. Select receivers: a `subscription_target` token narrows to that broadcast
   binding; a `receiver()` narrows to the matching endpoint binding (first
   match); otherwise all broadcast subscribers of the message kind — the
   registry scan preserves subscription order.
3. Call `handle_message` on each receiver, counting `handled_count`.
4. Aggregate `ReceiptStatus`: `ReceiverUnavailable` when no receiver was
   selected, `Handled` when any reported handled, else `Unhandled`.
5. `queue_receipt` enqueues the receipt `Telegram` to the sender endpoint at
   `captured_now` when a receipt was requested; the receipt carries
   correlation, message, status name, and optional receiver in its JSON
   payload.

`snapshot_pending`:

1. `inspect_pending` over `visit_ordered` computes remaining delays against a
   fresh `clock_.now()`; any `subscription_targeted` entry aborts with
   `SubscriptionTargetedDelivery`.
2. Each entry becomes an object with `remainingNanoseconds` (unsigned),
   `message`, `payload`, `sender`/`receiver` (null when absent), `correlation`
   (unsigned), and `receiptRequested` (boolean). The root adds `schema`
   (`squared.messaging.pending`), `version` (`1`), and `messages`.

`restore_pending`:

1. Refuse when the pending queue is not empty.
2. Validate root shape (`schema`, `version`, `messages`, exactly three
   members), message count against capacity, and every entry's exact seven
   fields with correct types; endpoint fields must decode to valid
   `EndpointId`s and the rebuilt `Telegram` must be `valid()`.
3. Validate `current + remainingNanoseconds` against the time domain
   (`TimeOverflow`).
4. Commit via `schedule_at(captured_now + delay, ...)`; on any failure cancel
   every committed ticket and report the mapped status.

## Design patterns

- **Publish/Subscribe (Observer)** — broadcast delivery matches
  `Telegraph` subscribers by `MessageId`; `Subscription` handles are the
  standard scoped, move-only registration token. Directed delivery is a
  special case resolved through the same registry rather than a separate
  mechanism.
- **Command/Message objects** — `Telegram` is an immutable owned message
  object carrying all routing and correlation metadata; dispatching it is
  deferred through the queue, so senders and receivers never couple.
- **Provider mediation** — `TelegramProvider` replaces retained-message
  replay: on each new subscriber it computes authoritative current state,
  which is queued as ordinary mail targeted at that subscriber. This is a
  deliberate deviation from libGDX-AI-style retained messages and from a
  replay cache (which can serve stale envelopes).
- **Memento** — `snapshot_pending` captures the pending queue as a
  deterministic, versioned, transportable document; `restore_pending` is the
  transactional undo/restore side. The snapshot encodes relative delays and
  stable names only, so it survives process boundaries.
- **Mediator** — `MessageDispatcher` centralizes registration, routing,
  ordering, and receipt bookkeeping so Telegraphs never reference each other.

Alternatives rejected: a global singleton dispatcher (rejected — multiple
time domains need multiple routers); callback-registry scheduling (rejected —
the Time deadline queue keeps ordering and capacity policy in one tested
primitive); persisting handles/tokens in snapshots (rejected — they are
process-local); weak-ref Telegram callbacks (see Limitations).

## Limitations and technical debt

- The registry is a linear vector scan, so O(n) per registration/delivery;
  acceptable under `subscription_capacity` but not designed for very large
  subscriber sets.
- `TODO.md` lists dispatcher persistence tests and cancellation of delayed
  messages for removed receivers as "Next"; the snapshot/restore tests in
  the current tree already cover the persistence path, but receiver-removal
  cancellation semantics remain unfinished.
- No weak-callback telegrams and no cross-thread queue exist yet; handlers
  are synchronous on the calling thread by contract.
- Receipts are queued messages and are not themselves persisted as receipts;
  a snapshot captures the pending receipt envelopes but no separate receipt
  ledger exists.
- Snapshot refuses subscription-targeted provider state rather than encoding
  it; applications must drain such state before checkpointing.
- Priority ordering is explicitly deferred (`TODO.md` "Later") until the
  deterministic delivery semantics are fixed.
