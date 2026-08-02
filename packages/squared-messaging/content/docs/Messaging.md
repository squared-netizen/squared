---
title: Telegram Messaging
tags:
  - application
  - messaging
  - scheduling
---

# Telegram Messaging

The independent Squared Messaging module provides `squared::messaging`. It is
inspired by the useful Telegram/Telegraph model in libGDX-AI and declares
exact Squared Data and Squared Time module dependencies.

- `Telegram` is an owned message envelope.
- `Telegraph` handles an envelope and reports whether it was handled.
- `MessageDispatcher` performs registration, routing, queuing, delayed
  delivery, cancellation, and deterministic ordering.
- `TelegramProvider` creates authoritative current state for a newly
  registered subscriber.
- `Subscription` is a move-only scoped endpoint registration or broadcast
  subscription, including provider registrations.

There is no global dispatcher. Each dispatcher borrows one read-only
`squared::time::Clock`, and applications use separate dispatchers when they
need separate application, simulation, UI, or editor-preview time domains.
The dispatcher owns no thread; handlers run on the thread calling `update()`
or the explicitly synchronous `send_now()`.

## Identity and payloads

`MessageId` and `EndpointId` contain stable namespaced strings rather than
pointers or process-local numeric addresses. Valid IDs are at most 128 ASCII
characters, begin and end with a letter or digit, use a restricted safe
character set, and contain `.`, `/`, or `:` as a namespace separator.

Payloads are owned `squared::data::JsonValue` instances. Receivers get
immutable access through `Telegram`, so delivery never exposes a borrowed
payload pointer.

## Delivery

`send()` queues a Telegram at the current domain time. `schedule()` queues it
after a non-negative delay. `send_now()` exists for controlled internal use
where synchronous delivery is intentional.

A Telegram with a receiver goes only to that registered endpoint. A Telegram
without a receiver is broadcast to subscribers of its message ID in
subscription order. Pending messages are ordered by due time and insertion
sequence.

Queue capacity, subscription capacity, and maximum deliveries per update are
explicit configuration. Overflow and invalid input are reported rather than
silently dropped. Cancelling a pending message immediately reclaims its queue
slot.

## Authoritative state on subscription

`register_provider()` installs at most one `TelegramProvider` for a message
ID. When a new broadcast subscriber registers for that ID, the provider is
called once and returns one of:

- `Provided`, with an owned JSON value;
- `NoCurrentState`, allowing the subscription without initial delivery;
- `Failed`, rejecting the subscription with a diagnostic detail string.

A provided value becomes a fresh queued Telegram addressed only to that new
subscription. Existing subscribers do not receive it, and it follows ordinary
queue ordering and delivery limits. This replaces retained-message replay:
historical envelopes can be stale, while the provider computes current state
at the point of subscription.

Provider failure, exhausted subscription handles, or a full initial-state
queue reject the subscription atomically. No partially active subscriber is
left behind. The provider registration is scoped, and the provider object
must remain alive while its registration handle is active.

## Receipts

A Telegram may request a return receipt when it supplies a sender endpoint and
nonzero correlation ID. The receipt is a separate queued
`squared.messaging.receipt` Telegram with an owned JSON payload. Its status is
one of:

- `handled`;
- `unhandled`;
- `receiver_unavailable`;
- `cancelled`.

Receipts never mutate or redispatch the original envelope.

## Pending inspection and persistence

`inspect_pending()` visits queued entries in delivery order without copying
Telegrams or JSON payloads. Each temporary view contains its dispatch handle,
remaining domain-time delay, immutable Telegram, and whether it targets a
process-local subscription.

`snapshot_pending()` creates deterministic versioned JSON. It stores remaining
nanoseconds, stable message and endpoint names, owned payloads, correlations,
and receipt requests. It never stores raw time points, pointers, dispatcher
handles, or subscription tokens.

`restore_pending()` is transactional and accepts only an empty queue. The
complete document is validated against the version-1 schema and queue capacity
before any delivery is committed. Equal deadlines retain snapshot array order.
Invalid input, overflow, and nonempty-queue ambiguity are explicit results.

Fresh provider state targets a specific live subscription token and is
therefore intentionally nonpersistent. Snapshotting reports this condition
instead of silently broadening the message into a broadcast. Applications
should run the dispatcher once to deliver such startup state before taking a
checkpoint.

Cancellation metadata beyond the stable handles exposed by inspection remains
a follow-on Phase 5 refinement.

## Generated diagnostic

The generated application renders a text report with explicit `PASS`, `FAIL`,
or `PENDING` values for the atlas, strict JSON/SDL_ttf path, provider,
snapshot restoration, application-time delivery, and native log write. Color
squares remain supplementary signals rather than the primary diagnosis.

The magenta/cyan timing square is driven by delayed directed Telegrams sent to
the generated application endpoint. Its continued one-second alternation is
visible proof that `Timepiece`, `MessageDispatcher`, `Telegram`, and
`Telegraph` operate together in the Android application.

The tiny provider square beside it begins red and becomes green after the
generated application subscribes and receives freshly provided state through
the ordinary queue. Green is visible proof that provider registration,
state-on-subscription, targeted delivery, and JSON payload handling operated
together.

The second tiny square is orange on failure and green after a local pending
Telegram survives snapshot, deterministic JSON writing, strict parsing,
transactional restoration, time advancement, and final delivery.

The report includes a compact deterministic JSON representation of the
provider Telegram. The same report and JSON are written natively to
`/sdcard/Download/<project-id>-diagnostics.log`.
