# messaging

The Telegraph's parts: names and addresses, keys, the 32-byte telegram and its
one line of text. The Telegraph itself (the rings between the UI and the model
thread, the latest-value cache) is being built in Sprint 6 on these; this page
grows with it.

Every type below lives in its own header. Include exactly the ones a
translation unit names; `squared/messaging/messaging.hpp` pulls in all of them
and exists for convenience, not for use inside headers of your own.

Developer counterpart: [../developer/messaging.md](../developer/messaging.md)

> [!note] The old dispatcher is gone (2026-10-20)
> `MessageDispatcher`, `EndpointId`, `MessageId`, the JSON-payload `Telegram`
> and the providers were removed: nothing used them, and the app model replaces
> them. Scheduled delivery became the model's timers (`sq::time::DeadlineQueue`).

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

## Keys

**Every address in an app is a key:** a name with the type of its value, the
verbs it carries, and whether it takes a uid.

```cpp
#include <squared/messaging/key.hpp>

namespace names {
using namespace sq::messaging;
inline constexpr Key<std::int64_t, state>                  counter_ticks{ "counter.ticks" };
inline constexpr Key<bool, state | set>                    settings_sound{ "settings.sound" };
inline constexpr Key<std::string_view, do_>                file_open{ "file.open" };
inline constexpr Key<void, event>                          game_over{ "game.over" };
inline constexpr Key<std::string_view, state, with_uid>    item_name{ "item.name" };
}
```

Write them by hand in a header, or let `names.json` generate them (Sprint 9e):
the same type either way. A dot in the name becomes an underscore in the
identifier.

| Value type | Spelled |
|---|---|
| none | `void` |
| bool | `bool` |
| integer | `std::int64_t` |
| number | `double` |
| two to four floats | `std::array<float, 2>` to `std::array<float, 4>` |
| text | `std::string_view` |
| a list of uids | `std::span<const std::uint32_t>` |

| Verbs | Direction |
|---|---|
| `state` | model to UI: a value is now this |
| `event` | model to UI: something happened |
| `set` | UI to model: the user changed a value |
| `do_` | UI to model: an intent (`do` in text) |

Combine with `|`: `state | set` is a value both sides change, like a check
box's.

**The compiler checks every send against the key.** The Telegraph's calls are
constrained by `sendable<Key, Verb, Value>`, so these do not compile:

| Mistake | Example |
|---|---|
| the wrong type | text to `counter_ticks` |
| a bool for an integer | `true` to `counter_ticks` |
| a verb the key does not carry | `set` on `counter_ticks`, `state` on `file_open` |
| a value for an address with none | anything to `game_over` |

A key is 16 bytes: the name (8) and a pointer to its text (8), for the name
table. Keys are constants; a telegram carries only the 8-byte name.

## The telegram

`sq::messaging::Telegram` (`squared/messaging/telegram.hpp`) is one message,
32 bytes: the name, the uid, a correlation (which pairs a `do` with the `event`
that answers it, 0 for none), the verb and value type, and 16 bytes of value.
Apps send through the Telegraph with keys; this is what travels underneath,
and what a replay file or a test makes by hand.

```cpp
using namespace sq::messaging;
const Telegram ticks = Telegram::integer( Verb::state, { "counter.ticks" }, 42 );
const Telegram open  = Telegram::data( Verb::do_, { "file.open" }, ValueType::text, where )
                           .with_correlation( 12 );
```

| Factory | Value |
|---|---|
| `empty(verb, to)` | none |
| `boolean(verb, to, b)` | a bool |
| `integer(verb, to, i)` | a 64-bit integer |
| `number(verb, to, d)` | a double; not a NaN (asserted: a NaN has no text form) |
| `floats(verb, to, span)` | two to four floats; no NaN |
| `data(verb, to, type, where)` | text or a uid list, by where it is in the data area (`DataRef`: offset and length in bytes, 4 per uid) |

Read it with `verb()`, `type()`, `address()`, `correlation()`, and
`as_boolean()`, `as_integer()`, `as_number()`, `as_floats()`, `as_data()` for
its type. `valid()` says whether 32 bytes that came from elsewhere
(`TelegramBytes::from`) are a telegram; the factories only make valid ones,
with every byte a value does not use zero, so equal telegrams are equal byte
for byte.

## Telegrams as text

Every telegram has one canonical line, the same on every device:

```text
set settings.sound false
do file.open "notes.txt" @12
state item[17].name "fur coat"
state player.pos (1.5 2)
state counter.ticks 2.0
state inventory [17 4 22]
event game.over @12
```

| Part | Written |
|---|---|
| the verb | `set`, `do`, `state`, `event` |
| the address | as in [the text form](#the-text-form) of an address: `item[17].name` |
| a bool | `true`, `false` |
| an integer | decimal |
| a number | the shortest form that reads back to the same bits, always with a `.` or an exponent so it is not an integer: `2.0`, `0.1`, `1e+21`, `-0.0`, `inf` |
| floats | `(1.5 2 0 1)` |
| text | in double quotes; `\\`, `\"`, `\n`, `\t`, and `\xNN` for any other byte below 0x20 or 0x7F; other bytes as they are, so UTF-8 reads as itself |
| uids | `[17 4 22]`; none: `[]` |
| a correlation | ` @12` at the end, when not 0 |

```cpp
std::array<std::byte, 4096> storage;
sq::messaging::TelegramData data{ storage };        // where text and uid lists go
auto telegram = sq::messaging::read_telegram( "do file.open \"notes.txt\" @12", data );

std::array<char, 512> line;
std::size_t n = sq::messaging::write_telegram( *telegram, data.bytes(), names, line );
```

| Call | Does | Fails with |
|---|---|---|
| `write_telegram(t, data, names, out)` | the line, into `out`; the length | 0: `out` too small, `t` not valid, or its data outside `data` |
| `read_telegram(line, data)` | the telegram; text and uids appended to `data` | nothing: not a telegram, or `data` full; nothing is left in `data` |

`read_telegram` needs no name table: names are hashed. It reads more than the
canonical form (more spaces, `[0]`, `1.50`, `007`), and writing what it read
gives the canonical line. It refuses a NaN, an integer out of range, a
correlation past 65,535, two to four floats only, and anything after the value.

`TelegramData` is a data area over storage you give it, filled from the front:
`append(bytes)`, `at(where)`, `bytes()`, `clear()`. Nothing in this module
allocates.

## Dependencies

None outside the standard library.
