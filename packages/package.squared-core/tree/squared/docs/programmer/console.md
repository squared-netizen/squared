# console

The Console: text in, text out, for when the rest of the program is what's broken. It is being built in Sprint 4; this page grows with it. So far it has the **scrollback**, the ring of lines the Console shows, and the **build switches**.

Developer side: [../developer/console.md](../developer/console.md).

## Turning it on

The Console is off unless you ask for it, and then it is compiled in, with its sizes fixed at compile time:

```sh
make CONSOLE=app DEV_LOG=1 install                  # the in-app console
make CONSOLE=monitor DEV_LOG=1 install              # plus the monitor process (Sprint 4.7)
make CONSOLE=app CONSOLE_SCROLLBACK=1048576 install # a bigger history
```

| Setting | Default | Meaning |
|---|---|---|
| `CONSOLE` | `0` | `0`: none of it is linked, no flag reaches the compiler. `app`: the in-app console. `monitor`: `app`, plus a monitor process that survives the app's crashes |
| `CONSOLE_SCROLLBACK` | `65536` | Bytes of history: about 1,000 lines. At least 1024, a multiple of 4 |
| `CONSOLE_COMMANDS` | `64` | Commands the table holds, 1 to 1024 |

In code, `SQUARED_CONSOLE` is defined (1 for `app`, 2 for `monitor`) when it is on, and `sq::console::scrollback_bytes` and `sq::console::command_slots` hold the sizes. A wrong value is a build error, not a surprise on the device.

## Scrollback

`#include <squared/console/scrollback.hpp>`

| Type | Header | Purpose |
|---|---|---|
| `Scrollback` | `squared/console/scrollback.hpp` | The ring of lines: pushed in, read oldest first, the oldest dropped when full |
| `Line` | `squared/console/src/line.hpp` | One line as the ring hands it out: `text` and `level` |
| `Level` | `squared/console/src/level.hpp` | `info`, `warning`, `error` |

```cpp
std::array<char, sq::console::scrollback_bytes> storage;
sq::console::Scrollback lines(storage);

lines.push(sq::console::Level::error, "shader did not link\nsee the driver log");
for (const auto line : lines) { draw(line.text, line.level); }
for (auto it = lines.last(20); it != lines.end(); ++it) { copy((*it).text); }
```

The Console owns the storage and the scrollback; this is how it uses them, and how a test can.

| Call | Does |
|---|---|
| `Scrollback(std::span<char> storage)` | Uses `storage`, at least 64 bytes, for the lines. Kept, not copied. Never allocates |
| `push(Level, std::string_view)` | One line per `\n`-separated part; a trailing newline adds none. Drops the oldest lines until the new one fits; cuts a line longer than `longest_line()` |
| `begin()`, `end()` | The lines, oldest first, as `Line` values |
| `last(n)` | An iterator to the newest `n` lines, oldest of them first |
| `size()`, `empty()`, `dropped()` | Lines held; whether none is; lines dropped since construction |
| `clear()` | Forget the lines; `dropped()` is kept |
| `longest_line()` | The longest line kept whole: the storage less 4 bytes, at most 65535 |

**A `Line`'s `text` points into the storage.** It is valid until the next `push()` or `clear()`; copy it to keep it.

Nothing here fails: there are no error values. A line that doesn't fit drops older ones, and a line too long for the storage is cut.
