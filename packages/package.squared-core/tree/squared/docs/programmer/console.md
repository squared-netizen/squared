# console

The Console: text in, text out, for when the rest of the program is what's broken. It is being built in Sprint 4; this page grows with it. So far: the **build switches**; what a `CONSOLE=app` build **shows** and how you **use it on the device**; the **`Console`** itself, its **commands** and built-ins, the **input line** with history and completion, and the **scrollback**; and the **drawing** parts, for drawing it yourself. Crash reports (4.6) and the monitor process (4.7) come next.

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

## What you see

With `CONSOLE=app` the Android template draws the Console over the upper half of your app, from the first frame. With `DEV_LOG=1` too, it shows the dev log of the main thread: the template's lines (`device:`, `driver:`, `graphics:`, `safe area`, the lifecycle), your own `sq::dev::log_line()` lines, and the `alloc:` reports; warnings in amber, errors in red. Without `DEV_LOG`, only the template's lines. Your app runs underneath as usual. A `CONSOLE=0` build has none of this.

| To | Do |
|---|---|
| open or close it | a three-finger tap anywhere, or `` ` `` on a keyboard |
| type into it | tap it: the soft keyboard shows (it covers the lower half; the Console is the upper) |
| run a command | type it, Enter. Tab completes a name; Up and Down walk the history |
| scroll back | drag it down; typing returns to the newest |
| paste | hold the input row for half a second, or Ctrl+V on a keyboard |
| copy | `copy`, `copy errors` or `copy 20` |
| close it | `[x]`, Back, or `close` |
| stop, start or restart your app | `app stop`, `app start`, `app restart` (`app` says which it is) |
| quit, app and Console together | `quit` |

`app stop` disposes your `App` and leaves a fresh one, not yet created, in its place; `app start` creates it and gives it the surface, as at launch. It all happens in this process, so it recovers from a stuck screen or bad state, not from a crash (a crash ends the process, Console included; that is what the monitor process, Sprint 4.7, is for). If your `App::create()` fails in a Console build, the Console opens instead of a black window, and `app start` tries again.

**Making it stick.** `CONSOLE` and `DEV_LOG` are read on each `make`, so a later plain `make install` builds without them. Put them in `local.mk` in your project, which git ignores, to keep them until you delete the lines:

```make
CONSOLE := app
DEV_LOG := 1
```

While it is open, a gesture that starts on it is the Console's, and while it has the keyboard, so are the keys (but not the volume keys); everything else reaches your app. The three-finger tap is taken before your app sees it: the fingers already down are released far off screen, so nothing under them is clicked. Typed text is what the keyboard's keys map to: ASCII and the accented Latin letters a keyboard has keys for; an input method's composition (accents built from two keys, CJK input, emoji) does not arrive.

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

Three commands are built in: `help` (all commands, or `help <name>` for one), `clear` and `close`. More come from the calls below, and the Android template adds them all.

| Call | Does |
|---|---|
| `show()`, `hide()`, `shown()` | Whether it should be drawn. A new Console is hidden; `close` and the close button hide it. The drawing code reads it |

## Built-in commands

```cpp
#include <squared/console/system_commands.hpp>

if (sq::console::add_system_commands(console) != 0) { /* the table was too small */ }
```

| Command | Prints |
|---|---|
| `mem` | `memory: resident 52.3 MB (peak 60.1 MB); the system has 2140.0 MB of 7660.2 MB available` ("about" on kernels before 3.14, which do not estimate it themselves) |
| `heap` | the allocator's bytes in use, free in its arena, and mapped |
| `threads` | `threads: 14` |
| `fds` | `open files: 37` |
| `device` | `device: <maker> <model>, Android 16 (API 36), arm64-v8a` (Android and Termux), then `kernel: Linux <release>, aarch64` |
| `smaps` | memory by kind, largest first: this program's code, the native heap, the GPU driver, the Java runtime, stacks, other libraries, other files, other anonymous memory. Each with PSS (this process's fair share of pages it shares with others), RSS (every page it touches, shared ones whole) and private dirty (pages only it has written: what it alone costs the device) |
| `gl` | the driver's version, renderer and vendor; the GLSL version; texture, renderbuffer and viewport limits, vertex attributes, texture units; the extension count. With the gles backend: `add_gl_command(console)`, from `renderer.hpp` |
| `display` | pixels, dpi, the Console's cell and grid, the safe area. Added by the Android template |
| `alloc` | the frame-allocation counter's figures, what `operator new` holds now (your code and squared's, apart from the GPU driver's and the C library's own `malloc`), and every site it found (`sq::dev::report_allocations()`). Added by the Android template with `DEV_LOG` |
| `app`, `quit` | stop, start or restart your `App`; quit. Added by the Android template |

| `copy` | `copy`: every line; `copy errors`: the error lines; `copy 20`: the last 20. The command's own echo is left out. `add_copy_command(console, writer)`, from `copy_command.hpp`; the Android template's writer is the system clipboard |

`add_system_commands` returns how many did not fit in the table (0 when all did); `add_gl_command` and `add_copy_command` return the table's `AddResult`. None of them allocates or keeps anything.

**`ClipboardWriter`** (`squared/console/src/clipboard_writer.hpp`) is where `copy` writes: a `context` and three functions, `begin(context)` (false: there is no clipboard), `append(context, text)` as often as needed, and `end(context)` (false: it refused). The text arrives in pieces, so nothing builds a copy of the whole scrollback first. `add_copy_command` keeps the writer by address: it must outlive the Console.

```cpp
std::string clip;   // a test's clipboard
const sq::console::ClipboardWriter writer{
    .context = &clip,
    .begin = [](void* c) noexcept { static_cast<std::string*>(c)->clear(); return true; },
    .append = [](void* c, std::string_view text) noexcept { static_cast<std::string*>(c)->append(text); },
    .end = [](void*) noexcept { return true; }};
static_cast<void>(sq::console::add_copy_command(console, writer));
```

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

## Drawing it yourself

The template does this for you. To draw the Console somewhere else, or in another engine's frame:

```cpp
#include <squared/console/renderer.hpp>

sq::console::Renderer renderer;                        // 16 KB; keep it with the Console
if (!renderer.create()) { /* no GLES 3.0: draw nothing */ }

// Each frame, after your own drawing:
renderer.draw(console, {.x = 0, .y = 0, .width = w, .height = h / 2, .cell = 24}, w, h);
```

| Type | Header | Purpose |
|---|---|---|
| `Renderer` | `squared/console/renderer.hpp` | Draws a `Console` with GLES 3.0. Built with the gles backend only |
| `Frame` | `squared/console/src/frame.hpp` | Where: `x`, `y`, `width`, `height` in pixels from the top left; `cell`, the character size in pixels; `scroll`, rows moved back from the newest |
| `layout()` | `squared/console/layout.hpp` | Where each character goes, as `Quad`s, without drawing: for another renderer, or a test |
| `grid(frame)` | `squared/console/layout.hpp` | The columns and rows a frame holds |
| `touches_close(frame, x, y)` | `squared/console/layout.hpp` | Whether a point is on `[x]` |
| `scroll_limit(console, frame)` | `squared/console/layout.hpp` | The largest useful `frame.scroll`: clamp a scroll you keep to it, so scrolling back down takes effect at once |

| Call | Does |
|---|---|
| `Renderer::create()` | Compile its shader, upload its 8 KB font, make its buffers. Needs a current context. Returns `false` if any of that failed, leaving nothing behind |
| `Renderer::draw(console, frame, width, height)` | Draw into `frame`, with a `width` by `height` pixel viewport. Nothing before a successful `create()`. Leaves blending on, the scissor off, and no program or buffer bound |
| `Renderer::invalidate()` | The context was lost with its objects: forget them. Then `create()` again |
| `Renderer::destroy()` | Delete its GL objects (also done by the destructor). The context must be current |

**Pick a cell that is a multiple of 8**: the font is 8 by 8 pixels, and whole steps keep it sharp. The template uses 8 for every 160 dpi. Characters outside ASCII show as `?`; their bytes are kept in the scrollback.

**`layout(console, frame, emit)`** calls `emit(const Quad&)` once per rectangle, back to front: background, title, lines, input row. A `Quad` is `x`, `y`, `width`, `height`, a `glyph` (an ASCII code; 127, `solid_glyph`, is a filled square) and a `colour` (`Colour::text`, `warning`, `error`, `dim`, `background`, `cursor`, `title`). It allocates nothing.

