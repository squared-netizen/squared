# console

The Console: text in, text out, for when the rest of the program is what's broken. It is being built in Sprint 4; this page grows with it. So far: the **build switches**, and the Console's working core without drawing: the **`Console`** itself, its **commands**, the **input line** with history and completion, and the **scrollback**. Drawing it on screen comes next.

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

## Console

`#include <squared/console/console.hpp>`

```cpp
// Once, at start: hold it in something long-lived. It is about 70 KB.
auto console = std::make_unique<sq::console::Console>();

console->add("spawn", "spawn <monster> [count]",
    [](sq::console::Console& c, const sq::console::Args& a, void*) {
        c.printf(sq::console::Level::info, "spawned %lld %.*s",
                 static_cast<long long>(a.integer(2, 1)),
                 static_cast<int>(a.text(1).size()), a.text(1).data());
    });

console->run("spawn goblin 3");          // or typed: text("spawn goblin 3"), then key(Enter)
```

| Call | Does |
|---|---|
| `add(name, help, handler, context = nullptr)` | Register a command. Returns `Commands::AddResult`: `added`, `table_full`, `duplicate_name`, `invalid_name` (empty, a space or a quote, or no handler) |
| `print(text, level = info)` | Add lines to the scrollback |
| `printf(level, format, ...)` | printf rules, into a fixed 512-byte buffer; a longer line is cut |
| `run(line)` | Echo `> line`, split it into words, call the command. Unknown names and too many words print an error line |
| `submit()` | Run the input line: remember it in the history, clear it, run it |
| `complete()` | Complete the command name being typed. Returns the number of matches: one fills it in with a space; several fill in the common part and print them |
| `key(Key, KeyModifiers)` | Left, Right, Home, End, Up and Down (history), Backspace, Delete, Enter (`submit`), Tab (`complete`), Escape (clear the line). Returns false for keys it leaves alone, Space among them: typed characters, a space included, come through `text()` |
| `text(utf8)` | Typed text, inserted at the cursor |
| `lines()`, `input()`, `commands()` | The scrollback, the input line, the table |

Two commands are built in: `help` (all commands, or `help <name>` for one) and `clear`.

**A command** is a plain function, `void(Console&, const Args&, void* context)`; a lambda that captures nothing converts to one. The `context` is what you passed to `add`. Names and help texts are kept as pointers, not copied: pass string literals, or text that lives as long as the Console.

**`printf`, not `{}` formatting.** `std::format` would add a large amount of code to every console build, and its convenient form returns a `std::string`, which allocates. `printf` formatting into a fixed buffer does neither.

Nothing the Console does allocates, and nothing fails with an error value: a full table refuses with `AddResult`, a long line is cut, unknown input prints an error line. One thread uses it.

## Args

`#include <squared/console/args.hpp>`. What a command receives: the words of its line, word 0 its name. Words are separated by spaces; double quotes group words (`say "two words"`). Every word is a view into the line being run, valid while the command runs.

| Call | Returns |
|---|---|
| `size()` | Words, the name included |
| `text(i)` | Word `i`, or empty |
| `integer(i, fallback = 0)` | Word `i` as a whole number (decimal, or hex with `0x`, a sign allowed), or `fallback` |
| `real(i, fallback = 0.0)` | Word `i` as a real number, or `fallback` |

At most 16 words, the name included.

## InputLine

`#include <squared/console/input_line.hpp>`. The line being typed, which the Console owns and edits through `key()` and `text()`; use it directly to show the text and the cursor, or to edit by hand. UTF-8: the cursor moves, and deletes, whole characters. 256 bytes; text beyond is cut at a character boundary, and control characters are dropped. The history holds about 50 earlier lines (2 KB): `previous()` and `next()` walk it, keeping the line you were typing to come back to; `remember()` adds the current line unless it is empty or repeats the newest.

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

The Console owns the storage and the scrollback, and the input line uses one for its history; this is how they use it, and how a test can.

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
