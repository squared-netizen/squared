# messaging &mdash; internals

Addressed, queued message passing. Endpoints, broadcasts and providers share
one registry; delivery is either immediate or deferred through
`sq::time::DeadlineQueue`.

Programmer counterpart: [../programmer/messaging.md](../programmer/messaging.md)

- Public types: 23
- Translation units: 6

## Layout

```
messaging/
  include/squared/messaging/   one header per public type, plus messaging.hpp
  src/                         one translation unit per type with definitions
  src/detail/                  private, not installed
  Makefile
```

## Notes

`Subscription::Registry` is the module's shared implementation type: it holds
every binding, its capacity limit, and the token counter. Both `subscription.cpp`
and `message_dispatcher.cpp` need its definition, so the out-of-line member
definition lives in `src/detail/messaging_detail.hpp` rather than being
duplicated. That header also carries `detail::valid_stable_id`, the identifier
validator shared by `MessageId` and `EndpointId`.

`dispatcher_endpoint_id` stayed in `message_dispatcher.cpp`'s anonymous
namespace, because only that translation unit uses it. The rule is simple: a
helper leaves the anonymous namespace only when a second translation unit
needs it.

`receipt_message_id()` and `receipt_status_name()` belong to neither `Telegram`
nor `ReceiptStatus`, so they have a header and a translation unit of their own
rather than being attached to whichever type they happened to sit near.

## Known warning

`detail::valid_stable_id` iterates a `std::string_view` as `const unsigned
char`, which `-Wsign-conversion` flags. The code is unchanged from before the
split, but the warning is now emitted once per including translation unit
rather than once overall. The fix is a `static_cast<unsigned char>` in the
range-for; it changes no behaviour, and it was left out of the split so the
diff stays purely structural.

## Memory

`Telegram` carries a `sq::data::JsonValue` payload by value and two
`std::string`-backed identifier types. `MessageId` and `EndpointId` each hold a
`std::string` plus a validity flag, so every telegram carries at least two
heap-capable strings. Interning identifiers would apply here for the same
reason it applies to `gui::Skin`; see [priority-audit.md](priority-audit.md).
