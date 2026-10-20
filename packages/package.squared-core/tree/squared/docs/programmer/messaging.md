# messaging

Addressed, queued message passing between decoupled parts of a program:
endpoints register with a dispatcher, messages are delivered immediately or on
a deadline, and pending traffic can be snapshotted and restored.

Every type below lives in its own header. Include exactly the ones a
translation unit names; `squared/messaging/messaging.hpp` pulls in all of them
and exists for convenience, not for use inside headers of your own.

Developer counterpart: [../developer/messaging.md](../developer/messaging.md)

> [!note] Being replaced (Sprint 6)
> The Telegraph is being rebuilt to the app model's design: a 32-byte binary
> envelope, two lock-free rings between the UI and the model thread, and no
> allocation per telegram. Names, addresses and the name table below are its
> first part (6.1). The dispatcher, endpoints and `Telegram` further
> down are the old design; nothing in squared uses them, and they go when the
> new Telegraph replaces them.

## Names and addresses

A telegram goes to an **address**: a **name**, which is the schema
(`player.hp`, `item.name`, `inventory`), and a **uid**, which says which object
(the model's own id for an item, a buffer, a row). Names are written in the
code; uids come from the data.

```cpp
#include <squared/messaging/address.hpp>

using sq::messaging::Address;
Address hp{ "player.hp" };              // no uid
Address coat{ "item.name", 17 };        // item[17].name
```

| Type | Header | Is |
|---|---|---|
| `Name` | `squared/messaging/name.hpp` | 8 bytes: the 64-bit FNV-1a hash of the name's text |
| `Address` | `squared/messaging/address.hpp` | a `Name` and a 32-bit `uid` (0: none); 16 bytes |
| `NameTable` | `squared/messaging/name_table.hpp` | the registered names' text, for writing telegrams as text |

**A name literal costs nothing at run time.** `Name n = "player.hp";` is
hashed by the compiler, and checked there: a name is dot-separated segments of
`a`-`z`, `0`-`9` and `_`, at most 255 bytes, so `Name n = "Player HP";` does
not compile. A name built at run time (from a mod's file, say) is
`Name::of(text)`: hashed once, and checked when it is registered.

**Data never goes in a name.** Which item is the uid, how many is the value.
Names are a few hundred, or a few thousand with mods; that is what keeps the
name table small and the ids from colliding.

### The text form

| Address | Written |
|---|---|
| `{ "player.hp" }` | `player.hp` |
| `{ "item.name", 17 }` | `item[17].name`: the uid after the first segment |
| `{ "inventory", 20 }` | `inventory[20]` |
| a name not registered, uid 3 | `#` and the id's 16 hex digits, then `[3]` |

`parse_address(text)` reads that back with no table (the name is hashed):

```cpp
if( auto address = sq::messaging::parse_address( "item[17].name" ) ) {
    // address->name == Name("item.name"), address->uid == 17
}
```

It returns nothing for text that is not an address: an invalid name, a uid
past 4,294,967,295, brackets anywhere but after the first segment. `[0]` reads
as no uid.

### The name table

```cpp
std::array<std::byte, 8192> storage;           // the Telegraph will own this
sq::messaging::NameTable names{ storage };
names.add( "player.hp" );                      // added
std::array<char, 64> line;
std::size_t n = names.format( { "item.name", 17 }, line );  // "item[17].name"
```

| Call | Does |
|---|---|
| `add(text)` | Register a name. Returns `added`, `known` (already there, same text), `invalid` (not a name), `collision` (another name has the same id: rename one) or `full` |
| `text(name)` | The registered text, or empty |
| `format(address, out)` | Write the text form into `out`; the length, or 0 when `out` is too small |
| `size()`, `used()`, `capacity()` | Names registered; bytes used and available |

16 bytes per name plus its text: 8 KB holds about 250 names. Nothing
allocates; the storage is yours (the Telegraph's, later) and must outlive the
table. Register names before the threads start.

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
