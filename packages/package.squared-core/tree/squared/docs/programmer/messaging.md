# messaging

The Telegraph's parts: names and addresses, keys, the 32-byte telegram and its
one line of text, the ring that carries telegrams from one thread to
another, the latest-value cache, both ends of a virtualized list, and the
Telegraph that joins them, with the runner that gives the model its own
thread. This page grows with the Telegraph.

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

## The ring

`sq::messaging::TelegramRing` (`squared/messaging/telegram_ring.hpp`) carries
telegrams from one thread to one other, in order, with no lock and no
allocation. The Telegraph runs one each way; an app sends through the
Telegraph, not through a ring. A tool or a test that moves telegrams between
two threads of its own can use one directly.

```cpp
using namespace sq::messaging;
std::array<std::byte, 256 * 32> slots;   // 256 telegrams
std::array<std::byte, 8192> text;        // their text and uid lists
TelegramRing ring{ slots, text };

// the producer's thread
const std::string_view name = "notes.txt";
const Telegram open = Telegram::data( Verb::do_, { "file.open" }, ValueType::text, {} );
if( ring.push( open, std::as_bytes( std::span( name ) ) ) == RingResult::full ) { /* later */ }

// the consumer's thread
while( const auto got = ring.front() ) {
    handle( got->telegram, got->data ); // data: the text, valid until pop()
    ring.pop();
}
```

| Call | Thread | Does | Fails with |
|---|---|---|---|
| `TelegramRing(slots, data)` | either, once | a ring over your storage: 32 bytes a telegram, a power of two of them, at least 2; data a power of two, at least 16 bytes, or empty for a ring with no text | asserted: sizes not powers of two |
| `push(telegram, data = {})` | producer | copies the telegram in; for `text` and `uids`, copies `data` into the ring too, and the telegram that arrives says where | `full`: no room now, nothing written; `too_large`: the value is over a quarter of the data ring and can never be sent |
| `front()` | consumer | the oldest telegram and its data; nothing when empty | &mdash; |
| `pop()` | consumer | done with the front telegram; its data's room is free again | asserted: the ring is empty |
| `stats()` | either | the sizes, and the most telegrams and data bytes held at once | &mdash; |

Exactly one thread pushes and exactly one reads. Neither ever waits: a full
ring says so, an empty one has no `front()`. A uid list is 4 bytes a uid.

## The latest-value cache

`sq::messaging::ValueCache` (`squared/messaging/value_cache.hpp`) keeps the
last value of each address, so a screen built later shows current values at
once. The Telegraph keeps one on the UI side and fills it from the model's
`state` telegrams; an app reads it through its screens, not directly. A tool
or a test can use one by itself.

```cpp
using namespace sq::messaging;
std::array<std::byte, 512 * 32> entries;                      // 512 addresses
std::array<std::byte, ValueCache::index_bytes( 512 )> index;  // 2 KB
std::array<std::byte, 8192> text;                             // text over 16 bytes
ValueCache cache{ entries, index, text };

const Address name{ "item.name", 17 };
if( !cache.bind( name ) ) { /* every entry bound: a larger cache */ }   // a label shows it
const auto fur_coat = std::as_bytes( std::span( std::string_view( "fur coat" ) ) );
const Telegram state = Telegram::data( Verb::state, name, ValueType::text, {} );
if( cache.store( state, fur_coat ) != CacheResult::stored ) { /* counted in stats() */ }
if( const auto got = cache.find( name ) ) {
    show( got->data );                                // "fur coat", this frame only
}
cache.unbind( name );                                 // the label is gone: may be evicted
```

| Call | Does | Fails with |
|---|---|---|
| `ValueCache(entries, index, text)` | a cache over your storage: 32 bytes an entry, a power of two of them (2 to 32,768); `index_bytes(n)` of index; text may be empty | asserted: wrong sizes |
| `store(telegram, data = {})` | makes the telegram's value the latest for its address; `text` and `uids` take their bytes from `data` | `dropped`: no room (every entry bound, or the text area full of bound text), counted, and the address keeps its old value; `too_large`: over a quarter of the text area |
| `find(address)` | the latest value: the telegram (no correlation; for text and uids its `DataRef` is `{0, data.size()}`) and its bytes | nothing: no value cached |
| `bind(address)` | an actor shows it: its entry is never evicted | false: every entry bound, or 255 binds already |
| `unbind(address)` | one actor fewer shows it | asserted: not bound |
| `stats()` | entries used and bound, text used and its highest, evictions, drops | &mdash; |

- **What gets evicted:** only addresses nothing is bound to, roughly oldest
  first, when room is needed.
- **Text from `find` lasts until the next `store` or `bind`** (the cache may
  move it to make room). Use it within the frame; copy it to keep it.
- **Text up to 16 bytes** lives in the entry and costs no text room.
- One thread only. Nothing allocates.

## The Telegraph

What a model sees: `sq::messaging::Telegraph` (`squared/messaging/telegraph.hpp`),
its end of the line to the UI. It sends `state` and `event` by key, declares
lists, and runs the model's side of a turn. The UI's end, `TelegraphUi`, is
the Stage's; both live in one `TelegraphLine`, which the App will own, and a
`ModelRunner` runs the model, on its own thread or the UI's.

```cpp
namespace names {
inline constexpr Key<bool, state | set> sound{ "settings.sound" };
inline constexpr Key<std::int64_t, state> ticks{ "counter.ticks" };
inline constexpr Key<void, do_> reset{ "counter.reset" };
}

struct Counter {
    std::int64_t ticks = 0;
    bool sound = true;
    void start( Telegraph &t ) { t.state( names::ticks, ticks ); t.state( names::sound, sound ); }
    void receive( Telegraph &t, const Received &m ) {
        if( m.telegram.address().name == names::reset.name() ) t.state( names::ticks, ticks = 0 );
        if( m.telegram.address().name == names::sound.name() )
            t.state( names::sound, sound = m.telegram.as_boolean() );
    }
    void low_memory( Telegraph & ) {}            // optional, like background and foreground
};

auto line = std::make_unique<TelegraphLine>();  // about 86 KB, once
ModelRunner<Counter> runner( *line );            // Counter on its own thread, named "model"

TelegraphUi &ui = line->ui();                    // the Stage's end
ui.set( names::sound, false );                   // wakes the model
ui.do_( names::reset );
ui.drain();                                      // each frame: into the cache
std::optional<bool> sound = ui.value( names::sound );  // false, once the model has answered
```

(The App, Sprint 7, does all but the struct for you; a model writes only its
struct.)

**The model's end, `Telegraph`:**

| Call | Does |
|---|---|
| `state(key, value)`, `state(key, uid, value)` | `state` an address; when the ring is full, held back and coalesced (`pending`): only the latest value per address goes, when room frees |
| `event(key)`, `event(key, value)`, `event(key, uid, value)` | `event`; never held back, and never ahead of states sent before it: `full` while any wait |
| `answer(request, key[, value])` | an `event` answering a `do`, with its correlation |
| `list(rows, source)` | a virtualized list (see [Lists](#lists)); its count is sent at once |
| `changed(rows)`, `changed(rows, uid)` | the list changed, or one item: what changed on screen is sent |
| `tick_every(ms)` | ask for `tick(t, seconds)` every `ms` (0, the default: none); `seconds` is the time since the last, at most 0.25 |
| `start(model)` | calls `model.start(t)`, once |
| `deliver(model)` | one turn: lifecycle hooks, held-back states, then each telegram from the UI to `model.receive(t, telegram)` (a list's `view` goes to its list) |
| `pending()`, `quitting()` | states waiting; whether quit was asked for |

A model is any type with `receive(Telegraph&, const Received&)` (the
`TelegraphModel` concept). Optional: `start(Telegraph&)`,
`tick(Telegraph&, double seconds)`, `low_memory(Telegraph&)`,
`background(Telegraph&)`, `foreground(Telegraph&, std::uint64_t elapsed_ms)`,
and for timers the pair `wake_at()` and `due(Telegraph&, sq::time::TimePoint now)`
(see [The model's thread](#the-models-thread)). On a turn they come in a
fixed order: low memory first; background and foreground in the order that
ends where the app is now; quit last, after `background` (quit saves).

**Sending mistakes do not compile:** a verb the key does not carry, a value
of the wrong type, a uid where the key takes none or none where it does.

**`SendResult`:** `sent`; `pending` (a state held back); `full` (no room: a
`set` or `do` from the UI at once; from the model, an `event`, or a state when
the pending table is full too, after its thread waited up to
`TELEGRAPH_WAIT_MS` for the UI to drain); `too_large` (over a quarter of the
data ring, never sendable).

**The UI's end, `TelegraphUi`:** `set`, `do_` (returns the correlation), `view`
(a list widget's rows), `drain` (states into the cache, events to a handler's
`event`), `find` and `value` (typed; text valid until the next `drain`),
`bind`/`unbind`, and `background`, `foreground(elapsed_ms)`, `low_memory`,
`quit` for the model's next turn. It never waits. Each send wakes a sleeping
model.

| Call | Does |
|---|---|
| `wake_fd()` | a descriptor that turns readable when the model sends while the UI sleeps: poll it with the others (the looper, an event loop); -1 if it could not be made |
| `ready_to_sleep()` | before sleeping on it: `false` when the model has sent already (drain instead) |
| `woke()` | after the sleep, woken or timed out |
| `wait_stats()` | `waits` (times the model waited for room) and `full` (sends it had refused) |
| `model_stats()` | the model's side as the UI may read it: `held` (states held back now), `highest_held`, `lists` declared |
| `cached_from(from)` | a walk over every value the cache holds: from 0, then from the last found's `entry` plus 1; each has its `address` and, unless only bound so far, its `value` (the Console's `telegraph state`) |
| `set_names(table)`, `names()` | a name table for the text form (the dev tools' own): each key the UI uses (`set`, `do_`, `view`, `value`) is added the first time. nullptr, the default, keeps none |
| `register_names(keys...)` | add these keys' names too: the ones only the model sends. Nothing without a table |
| `set_tap(tap, context)` | `tap(context, direction, telegram, data)` for every telegram the UI sends (`to_model`) and drains (`to_ui`), on the UI thread; its text or uid list in `data`, from 0. nullptr, the default: none |
| `send(telegram, data)` | a telegram read from text, sent as it is (a replay): no key checks it |

The Stage sleeps like this, when nothing is animating:

```cpp
if( ui.ready_to_sleep() ) {
    pollfd fds[] = { { ui.wake_fd(), POLLIN, 0 }, /* input, ... */ };
    poll( fds, 2, -1 );
    ui.woke();
}
ui.drain( screens );
```

**Capacities** are compile-time knobs, set as make variables in a project
(`make TELEGRAPH_RING=512`; the framework and the app are built with the same
ones) or `-DSQUARED_TELEGRAPH_RING=512` by hand: `TELEGRAPH_RING` 256
telegrams each way, `TELEGRAPH_DATA` 8 KB of text each way,
`TELEGRAPH_PENDING` 256 held states, `TELEGRAPH_CACHE` 512 values with
`TELEGRAPH_CACHE_TEXT` 8 KB, `TELEGRAPH_LISTS` 16 of `TELEGRAPH_LIST_VIEW`
128 rows, `TELEGRAPH_WAIT_MS` 100 ms of waiting for room (0: never wait),
`TELEGRAPH_NAMES` 8 KB of names for the dev tools (Console builds only: the
table is theirs, not the line's).
`RING`, `DATA`, `PENDING` and `CACHE` are powers of two; checked when the
framework compiles. `make status` shows the ones set.

## The model's thread

`sq::messaging::ModelRunner<M>` (`squared/messaging/model_runner.hpp`) runs a
model at the model's end of a line.

```cpp
ModelRunner<Shop> runner( *line, std::ref( database ) );   // its own thread
ModelRunner<Shop> runner( *line, on_ui_thread );           // or the UI's: runner.turn() each frame
```

| Call | Does |
|---|---|
| `ModelRunner<M>(line, args...)` | makes `M` from `args` on a new thread named `model` (the arguments are copied, as `std::thread` copies them: `std::ref` for a reference), calls `start`, then runs turns: `deliver`, then `tick` if due, then `due` if its time has come, then sleeps until the UI sends, a lifecycle signal, room for a held state, the next tick, or `wake_at()` |
| `ModelRunner<M>(line, on_ui_thread, args...)` | makes `M` now, on the calling thread; nothing runs until `turn()` |
| `turn()` | inline only: one turn, on the UI thread (ticks and timers that are due run in it); the telegrams handled, 0 once the model has quit |
| `threaded()` | whether the model has its own thread |
| `~ModelRunner()` | asks the model to quit (it saves: `background` if the app was visible), wakes it, and waits for it to end; `M` is destroyed on the thread it lived on |

Preconditions: one runner to a line, and the line outlives the runner. The
model's thread never touches the UI's end, nor the UI the model; on its own
thread the model is not reachable from the UI at all. Failure modes: none
returned; a thread the system cannot make ends the program at start.

**Waiting for room.** On its own thread, a model whose `event` finds the ring
full, or whose `state` finds the pending table full too, waits up to
`TELEGRAPH_WAIT_MS` (100 ms) for the UI to drain, then gets `full`. Inline,
the UI cannot drain meanwhile, so `full` comes at once. A quit ends a wait at
once. Both are counted in `wait_stats()`, which the `telegraph` Console
command (6.7) will show.

**Ticks** run on the model's thread at the interval `tick_every` asked for,
measured by a `sq::time::Timepiece` (see [time.md](time.md)): `seconds` is
the time since the last tick, at most 0.25, and the first tick measures
from when `tick_every` was called. A model that fell behind starts the
cadence again rather than catching up in a burst.

**Timers.** A model with timers says when it next needs a turn, and is
called when that time comes; it sleeps in between, ticking or not. Most keep
a `sq::time::DeadlineQueue`:

```cpp
std::optional<sq::time::TimePoint> wake_at() const { return timers.next_due(); }
void due( Telegraph &t, sq::time::TimePoint now ) {
    timers.poll_due( now, 16, [&]( Timer timer ) { fire( t, timer ); } );
}
```

| Hook | Called |
|---|---|
| `wake_at()` | after each turn's `deliver` and `tick`: the time the model next needs a turn, or nothing. The thread sleeps no later than that |
| `due(t, now)` | when that time has come, with the turn's one reading of the clock; then `wake_at()` is asked again |

A model with `wake_at()` and no `due` does not compile. The clock is read
once a turn, after `deliver`, for both the tick and the timers.

## Lists

A list of any length costs the UI only the rows on screen. The model holds
the items; the list widget asks for the rows it shows, and the model sends
those rows and their items' fields, nothing else. For a list named
`inventory`:

| Telegram | Means |
|---|---|
| `state inventory.count 300` | how many rows |
| `do inventory.view 20 52` | the widget shows rows 20 to 52, a screen above and below included |
| `state inventory[20] 17` | row 20 is the item with uid 17 (row 0 is written `inventory`) |
| `state item[17].name "fur coat"` | that item's fields, by its uid |

**What a model provides** for a list (the `ListRows` concept): any object
with three members. Uids are not 0.

```cpp
struct Inventory {
    std::vector<Item> items;
    std::uint32_t count() const { return std::uint32_t( items.size() ); }
    std::uint32_t uid_at( std::uint32_t row ) const { return items[row].uid; }
    void publish( Out &t, std::uint32_t uid ) { /* t.state( names::item_name, uid, ... ) */ }
};
```

A type missing one of them does not compile. The Telegraph takes it as
`t.list( names::inventory, inventory )`, and changes as
`t.changed( names::inventory )` (a sort, an insert, a removal) or
`t.changed( names::inventory, uid )` (one item's fields). The widget asks with
`ui.view( names::inventory, window )`, the `view` travelling as two floats:
`do inventory.view (20 52)`, exact up to 16.7 million rows. Only what is on
screen is ever sent: after a sort, the rows whose item changed, and the
fields of items new to the screen; nothing for the rest of the list.

`sq::messaging::ModelList` (`squared/messaging/model_list.hpp`) is the
model's end, `sq::messaging::ListWindow` (`squared/messaging/list_window.hpp`)
the widget's. The Telegraph and the list widget use them; an app does not.

| Call | Does |
|---|---|
| `list_source<Out>(source)` | a `ListSource`: the three members as plain functions |
| `ModelList(name, source, window, scratch)` | the model's end: `window` holds the uids sent for the rows in view (4 bytes a row, `TELEGRAPH_LIST_VIEW`); `scratch` as large, shared by every list on the thread |
| `start(out)` | sends the count |
| `view(first, last, out)` | answers a `view`: the rows whose item is not the one sent, the fields of items new to it; cut to the count and the window's size |
| `changed(out)` | the count if it changed, then the view read again |
| `changed(uid, out)` | the item's fields again, if it is on screen |
| `ListWindow(most_rows)` | the widget's end |
| `set_count(n)` | from `state inventory.count` |
| `visible(first, rows)` | each frame: the rows to ask for, when they should be asked for (the screen and one more each side, asked again when the screen comes within half a screen of an edge, or the count changes) |
| `window()` | the rows last asked for: the ones to keep bound in the cache |

Nothing allocates. The model's thread for `ModelList`, the UI thread for
`ListWindow`.

## Dependencies

The standard library; `time` (the runner's `Timepiece` and clock); and POSIX
as Linux and Android both have it: `eventfd` and `poll` for the wakes,
`pthread_setname_np` for the thread's name. An app that uses `ModelRunner`
links with `-pthread` (the project's make file does).
