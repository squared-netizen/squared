# messaging &mdash; internals

The Telegraph's parts, rebuilt in Sprint 6 to the app model: names and
addresses (6.1), keys (6.1b), the 32-byte telegram and its text form (6.2).
Nothing here allocates.

Programmer counterpart: [../programmer/messaging.md](../programmer/messaging.md)

## Layout

```
messaging/
  name.hpp address.hpp name_table.hpp key.hpp telegram.hpp telegram_data.hpp
  telegram_text.hpp messaging.hpp
  src/   verb.hpp value_type.hpp data_ref.hpp (supporting); address.cpp,
         name_table.cpp, telegram.cpp, telegram_data.cpp, telegram_text.cpp
  test/  address_test.cpp, telegram_test.cpp
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

## Memory

| Type | Bytes |
|---|---|
| `Name` | 8 |
| `Address` | 16 |
| `NameTable` | 24, plus the storage it is given |
| `Key` | 16 (a constant) |
| `Telegram` | 32 |
| `TelegramData` | 24, plus the storage it is given |

Nothing allocates; the rings and caches of 6.3 to 6.5 are sized at init.
