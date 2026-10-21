# messaging &mdash; internals

The Telegraph's parts, rebuilt in Sprint 6 to the app model: names and
addresses (6.1), keys (6.1b), the 32-byte telegram and its text form (6.2),
the ring between two threads (6.3), the latest-value cache (6.4), both ends
of a virtualized list (6.5), the Telegraph (6.6a) and the model's thread
with its wakes (6.6c). Nothing here allocates but `std::thread`'s state, once
at start.

Programmer counterpart: [../programmer/messaging.md](../programmer/messaging.md)

## Layout

```
messaging/
  name.hpp address.hpp name_table.hpp key.hpp telegram.hpp telegram_data.hpp
  telegram_ring.hpp telegram_text.hpp value_cache.hpp model_list.hpp
  list_window.hpp telegraph.hpp model_runner.hpp messaging.hpp
  src/   verb.hpp value_type.hpp data_ref.hpp telegraph_config.hpp wake.hpp
         (supporting); address.cpp, name_table.cpp, telegram.cpp,
         telegram_data.cpp, telegram_ring.cpp, telegram_text.cpp,
         value_cache.cpp, model_list.cpp, list_window.cpp, telegraph.cpp,
         wake.cpp, model_runner.cpp
  test/  address_test.cpp, telegram_test.cpp, telegram_ring_test.cpp,
         value_cache_test.cpp, list_test.cpp, telegraph_test.cpp,
         telegraph_thread_test.cpp
```

**The old dispatcher was removed (2026-10-20),** 34 files: `MessageDispatcher`,
`Subscription`, `EndpointId`, `MessageId`, the receipts, the providers (the
module's extension surface, so `tools/check_headers.sh` lists none now) and a
208-byte `Telegram` carrying a `JsonValue` and two `std::string` ids. Nothing
outside the module used it, it had no tests, and the new `Telegram` needed its
name. Its fates, as the app model decided: scheduled delivery is the model's
timers (`time::DeadlineQueue`, moved to the module root, since nothing in
`src/` reached it any more); correlation ids are in the envelope; providers
become the latest-value cache; snapshots become persistence in `background()`;
receipts are dropped; JSON payloads are for saves and the network only. The
module no longer depends on `time` or `data`.

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

## Keys (6.1b)

`Key<T, Verbs, Uid>` is a name, a `const char*` to the literal (for the name
table: the text is already in the binary, so registering a key copies nothing
until the Telegraph does it once), and three things only the compiler sees.
16 bytes; keys are constants and never travel.

- **`Verbs`** is a structural struct of four bits, so it can be a template
  argument and be combined with `|` (`state | set`).
- **`sendable<K, Verb, Value>`** is the concept every sending call will be
  constrained by: the key carries the verb, and the value converts as the
  type says. Deliberately narrow: an integer takes any integer but never a
  bool; a number takes any arithmetic type but a bool; text takes anything a
  `string_view` can be made from. A narrower concept makes the compile error
  the place a mistake is found.
- `decltype` of an `inline constexpr` key is `const Key<...>`, so the concept
  strips cv and references first (the first build's tests caught it).
- Pattern: none beyond a type tag. Nothing virtual, nothing at run time.

## The telegram (6.2)

| Bytes | Field |
|---|---|
| 0&ndash;7 | name id |
| 8&ndash;11 | uid |
| 12&ndash;13 | correlation |
| 14&ndash;15 | kind: verb in bits 0&ndash;1, value type in bits 2&ndash;5, the rest 0 |
| 16&ndash;31 | value |

No padding, so the 32 bytes are the telegram: `memcpy` in, `memcpy` out, and
`operator==` is the bytes. The value is a `std::array<std::byte, 16>` read and
written with `memcpy`, not a union, so there is no type punning to argue
about. The factories zero the bytes a type does not use, which is what makes
"equal" mean "equal bytes"; `valid()` checks it for bytes that arrive from a
file, along with the spare bits, the type range, a bool being 0 or 1, no NaN,
and a uid list being whole uids. The class is trivially copyable, which the
rings (6.3) rely on.

**Why no NaN:** the text form must give back the same bits, and NaN has many
bit patterns and one word. A NaN is a programmer error (asserted in the
factories); `read_telegram` refuses `nan`.

**Text and uid lists live in a data area,** and the telegram holds where
(`DataRef`: 32-bit offset and length). `TelegramData` is the simple one, a
span filled from the front, for reading lines and for tests; the Telegraph's
rings keep their own data rings (6.3). "Bit for bit" for these types means
everything but the offset, which depends on where the data area put them;
the tests compare the bytes instead.

## The text form (6.2)

`write_telegram` and `read_telegram`, in `telegram_text.cpp`.

- **Numbers:** `std::to_chars` and `std::from_chars`, so no locale and the
  shortest form that reads back to the same bits (doubles and floats). A
  double always shows a `.`, an exponent or `inf`, or `.0` is added, so `2.0`
  is a number and `2` an integer. `-0.0` survives.
- **Text:** escapes for `\`, `"`, newline and tab, `\xNN` for other control
  bytes and 0x7F; bytes from 0x80 up are written as they are, so UTF-8 stays
  readable. Reading decodes in two passes: the first checks and counts, the
  second writes straight into the data area at its final place
  (`TelegramData::reserve`, `writable`), so nothing is copied twice and
  nothing is allocated.
- **Refusal leaves nothing behind:** a line that fails after its text or uids
  were read rewinds the data area (`TelegramData::rewind`).
- **Cost to note:** `to_chars` for doubles carries tables (Ryu) into any app
  that writes telegrams as text; to be measured when the dev log uses it.

**Tested** (`test/telegram_test.cpp`): the key concept both ways (7 sends that
compile, 8 that must not, with `static_assert`, so a regression is a build
failure); the telegram's layout, byte equality, and `valid()` refusing a stray
value byte, a spare bit, an unknown type; 16 exact lines, each read back;
8 non-canonical lines read and written canonically; 25 lines refused,
3 of them after their data was read, with the data area left empty; a data
area too small; 20,000 random telegrams (every type, random uids,
correlations, text bytes 0&ndash;255, floats including infinities, &minus;0
and denormals, doubles from random bits) written, read and written again,
identical; 20,000 damaged lines, read or refused but never crashing, and
every one read written canonically and read again to the same line; no
allocation in reading or writing. Four planted bugs (no `.0` on a whole
double, 0x7F unescaped, the spare bits unchecked, no rewind on failure) each
fail it. g++ and clang.

## The ring (6.3)

`TelegramRing`, in `telegram_ring.cpp`: a bounded single-producer,
single-consumer queue of telegrams with a data ring beside it, as the app
model specifies (one each way, sized at init, no lock, no allocation, a full
ring returns `full`).

- **Header placement:** at the module root, beside `TelegramData`, though
  apps will reach it only through the Telegraph. A header in `src/` must be
  reachable from a workhorse header (`make headers`), and nothing reaches it
  until the Telegraph exists (6.6); listing it as extension surface would be
  wrong. When the Telegraph includes it, it can move to `src/` or stay; to be
  settled then.
- **Slots:** a power of two of 32-byte slots, so an index is a free-running
  32-bit counter masked; the capacity divides 2^32, so the counters wrap
  harmlessly, and `head - tail` is the fill. Telegrams go in and out with
  `memcpy` (`TelegramBytes`), so the storage needs no alignment. Full is
  `head - tail == capacity`: no slot is wasted to tell full from empty.
- **The data ring:** `push` copies a `text` or `uids` value in and rewrites
  the telegram's `DataRef` to where it went. A value never crosses the end of
  the ring: when it would, the rest of the ring is skipped and it starts at 0.
  The consumer does not need the producer's position to follow: values are
  popped in the order they were written, so a value starts where the last one
  ended, unless its offset is not that place, which means the end was
  skipped. Empty text takes no room. A value over a quarter of the data ring
  is `too_large` (decided 2026-10-20). With the skip, a value can need almost
  twice its size, so anything up to half the ring would always fit in an
  empty one and the ring could never be stuck; the quarter also keeps room
  for a second value of the largest size while one waits, so one big value
  never holds up the next (throughput, not safety; corrected 2026-10-20, the
  first text said a larger value could block the ring). The spec makes an
  oversized value a programmer error, asserted in debug builds; that assert
  belongs to the Telegraph's send (6.6), because the ring's own test has to
  see `too_large` come back.
- **Memory order:** the producer writes the data and the slot, then stores
  `head` with release; the consumer loads `head` with acquire, reads, then
  stores `tail` and `data_tail` with release; the producer loads both with
  acquire before reusing room. The producer's and the consumer's counters sit
  on separate 64-byte lines, so the two threads do not take one cache line
  from each other at every telegram (priority 3, for 128 bytes). No cached
  copies of the other side's counter: one acquire load a call is cheap at the
  spec's rate (about 50 telegrams a turn), and it keeps `stats()` exact.
- **Stats:** the highest number of telegrams and data bytes held at once (the
  skipped end included), seen by the producer as it pushes; relaxed atomics,
  so the Console's `telegraph` command can read them from another thread. The
  spec sizes the knobs from these.
- **Waiting is not here.** Neither side waits. The UI drains at the start of
  a frame; waking the model thread when a telegram arrives, and the model's
  bounded wait for room, are the model thread's (6.6).
- **Standard library:** `std::atomic<std::uint32_t>` (lock-free on arm64 and
  x86-64), `std::span`, `std::optional`, `std::memcpy`. Passed over:
  `std::deque` and `std::queue` (they allocate), `std::mutex` (the UI thread
  must never block), `std::hardware_destructive_interference_size` (g++ warns
  that its value may differ between compilers; 64 is right for both targets).
- **Patterns:** none from the catalogue; a ring buffer (bounded queue).
- **Tested** (`test/telegram_ring_test.cpp`): every value type in order, with
  correlations; empty text; full slots (nothing written, room after a pop);
  a full data ring (other telegrams still sent); a value that skips the end,
  the consumer following it, and the room after; `too_large`, also on a ring
  with no data ring; no allocation over 2,000 pushes and pops; 200,000 random
  steps against a `std::deque` model, every telegram and byte checked, and
  never `full` when the ring is empty; and two real threads, 200,000
  telegrams (integers, text and uid lists up to a quarter of a 256-byte data
  ring, 16 slots, so it is full most of the time), each checked for order and
  bytes. Clean under ThreadSanitizer (g++). Four planted bugs each fail it:
  `head` stored relaxed (ThreadSanitizer), the consumer ignoring the skip,
  the producer forgetting the skip when it checks for room, and the size
  limit at a half. g++ and clang.

## The latest-value cache (6.4)

`ValueCache`, in `value_cache.cpp`: the last value of each address, on the
UI side, so a screen built later shows current values at once (the app
model's cache). Decided 2026-10-20 ("yes to all your suggestions"), from the
choices the spec left open: how an entry is found, and how text room comes
back.

- **Entries:** a dense array of 32-byte entries, laid out as a telegram's
  bytes with the correlation's two bytes used for a bound count and flags
  (0&ndash;7 name, 8&ndash;11 uid, 12 bound, 13 flags, 14&ndash;15 kind,
  16&ndash;31 value). Read and written with `memcpy`, so the storage needs no
  alignment. A power of two of them, 2 to 32,768. New entries are handed out
  in order (`fresh_`); after that, an entry is reused only through the clock.
- **Finding one:** an open-addressed index beside the array, two 2-byte
  slots an entry, so it is at most half full and probes stay short. The slot
  is the name's hash with the uid mixed in and the bits spread (a
  multiply-xorshift), so the low bits depend on all of them. Linear probing;
  a removed entry's slot is closed by a backward shift, not a tombstone, so
  lookups never slow down with age. Priority 1 chose the separate index: the
  entries in the table itself at a safe fill would leave about 4 KB of spare
  entries at the defaults; the index is 2 KB. Passed over: a sorted array
  with binary search, like the name table (smaller, but adding an address
  shifts up to 16 KB, and virtualized lists add uids at every scroll:
  priority 3); `std::unordered_map` (a node per entry, &sect;2.1).
- **Eviction, the clock:** one "recently written" bit an entry and a hand
  going round. A bound entry is passed; a marked one loses its mark and is
  passed; the first other is taken. Two turns see every entry unmarked, so if
  none is taken every entry is bound, and the value is dropped and counted.
  Roughly oldest first, not strictly: an entry written once long ago can
  outlive one written twice since. Chosen over a 2-byte timestamp an entry
  and a scan of all 512 for the oldest, which a full cache would pay on every
  new value while a list scrolls. `unbind` marks the entry, so one just shown
  is passed once more.
- **Binding:** a count, 255 at most; `bind` makes an entry with no value yet
  when the address has none, so the first value lands in it.
- **Text and uid lists:** up to 16 bytes in the entry's value (the flags
  byte holds the length plus one; no text-area room, nothing to reclaim).
  Longer ones in the text area, each after a 4-byte header (its entry, its
  room). A longer value replacing one is rewritten in place when it fits the
  old room; otherwise appended. When the end is reached, the area is
  compacted: walked front to back, a text kept only if its entry still
  points at it, slid down, its room shrunk to its length, its entry's offset
  updated. No sort, at most the area moved, and only when it fills. Still
  short: the text being replaced is dropped, but only once that is enough
  (so a store that fails keeps the old value); then unbound entries holding
  text give theirs up, by the clock; then the value is dropped and counted.
  A value over a quarter of the text area is `too_large`, as in the ring.
  Passed over: fixed blocks with a free list (a long text would span blocks,
  so it could not be read as one piece); a ring (a replaced text in the
  middle could never be freed).
- **A view's life:** `find`'s data points into the cache, valid until the
  next `store` or `bind` (compaction moves text, eviction reuses entries).
  The cache is used on the UI thread only, so that means within the frame;
  a caller keeping it longer copies it.
- **`dropped` keeps the old value.** The spec: "the new value is dropped".
  An actor then shows the value before; the drops are counted for the
  Console's `telegraph` command, which is how the knobs are set.
- **Standard library:** `std::span`, `std::optional`, `std::memcpy`,
  `std::memmove`, `std::fill`. No container allocates.
- **Patterns:** none from the catalogue; the clock is the textbook
  second-chance page replacement.
- **Tested** (`test/value_cache_test.cpp`): every value type back as it went
  in, the correlation dropped; text of 0, 8 and 16 bytes in the entry, 17 and
  100 in the text area; uid lists of 3 and 5; a value replaced by another
  type and back; in place (shorter, then back to its size, nothing
  appended); 2,000 random growing values on three addresses, every value
  right after each store, compaction always enough; the clock's exact order
  on 4 entries (bound passed, marks cleared, the rewritten one kept); every
  entry bound (dropped, counted, `bind` refused), unbound again and taken;
  255 binds; a full text area (an unbound text-holder gives way, bound ones
  do not, a dropped value keeps the old one); too large; no allocation over
  2,000 rounds; 300,000 random steps over 120 addresses and 32 entries
  against a model (every value found is the latest stored, a value stored
  while bound is never lost, drops only for want of room, the counts agree;
  the last third only binds, so the full paths run too: 152,107 stored,
  58,320 dropped, 127,885 evicted); 8 rounds of 1,024 addresses on 1,024
  entries, each round all found. Clean under AddressSanitizer and
  UndefinedBehaviorSanitizer. Five planted bugs each fail it: the backward
  shift's condition inverted, compaction not moving the offset, the clock
  taking bound entries, a rewrite in place ignoring its room, and the old
  text dropped before the new one is sure to fit. g++ and clang.

## Virtualized lists (6.5)

The app model's list protocol (`count`, `view`, a row's uid, fields by uid),
both ends, without a Telegraph: 6.6 wires them in. Decided 2026-10-20 ("memory
is a priority, but i am willing to spend it on large speed gains. This is a
hot path"): the four suggestions below.

- **A row source,** `ListRows<Source, Out>`: `count()`, `uid_at(row)`,
  `publish(out, uid)`, checked by a concept, so a missing member is a compile
  error at `t.list`. `list_source<Out>(source)` turns it into a `ListSource`:
  a `void*` and three captureless lambdas converted to function pointers.
  No virtual, no `std::function` (&sect;2.1), nothing allocated; the
  Telegraph can hold lists of different types in one table. Chosen over a
  base class with virtuals (inheritance for extension, a vtable call a row).
- **`ModelList`, the model's end:** keeps the uids it last sent for the rows
  in view (`window_`, 4 bytes a row) and where the view is. `show(first,
  rows)` reads the new rows' uids into a scratch area, sends `state
  name[row] uid` for each row whose uid is not the one sent for it, then
  `publish(uid)` for each item not in the old view. Scrolling keeps an item
  at its row, so that is checked first; only the others search the old view
  (at most 128 compares each). So a scroll of k rows costs k rows and k
  items; a sort of 10,000 with the same items on screen costs their rows
  only; nothing changed costs nothing. Chosen over resending every row and
  field in view on each change: no memory, but 30 rows of 5 fields is 150
  telegrams into a 256-slot ring, the burst that makes the model wait
  (priority 3 bought with priority 1 on a hot path: about 8 KB at
  `TELEGRAPH_LISTS` 16 &times; `TELEGRAPH_LIST_VIEW` 128). The scratch area
  is one per thread, shared by its lists (512 bytes), not one per list.
- **Count and view:** `changed()` sends the count when it changed, then reads
  the same view again, cut to the new count. `view` is cut to the count and
  to the window's size; an empty view sends nothing. `changed(uid)` publishes
  an item again only when it is in the view (a linear search, 128 at most).
- **Names:** `inventory.count` is `name_hash(".count", inventory.id)`, the
  name hashed on from the list's, nothing copied. A row is the list's name
  with the row as its uid, so row 0 is written `inventory` (uid 0 means none;
  the 6.1 note). Uids from a source must not be 0 (asserted).
- **`ListWindow`, the widget's end:** the window is the rows on screen plus
  a margin of one screen each side, as far as `most_rows` allows (no margin
  when the screen alone fills it). It is asked for again when the screen
  comes within half a margin of an edge that is not the list's own end, when
  the rows on screen change (a rotation), or when the count changes; at most
  once a call, and the widget calls once a frame. A slow scroll therefore
  asks every half screen (1,000 rows at a row a frame: about 62 asks), a
  fling every frame. Holds no rows: they are in the latest-value cache, the
  window says which to keep bound. 24 bytes.
- **What 6.6 adds:** encoding `view` as a telegram (two ints: the value type
  is the Telegraph's to choose), dispatch to the right `ModelList` by name,
  the cache's bindings following `window()`, and the knobs.
- **Standard library:** `std::span`, `std::optional`, `std::find`,
  `std::copy_n`, `std::min`, a concept from `<concepts>`.
- **Patterns:** none from the catalogue; the source is static polymorphism
  (a concept) lowered to function pointers where a table needs one type.
- **Tested** (`test/list_test.cpp`): the concept refusing a type with no
  `uid_at`; start (the count, by its hashed name); a view of 30 (30 rows, 30
  items); the same view again (nothing); a scroll of 10 (10 and 10); a
  change with none (nothing); the view reversed (30 rows, no items); a
  shuffle of all (exactly the rows that moved and the items that are new);
  one item on screen and one off; a shrink to 15 (the count, the view cut),
  a view past the end, an empty view; the window's size (128 of 1,000); no
  allocation over 2,000 frames of asking, answering and changing. The window:
  no count, the first ask, nothing when still; a slow scroll of 1,000 rows
  always covered, 60 to 70 asks; back up inside, nothing; a fling of 100 rows
  a frame, every frame covered; a rotation; the list's end, then nothing; a
  new count; more on screen than a window. Together, through a stand-in for
  the cache: a fling to row 9,000 (61 frames, 10,920 telegrams), a shuffle
  while on screen, and back up 40 rows a frame; every row on screen has its
  item and the item its field, every frame. Clean under AddressSanitizer and
  UndefinedBehaviorSanitizer. Five planted bugs each fail it: a changed row
  not resent, fields published for items already in view, asking at the
  list's end, `changed` not cutting the view to the count, a rotation not
  asking. g++ and clang.

## The Telegraph, single-threaded (6.6a)

`TelegraphLine`, `Telegraph` and `TelegraphUi`, in `telegraph.hpp` and
`telegraph.cpp`. Decided 2026-10-20 ("no change. implement as two
packages"); the thread, its wakes and the bounded waits are 6.6c.

- **One object, two ends.** `TelegraphLine` holds every array (rings,
  cache, pending table, list windows) as members sized by the knobs in
  `src/telegraph_config.hpp` (`SQUARED_TELEGRAPH_*`, with defaults and
  `static_assert`s), and the two ends as members. The model is handed
  `model()` (a `Telegraph&`), the Stage `ui()`: each end holds only the line,
  and neither exposes the other, so nothing points across (the app model's
  rule 5). Their constructors are private to the line. 87,936 bytes at the
  defaults; made once at start, with `make_unique` (it is 64-byte aligned:
  the rings' counters). Priority 1: known before `main` runs, nothing after.
- **Sending by key.** Every send is a template constrained by `sendable`
  (6.1b) and the key's uid flag, so a mistake is a compile error, then
  `detail::pack` makes the telegram and its bytes; the rest is not a template
  (`send_state`, `send_event`, `push`). `answer` copies a `do`'s correlation
  onto its `event`. Correlations are 1 to 65,535, then round again, and a
  refused `do` does not use one up.
- **States held back.** A `state` the ring cannot take goes to the pending
  table: a second `ValueCache` (no new code), each entry bound so it is never
  evicted. A state for an address already held goes there too (coalesced:
  the latest is the one that arrives); another address may go straight on
  when there is room, since between addresses the order of states does not
  matter. Every send and every turn tries to flush first: the walk is
  `bound_from` (added to `ValueCache` with `bindings`; each flushed entry is
  unbound and left to be evicted). Pending text is as large as a ring's data,
  so whatever a ring takes the table can hold (both refuse over a quarter).
  When every entry is held, `full`, after the model's thread waits for the
  UI to drain (6.6c, below).
- **Events never overtake states.** An `event` is not held back (each one
  counts); while states wait it is `full`. Single-threaded, this guard and
  the state's "same address waits behind" are reached only when the flush
  just failed, so the ring is full and the push would fail anyway; they
  matter once the UI drains between the flush and the push. So a planted bug
  in the event guard passes the single-threaded test; the two-thread test of
  6.6c catches it.
- **Lifecycle as bits.** The UI end ORs a bit into one atomic word (low
  memory, background, foreground, quit) and stores whether the app is now
  visible and the time away. The model's turn takes the word and calls the
  hooks: low memory first; if both background and foreground came, the one
  that ends where the app is goes last; quit last, after a `background` if
  the app was visible (quit saves), and `quitting()` turns true. Not through
  the ring, which could be full; a signal is never lost. Hooks are optional,
  found with `requires`.
- **Lists wired in.** `list(rows, source)` puts a `ModelList` in a fixed
  table (`std::optional`, `TELEGRAPH_LISTS` of them) over that slot's window
  and the shared scratch, with its view name worked out once
  (`name_hash(".view", rows.id)`), and sends the count. A turn routes a `do`
  with two floats to the list whose view name matches; everything else goes
  to `model.receive`. The list's own telegrams go through `send_state`, so
  they are held back like any state.
- **The UI's end** drains at most a ring's worth a call: states into the
  cache (a value the cache cannot keep is counted there), events to the
  handler's `event` if it has one. `value(key)` reads the cache as the key's
  type; a uid list is read with `find` (its bytes may be unaligned).
- **Not yet:** `set` coalesced per frame (the Stage's frame, Sprint 7); the
  looper watching `wake_fd` (Sprint 7); names registered for the text form
  (the dev-log tap, 6.7).
- **The knobs as make variables:** `mk/pkg_squared_core.mk` passes
  `TELEGRAPH_<KNOB>=<n>` as `-DSQUARED_TELEGRAPH_<KNOB>=<n>` to the app, the
  framework (`CXXFLAGS_EXTRA`) and the framework's flags stamp, only for the
  ones set: the app makes the line, so both must agree on its layout, and a
  change rebuilds both.
- **Standard library:** `std::array`, `std::optional`, `std::atomic`,
  `std::span`, `std::unique_ptr` (in tests and the App only). Passed over:
  `std::function` for the model (a template and `requires`), a virtual base
  class for the model (the façade of Sprint 7 may add one; nothing here needs
  it).
- **Patterns:** the line is a Mediator between the two ends, kept to
  storage; the pending table is a coalescing buffer.
- **Tested** (`test/telegraph_test.cpp`, the model inline): ten sends that
  must not compile and the ones that must; start, `set` round trip, `do` with
  text and with nothing answered with correlations 1 and 2, `do` with a uid,
  every value type read back as its type; a ring's worth of states then 1,000
  more on one address (one pending), an event refused while it waits, text
  held, the flush sending only the latest; a newer state behind an older one
  for the same address; the pending table full of distinct addresses
  (exactly 256, then `full`); too large both ways; the UI's ring full (`full`,
  never waits) and a refused `do` keeping its correlation; a list declared
  once, asked by a `ListWindow`, answered, shuffled, one item on and off
  screen; every lifecycle order, and a model with no hooks; no allocation over
  2,000 frames through both ends. Clean under AddressSanitizer and
  UndefinedBehaviorSanitizer. Planted bugs caught: a flushed state never
  unbound, a list's view under the wrong name, a coalesced state counted as
  new, a refused `do` using a correlation, background and foreground in a
  fixed order. Not caught single-threaded, as above: an event overtaking held
  states (caught on two threads, 6.6c). g++ and clang.

## The model's thread (6.6c)

`ModelRunner<M>` (`model_runner.hpp`, `src/model_runner.cpp`), the wakes
(`src/wake.hpp`, `src/wake.cpp`) and the bounded waits (`telegraph.cpp`).
Decided 2026-10-21 ("we go with these changes"): the model waits 100 ms for
room, then `full`, counted; a `std::thread` named `model`, default stack; the
eventfd logic tested with plain `poll` on two threads under ThreadSanitizer,
the looper's wiring left to Sprint 7.

- **Wakes.** `Wake` is one `eventfd` (`EFD_NONBLOCK | EFD_CLOEXEC`):
  `signal` writes 1, `clear` reads the counter back to 0, `wait` polls it.
  The line has two, the model's and the UI's. Chosen over a pipe (one
  descriptor, not two, and signals coalesce in the counter), a
  `std::condition_variable` (it needs a mutex the UI would take, and it
  cannot be polled), and `std::atomic::wait` (it cannot be polled either):
  the UI's wake must sit in the same `poll` as the looper's other
  descriptors (`ALooper_addFd`, Sprint 7), and the model's may later share a
  `poll` with a pty or a child's pipes. Priority 2: one wait for everything a
  thread sleeps on. If `eventfd` fails, `fd()` is -1 and a wait only times
  out: degraded, never stuck forever.
- **No wakeup lost.** Three flags on the line: `model_sleeping_` (until the
  UI sends or signals), `model_waiting_` (until the UI drains), and
  `ui_sleeping_` (until the model sends). The sleeper stores its flag, fences
  (`seq_cst`), looks for work, and sleeps only if there is none. The sender
  pushes, fences, reads the flag, and signals if it is set. With a fence on
  each side, one of them sees the other: either the sleeper's fence comes
  first in the single total order, and the sender's read sees the flag, or
  the sender's comes first, and the sleeper's look sees the push. The sender
  takes the flag (`exchange(false)`) as it signals, so a burst of sends costs
  one system call, not one each. A stale signal can only cause one extra
  turn.
- **Where the fences are.** The UI fences on every send and signal (input:
  a few a frame) and once after a `drain` that took something. The model does
  not fence per telegram: each push to the UI reads `ui_sleeping_` relaxed
  and signals if it is set (the quick look), and the fenced look happens at
  the end of every turn (`idle`) and before every wait. So a UI that fell
  asleep a moment ago is woken at the turn's end at the latest. Priority 3: a
  fence costs tens of nanoseconds, and a simulation's turn may send thousands
  of states. Priority 1 is untouched either way.

| Who | Wakes | When |
|---|---|---|
| UI `set`, `do_`, `view` | the model, if sleeping | after the push |
| UI lifecycle (`background`, `foreground`, `low_memory`, `quit`) | the model, if sleeping or waiting for room | after the bit is set: quit ends a wait |
| UI `drain` | the model, if waiting for room (or sleeping on held states) | after it took one or more |
| model push to the UI | the UI, if sleeping | quick look on each push; fenced look at each turn's end and before each wait |

- **`idle(timeout)`**, the model between turns: flush what is held; set
  `model_sleeping_`, and `model_waiting_` too if states are still held;
  fence; wake the UI for what this turn sent; then look for work: a
  telegram from the UI, a lifecycle bit, or (if states are held) a flush that
  now sends some. Sleep on the model's wake only if there is none, until
  signalled or `timeout` (the next tick; -1 for none). Clear both flags.
- **Bounded waits.** `send_state` and `send_event` try once (`try_state`,
  `try_event`: the 6.6a bodies), and on `full` call `wait_for_room` with that
  try as the attempt. Inline (no runner thread) or with `TELEGRAPH_WAIT_MS`
  at 0, it returns at once. Otherwise it counts a wait and loops: set
  `model_waiting_`, fence, wake the UI (room frees only when it drains), try;
  stop on success, on a quit bit (peeked, not taken: the next turn takes
  it), or at the deadline; else sleep on the model's wake for the time left
  and clear it. A wait that ends unsuccessful is a `full`, counted
  (`full_`); inline `full`s are counted too. Both counters are atomics
  written by the model, read by `wait_stats()`. An event still never
  overtakes held states: its attempt is the whole guarded try.
- **`ModelRunner<M>`.** On its own thread: it claims the line (one runner
  to a line, asserted), sets `threaded_` before the thread starts (the
  thread's start orders it), and the thread's body names the thread, makes
  `M` in the runner's `std::optional<M>` from copies of the arguments (an
  init-capture pack: `std::thread`'s rule, `std::ref` for a reference), runs
  `start`, then turns until `quitting()`: `deliver`, `tick` if due, `idle`
  until the next tick. It destroys `M` on the same thread. The model lives in
  the runner, not on the thread's stack: a large model cannot overflow the
  stack, and its size is part of the App's footprint, known before `main`
  (priority 1). The destructor calls `quit()` (which wakes the model, waiting
  or sleeping) and joins. Inline: `M` is made in the constructor on the
  calling thread and started; `turn()` is `deliver` then `tick` if due; the
  destructor quits and runs one last turn, so the model saves either way.
- **Ticks.** `detail::TickClock`, on the steady clock: the first tick comes
  a whole interval after an interval is first seen or changed; each step is
  the time since the last, clamped to 0.25 s (a debugger, a suspended app);
  a clock that fell behind starts the cadence again instead of bursting to
  catch up. A model without `tick` ignores `tick_every`.
- **The thread's name** is set with `pthread_setname_np` (glibc and bionic
  both; 15 characters at most), for `top -H`, `ps -T` and debuggers.
- **Standard library:** `std::thread`, `std::optional` (the model in place),
  `std::chrono::steady_clock`, `std::atomic` and `std::atomic_thread_fence`.
  Passed over: `std::condition_variable` and `std::atomic::wait` (above),
  `std::function` for the model's hooks (a template and `requires`), and raw
  `pthread_create` (`std::thread` is the standard spelling, and its one
  allocation, below, happens once at start).
- **Patterns:** the runner with the ring is an Active Object: the model's
  thread and its queue, the UI's sends as its requests. The line stays a
  Mediator.
- **Known limitations.** `eventfd` is Linux's, which Android is; another
  platform needs only its own `Wake`, the one class to port. ThreadSanitizer
  does not model standalone fences (GCC says so with `-Wtsan`): it checks the
  data, which acquire and release on the ring counters order, while the
  flags' protocol rests on the argument above and on the planted bugs below.
  Android's clang ships no ThreadSanitizer runtime, so it runs on desktop
  Linux. The flags share cache lines with the rest of the line: each is
  written once a sleep, so 128 bytes of alignment would buy nothing
  measurable.
- **Tested** (`test/telegraph_thread_test.cpp`, every wait bounded so a lost
  wakeup fails a `CHECK` instead of hanging): the tick clock alone (due,
  clamped, no burst after a stall); 2,000 round trips, each a sleep and a
  wake on both sides, with no allocation on either; an idle model using
  under 50 ms of CPU in 200 ms; `ready_to_sleep` refusing while something
  waits; ticks at 10 ms counted over 150 ms; the wait timing out (a ring's
  worth sent, then `full` after at least 95 ms, counted); the wait ended by
  drains (three rings' worth sent, none refused, well inside one deadline);
  60 rounds of 300 held states and an event, the UI draining meanwhile, the
  event never ahead of a state; 20 rounds of 300 states alone, the held ones
  arriving with nothing sent after them; `low_memory` waking a sleeping
  model; quit saving once, the model made and destroyed on its own thread,
  named `model`; quit ending a wait for room (1,700 sends that would take
  170 s, done within 60 ms); the inline runner (`full` at once, counted, no
  wait; saving at its end). Clean under ThreadSanitizer, AddressSanitizer and
  UndefinedBehaviorSanitizer (g++; this host's clang has no sanitizer
  runtimes) and warning-free with g++ and clang, 40 runs serially and 32 four
  at a time. Planted bugs caught, each by its own check: no wake on a
  UI send, no wake on a drain, quit not ending a wait, a lifecycle signal
  waking only a sleeper, an event overtaking held states (the 6.6a guard),
  a model that spins, the UI never woken by the model, no wait for room, a
  wait with no deadline, ticks unclamped, and `idle` forgetting what it
  holds.

## Memory

| Type | Bytes |
|---|---|
| `Name` | 8 |
| `Address` | 16 |
| `NameTable` | 24, plus the storage it is given |
| `Key` | 16 (a constant) |
| `Telegram` | 32 |
| `TelegramData` | 24, plus the storage it is given |
| `TelegramRing` | 192 (three 64-byte lines: shared, producer's, consumer's), plus the storage it is given |
| `ValueCache` | 88, plus the storage it is given |
| `ModelList` | 96, plus 4 bytes a row of its window (and a shared scratch as large) |
| `ListWindow` | 24 |
| `TelegraphLine` | 87,936 at the default knobs, every array inside (the two ends are a reference each); 6.6c's flags, counters and two descriptors fit its padding |
| `Wake` | 4 (a descriptor) |
| `WaitStats` | 8 |
| `ModelRunner<M>` | 40 plus `std::optional<M>` (`M` and a flag, rounded to `M`'s alignment): the line's reference, the tick clock, the thread; 48 for an empty model |

Nothing allocates. At the spec's defaults a ring's storage is 8 KB of slots
(`TELEGRAPH_RING`, 256) and 8 KB of data (`TELEGRAPH_DATA`), 16 KB a
direction, and the cache's is 16 KB of entries (`TELEGRAPH_CACHE`, 512),
2 KB of index and 8 KB of text (`TELEGRAPH_CACHE_TEXT`), 26 KB (the spec
estimated 24; the index is the difference). The Telegraph (6.6) will own
them. CDDA-sized (4,096 entries, 64 KB of text): 128 + 16 + 64 = 208 KB.
Lists: 16 of 128 rows, 16 &times; (96 + 512) bytes and one 512-byte
scratch, about 10 KB.

The model's thread (6.6c) adds, once at start: `std::thread`'s one heap
allocation (its state: the body and the copied arguments, tens of bytes),
two `eventfd`s, and the thread's stack, a mapping (8 MB of address space on
glibc, about 1 MB on bionic) of which only the pages the model touches are
resident. Nothing after.
