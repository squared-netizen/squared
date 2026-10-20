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
  message_dispatcher.hpp subscription.hpp telegram.hpp telegraph.hpp messaging.hpp
  src/          the 20 supporting headers: ids, results, statuses, receipts,
                and the provider extension surface (telegram_provider.hpp);
                one translation unit per type with definitions
  src/detail/   private, not installed
  Makefile
```

## Names, addresses and the name table (Sprint 6.1)

The first part of the new Telegraph, as the app model specifies it (an
address is a name plus a uid). Three types, two translation units, no
allocation.

- **`Name`** is a 64-bit FNV-1a hash (offset basis `0xcbf29ce484222325`, prime
  `0x100000001b3`). `name_hash` takes a seed so a name can be hashed in
  pieces: `item[17].name` is parsed by hashing `item`, then `.name` continuing
  from it, with nothing copied. The literal constructor is `consteval`; an
  invalid literal calls `detail::not_a_valid_name()`, which is declared and
  never defined or constexpr, so the program does not compile (no `throw`,
  which the source rules count). 64 bits, decided in Sprint 5: one in about 37
  trillion for 1,000 names to collide, where 32 bits gives one in 8,600, and
  mods register names on users' phones.
- **`Address`** is a `Name` and a `std::uint32_t` uid: 16 bytes with padding.
  The envelope (6.2) packs it into 12.
- **`NameTable`** is one span of bytes its owner supplies. Entries (16 bytes:
  id, text offset, length) sit sorted by id at the front; text grows from the
  back; they meet in the middle, and `full` is the gap closing. Lookup is a
  binary search, O(log n); `add` is O(n) for the shift, fine for a few
  thousand names registered at start. Entries are read and written with
  `memcpy`, so the storage needs no alignment. A collision (same id, other
  text) is refused and the first name kept; it can only be tested through a
  friend (`NameTableProbe`), since no two real names are known to collide.
- **Text form:** the uid goes after the first segment (`item[17].name`), as the
  app model's list protocol writes it. An unregistered name is `#` and 16 hex
  digits, so a line can always be written and read back.
- **Standard library:** `std::span`, `std::string_view`, `std::from_chars`,
  `std::to_chars`, `std::optional` for the parse result; none allocates. A
  hash map was passed over for the sorted array: smaller (no buckets, no
  load factor) and the lookups are not on a per-frame path.
- **Patterns:** the table is a Flyweight store for name text: every telegram
  carries 8 bytes, and the text exists once.
- **Memory:** `sizeof(Name)` 8, `sizeof(Address)` 16, `sizeof(NameTable)` 24,
  plus the storage it is given (`TELEGRAPH_NAMES`, 8 KB by default, when the
  Telegraph owns it). Measured: 203 names in 5,649 bytes, about 28 bytes each.
- **Tested** (`test/address_test.cpp`): the published FNV-1a vectors and
  hashing in pieces at compile time; valid and invalid names; parsing 9 forms
  and refusing 16; registration (added, known, invalid, collision, full,
  nothing half-written); 200 names in no order all found again; every address
  written and read back to itself; a buffer too small; and no allocation in
  `add`, `text`, `format` or `parse_address`. An invalid literal was checked
  by hand not to compile, with g++ and clang.

The old dispatcher below stays until the new Telegraph replaces it; nothing in
squared uses it.

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
