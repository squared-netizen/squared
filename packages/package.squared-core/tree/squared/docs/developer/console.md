# console &mdash; internals

The Console module (Sprint 4). Programmer counterpart: [../programmer/console.md](../programmer/console.md). Design: the app-model spec, "The Console: the escape hatch".

## Where it lives, and why there

`squared/console/`, a module of squared-core, built into `libsquared_console.a`. A project links it only when `CONSOLE` is `app` or `monitor` (`mk/pkg_squared_core.mk`); with `CONSOLE=0`, the default, the archive is built with the framework but never linked, and no `SQUARED_CONSOLE*` flag reaches the compiler, so the APK carries none of it.

It was planned as its own package, `package.squared-console` (decided 2026-10-15). It became a module instead (2026-10-16). `vendor_framework.sh`, and sqpg's packages, put squared-core's `squared/` in place whole, so a second package writing into `squared/console/` would be removed by the next vendoring. Overlaying packages, or a second include root that `make headers` and `make lint` would have to learn, is tooling for a second consumer that does not exist. As a module, the existing checks cover it unchanged. Priority 4 (extensibility) lost to 2 (one tree, one vendoring step); priority 1 was unaffected either way, since an unlinked archive costs nothing.

## Sizes

`console/src/limits.hpp` reads `SQUARED_CONSOLE_SCROLLBACK` (default 65536) and `SQUARED_CONSOLE_COMMANDS` (default 64), checks them with `static_assert`, and exposes them as `scrollback_bytes` and `command_slots`. The template passes both, from `CONSOLE_SCROLLBACK` and `CONSOLE_COMMANDS`, to the framework's build (in `CXXFLAGS_EXTRA`) and to the application's (in `SQ_CPPFLAGS`) alike, and includes them in the framework's flags stamp, so archive and application cannot disagree about a size.

## The Console's layout

`Console` is the model: state and behaviour, no drawing. The view that draws it (Drawing, below) only reads it. Everything is inline, so its size is its cost, fixed at compile time:

| Part | Bytes (defaults) |
|---|---:|
| scrollback storage | 65,536 |
| `InputLine` (text 256, the draft kept while browsing history 256, history 2,048, its `Scrollback` 56, counters) | 2,648 |
| `Commands` (64 entries × 32: name, help, function, context) | 2,056 |
| the `printf` buffer | 512 |
| the copy of the line being run | 256 |
| `Args` (16 views and a count) | 264 |
| the scrollback's own `Scrollback` | 56 |
| the shown flag, padded | 8 |
| **`sizeof(Console)`** | **71,336** |

Owned by its user, constructed once and long-lived; 70 KB is too much for a stack. Whether it is shown is a flag in the model (`show`, `hide`, `shown`), not in the view: the `close` command and the close button change the same state, and whoever draws it reads it. Nothing it does allocates (checked: 200 rounds of running, printing, completing and editing make no allocation, by the frame counter).

**Choices.**

- **printf over `{}` formatting** (the spec showed `c.print("spawned {} {}", ...)`). `std::format` pulls in a large amount of code per build that uses it, and its returning form allocates a `std::string`; `std::format_to_n` into a buffer avoids the allocation but not the code. `vsnprintf` into the 512-byte buffer costs neither and is checked by the compiler through `[[gnu::format(printf, ...)]]`. Priority 1 (code size, no allocation) over 2 (the nicer syntax).
- **A command runs on a copy of its line.** `submit()` copies the input into `running_` before running it, so the words a command receives stay valid even if it edits the input line, and the history and input are updated before the command can see them.
- **The table is scanned, not hashed.** A few dozen names, looked up when a person presses Enter: a scan of 64 entries costs nothing measurable, and any index would cost memory for nothing. Names and help are pointers to text that outlives the table (literals), not copies: 16 bytes an entry instead of a fixed name buffer.
- **The history is a `Scrollback`.** Same structure, same guarantees (fixed storage, oldest dropped, no allocation); `last(n)` gives the n-th newest line for Up. It is the second user of the ring, so the ring stayed general.
- **Completion** works on the command name only (the text before the first space), as typed: one match fills it in and adds a space; several fill in the longest common part and print the matches on one line, from the `printf` buffer.
- **Keys:** Space is left to typed text, because on Android a typed space arrives through the text-input path; taking the key too would insert it twice.

**Threads.** One thread uses a Console. The spec's one-slot mailbox for commands that must run on the model thread comes with the model thread (Sprint 6).

## Built-in commands

| Command | Where | Reads |
|---|---|---|
| `help`, `clear`, `close` | `src/console.cpp`, always | the table; the scrollback; the shown flag |
| `mem` | `src/system_commands.cpp` | `/proc/self/status` (`VmRSS`, `VmHWM`), `/proc/meminfo` (`MemAvailable`, `MemTotal`) |
| `heap` | same | `mallinfo` (bionic, older glibc) or `mallinfo2` (glibc 2.33 and later): `uordblks`, `fordblks`, `hblkhd` |
| `threads` | same | `/proc/self/status` (`Threads`) |
| `fds` | same | `/proc/self/fd`, through `getdents64` |
| `device` | same | system properties (`ro.product.manufacturer`, `.model`, `ro.build.version.release`, `.sdk`, `ro.product.cpu.abi`) on bionic; `uname` everywhere |
| `gl` | `src/gles/gl_command.cpp`, gles backend | `glGetString` (version, renderer, vendor, GLSL), `glGetIntegerv` (texture, renderbuffer and viewport limits, attributes, texture units, extension count) |
| `display` | the Android template | its own state: pixels, dpi, the Console's cell and grid, the safe area |
| `alloc` | the Android template, `DEV_LOG` only | `sq::dev::report_allocations()`, which arrives through the dev log's listener |

`add_system_commands()` and `add_gl_command()` register them; the template adds `display` and `alloc` itself, because only it holds what they read (and `dev` is linked only with `DEV_LOG`; the Console must not depend on it). A table too small for them refuses the rest, and the template logs how many once.

**Nothing is allocated, nothing kept.** Each command reads into stack buffers (4 KB for a `/proc` file, 2 KB of directory records) and prints through the Console's `printf` buffer. No `fopen` (stdio allocates its buffer) and no `opendir` (allocates its `DIR`): `fds` calls `getdents64` straight into the stack and counts names not starting with `.`, less the descriptor reading them. Sizes print with integer arithmetic (`%lld.%lld MB`), not `%f`: bionic's float formatting can allocate in its dtoa. Priority 1 over 3 (a few more lines).

**One file for Android, Termux and desktop Linux.** The interfaces are Linux's. What differs is the C library: bionic has system properties and `mallinfo`; glibc deprecated `mallinfo` for `mallinfo2` in 2.33. The two `#if` blocks test the C library (`__BIONIC__`, `__GLIBC__`), not the platform, because Termux is the posix platform on bionic: a Makefile switch on `SQUARED_PLATFORM` would pick the wrong branch there. Priority 2 (one file) over 4.

**`copy`** (`src/copy_command.cpp`) selects lines (all, the error lines, the last *n*; never its own echo, which is the newest line when it runs) and streams them to a `ClipboardWriter`: three function pointers and a context, the same shape as the command table's entries, not a virtual interface, since there is one writer per program (§5). The template's writer builds a Java string through JNI in 512-unit pieces, so the whole scrollback is never copied into a second buffer: a 64 KB buffer for a command run a few times a session would be priority 1 spent on nothing. Tested with a collecting writer: all, errors, last 2 (the echo excluded), more than there are, bad arguments, no clipboard, a refusal, an empty scrollback.

**Left for later.** `crash` needs the crash file (4.6); `telegraph` and `screens` arrive with their subsystems (Sprints 6, 7); `smaps`, `libs`, `thermal` and `storage` from the spec are not built yet (`libs` belongs with the crash report, 4.6).

## Drawing

Three parts, from data to pixels: the **font** and its atlas (`console/src/font8x8.*`), the **layout** (`console/layout.hpp`, headless), and the **renderer** (`console/renderer.hpp`, `console/src/gles/renderer.cpp`, gles backend only). The template's entry point puts them on screen (below).

### The font

`font8x8`: the 128 ASCII glyphs of Daniel Hepper's font8x8 (`font8x8_basic.h`, public domain, credited in the file), 8 bytes a glyph, 1 KB of read-only data. Bit 0 of each row byte is the leftmost pixel. Glyph 127 (DEL, blank in the font) is drawn as a filled square, `solid_glyph`: the background and the cursor are that glyph stretched, so the renderer needs no second shader or texture for rectangles.

`build_atlas()` expands it into a 128 by 64 texture of one byte a texel (16 glyphs by 8, 255 where a glyph has a pixel): 8 KB, made on the stack in `Renderer::create()`, uploaded as `GL_R8` and dropped. Sampled with `GL_NEAREST`, and the cell is always a whole multiple of 8 pixels, so every glyph pixel is a whole square of screen pixels.

### Layout

`layout(console, frame, emit)` is a template that calls `emit(Quad)` for every rectangle, back to front: the background, the title row (" console" and `[x]`), the lines, the input row (the prompt, the cursor block, then the text over it). It draws nothing and allocates nothing, so all of it is tested on the host by reading the quads back into a grid of characters. The emitter is a template parameter, not a function pointer or `std::function` (§2.1): the renderer's lambda inlines into the walk, and a test's lambda collects.

**The lines.** The view shows the newest wrapped rows that fit, moved back by `frame.scroll` rows (clamped to the oldest). Lines wrap at the grid's width, by character, not by word. The scrollback has no back links (see below), so the layout walks it twice, oldest first: once to count wrapped rows, once to emit those in view. That is O(scrollback bytes) a frame, about 130,000 byte reads for a full 64 KB scrollback, well under a millisecond; a cached row count would save it at the price of state the Console does not have today. Priority 1 (no state, no back links) over 3.

**Characters.** UTF-8 is walked by code point, so one character is one cell; anything outside printable ASCII is drawn as `?` (a tab as a space). A font of 128 glyphs cannot do better without a second font; the bytes are kept, so `copy` (4.4) copies them unchanged.

**The input row** shows as much of the line as fits, scrolled so the cursor is in view; the cursor is a block in its own colour, under the character it is before.

**Too small.** A frame under 8 columns or 3 rows gets only its background; a cell size of 0 or a frame smaller than one cell gets nothing.

| Type | Bytes | What |
|---|---:|---|
| `Quad` | 20 | position and size in pixels, a glyph, a `Colour` |
| `Frame` | 24 | where to draw (pixels, top left), the cell size, the scroll |
| `Colour` | 1 | a palette index: text, warning, error, dim, background, cursor, title |

### Renderer

**What it holds.**

| Object | Where | Bytes |
|---|---|---:|
| The vertex staging array (512 quads × 4 vertices × 8 bytes) | inside `Renderer` | 16,384 |
| GL names, uniform locations, a count | inside `Renderer` | 32 |
| **`sizeof(Renderer)`** | | **16,416** |
| The vertex buffer (`GL_STREAM_DRAW`, full size once) | driver | 16,384 |
| The index buffer (512 × 6 `uint16`, `GL_STATIC_DRAW`) | driver | 6,144 |
| The font atlas (`GL_R8`, 128 × 64) | driver | 8,192 |
| One program | driver | |

Transient, in `create()` only: the atlas texels (8 KB) and the indices (6 KB), on the stack.

**The vertex** is 8 bytes: position as two `int16` pixels, the glyph's texel corner as two `uint8` (the atlas is 128 by 64, so a corner fits a byte), the palette index, a pad byte. The sprite batch's 32-byte float vertex would be four times the upload for text that never rotates or scales by a fraction. The shader turns pixels into clip space from a `u_screen` uniform and looks the colour up in a 7-entry `u_palette` uniform in the vertex shader; the fragment shader is one texture read: `alpha = colour.a × texel`. Priority 1 (a quarter of the bytes) and 3.

**Batching.** `layout()` feeds `add()`, which writes four vertices into the staging array; at 512 quads it flushes: one `glBufferSubData` of what is filled and one `glDrawElements`. A full phone screen of text (45 by 100 cells, about 1,900 quads) is four draws. The indices never change (quad n is always 4n, 4n+1, 4n+2, 4n, 4n+2, 4n+3), so they are uploaded once.

**GL state.** `draw()` uses the program, the atlas on unit 0, and the default vertex array with attributes 0 to 2 (the sprite batch's own arrangement, which re-specifies its attributes on every flush); turns blending on with straight alpha and the scissor, depth test and face culling off. It leaves no program, array buffer or texture bound. It never touches the viewport: the caller's, the whole surface, is used.

**Raw GL, not the gles module's wrappers**, as the sprite batch does. `gles::VertexArray` takes float attributes only, and `gles::GlesError` owns a `std::string`; a failure here is a `false`, and its causes (a context without GLES 3.0, out of memory) need no text. The cost is about 40 lines of shader setup the wrappers would have saved. Priority 1 over 4.

**Context loss.** `invalidate()` forgets the names without deleting them (they belong to the lost context; deleting them would free unrelated objects in the new one); `create()` makes everything again. `destroy()`, also the destructor, deletes them.

### On screen: the template

The Android template's `entry.cpp` holds the Console under `#ifdef SQUARED_CONSOLE`, so a `CONSOLE=0` build has none of this:

- **Ownership.** `Console` and `Renderer` are members of the entry's `Platform`, which is allocated once: about 88 KB inside that one allocation, nothing more. They come after `graphics` in it, so the renderer's GL objects go before the context.
- **The log.** `SQ_LOGI` and `SQ_LOGW` format a line once into the stack. With `DEV_LOG`, it goes to the dev log with its level, and the dev log's listener, set to the Console while the platform exists, prints it: so every dev log line of the main thread reaches the Console, the application's own (`sq::dev::log_line`) and the allocation reports included. Without `DEV_LOG`, the template prints its lines into the Console directly; the application's lines then have nowhere to go but logcat. Warnings and errors keep their colour. Lines logged before the platform exists (the first, "starting") are not shown, nor are lines from other threads.
- **Commands.** The platform's constructor registers the built-ins (above) and shows the Console.
- **When.** The renderer is made as soon as the surface is ready, before the application's `create()`, because the Console is for when that goes wrong; after a resume that lost resources it is invalidated and made again. It draws after `App::render`, inside the frame-allocation count, so a console frame that allocated would be reported.
- **Where.** The upper half of the screen inside the safe area. The cell is 8 pixels for every 160 dpi of the screen's density, rounded (`AConfiguration_getDensity`): 24 on a 420 dpi phone, 45 columns across 1080 pixels.
- **Input.** In the template's `Platform` (`console_motion`, `console_key`), before the application sees anything:
  - **Three-finger tap** (decided 2026-10-17; edge swipes and corners collide with Android's own gestures): from the third finger down until every finger is up, the gesture is the Console's. The fingers already down are sent to the application as `PointerUp` far off screen, which ends their presses without a click (the GUI clicks only on an up inside the pressed widget); a fourth finger, a move past 2 cells, or more than 500 ms from the first finger down makes it not a tap.
  - **A gesture that starts on the Console** is all the Console's, down to the up, so nothing leaks to the application (4.2's `[x]` sent the application an unpaired up). Under the slop it is a tap: `[x]` closes, a hold of 500 ms on the input row pastes, anything else focuses (shows the keyboard). Past it, a drag: one row per cell of vertical movement, clamped with `scroll_limit`. Other fingers in the same event still go to the application.
  - **Keys:** backtick (no Ctrl, Alt or Meta) toggles it from anywhere. While it has focus every key is its own except volume and power: Back closes; Ctrl+V pastes; `Console::key` takes editing keys; anything else is turned into the character it types (`KeyCharacterMap`, through JNI) and inserted.
  - **Keyboard:** opening it, or tapping it, shows the soft keyboard; closing hides it unless the application's text field still wants it.
  - **State:** about 70 bytes in `Platform` (the touch, the three fingers, the scroll, the focus flag), and the `ClipboardWriter`. Nothing allocates per event; JNI runs per key press and per paste or copy.

### Tested

`console/test/layout_test.cpp`: the atlas (blank space, solid glyph 127, the bit order of `A`, only 0 and 255); the title, prompt and cursor of an empty console; lines in order and coloured by level; a 45-character line cut into three rows of 20; an empty line still a row; `é` and `€` each one `?` cell; tab and control characters; the newest at the bottom; scrolling back, clamped at the oldest and at 0; a wrapped line cut at the top and at the bottom of the view; the input row scrolled to keep the cursor in view, at both ends, with UTF-8; frames too small; the `[x]` hit test at its edges; 50 layouts of a full screen allocate nothing. Mutations of the row window, the cursor scroll, the `[x]` column, the size floor, the scroll clamp, the character range and the wrap bound each fail it.

`console/test/renderer_test.cpp`, with the gles backend, against `console/test/gl_stub.cpp`, a fake driver defined against the real GLES headers that keeps counters and fixed arrays only (so the draw's own allocations can be counted): nothing is called before `create()`; `create()` makes an R8 128 × 64 texture, a 16 KB and a 6 KB buffer, and puts the unpack alignment back; a 1,885-quad screen is drawn as four draws of every quad once, uploading at most 16 KB at a time, with no allocation; the first vertex is the background at the frame's corner; the state left is blending on, no scissor, no program, no array buffer; `invalidate()` deletes nothing and `create()` builds again; a shader that does not compile leaves nothing behind. Mutations of the flush, the batch limit, the final unbinds, the scissor, the unpack alignment, the font's bit order and the shader clean-up each fail it. It proves the call sequence, not the picture: that is the device check's job.

## Scrollback

**What it is:** a ring of variable-length line records in storage the owner provides. It owns no memory and never allocates.

**Record:** a 4-byte header (length as 16 bits, level, a zero byte), the text, padding to a multiple of 4. A line costs `4 + length`, rounded up to 4.

**Records never wrap around the end of the storage.** A record that does not fit above the tail ends the upper records there (`end_`), and the next one starts at 0, dropping the oldest until there is room below `head_`. So every line is contiguous, and handed out as a `string_view` into the storage, with no copy and no reassembly. The cost is the room above `end_`, at most one record's worth, unused until the head passes it.

| State | Records | Free |
|---|---|---|
| not wrapped | `head_ .. tail_` | `tail_ .. capacity` |
| wrapped | `head_ .. end_`, then `0 .. tail_` | `tail_ .. head_` |

| Operation | Time | Space |
|---|---|---|
| `push` of one line | O(lines dropped) plus the copy of its text | none |
| walking the lines | O(lines) | none |
| `last(n)` | O(size − n): it walks from the oldest | none |
| `clear` | O(1) | none |

`last(n)` walks forward because records carry no back links: a back link would cost 4 bytes per line, about 6% of a typical line, for a command (`copy 20`) a person types. Priority 1 over 3.

**Memory.** `sizeof(Scrollback)` is 56 on 64-bit (a span, two counters, four 32-bit offsets, a flag), checked with `static_assert( <= 64 )`. The storage is the Console's own array of `scrollback_bytes`: 64 KB by default, about 1,000 lines of 60 characters.

**Standard library.** `std::span` for the storage; `std::string_view` for lines. A `std::deque<std::string>` would allocate a node per line and a buffer per long line, and its size would depend on what was logged; the ring's is fixed at compile time.

**Tested** (`console/test/scrollback_test.cpp`, and `console_test.cpp` for the rest of the module: words, the table and its refusals, UTF-8 editing, history, completion, keys, the built-ins, cut `printf` output, no allocation, the shown flag and `close`, and the system commands against the real `/proc` and C library (every `mem` figure found, `threads: 1`, `fds` rising by one after a `dup`, the kernel line, and on bionic the device line); `renderer_test.cpp` also runs `gl` against the fake driver; `make -C console test`): against a plain model (a deque with the same byte budget) over 20,000 random pushes into five storage sizes from 64 to 4096 bytes: the ring always holds a suffix of the model's lines, with the right levels, and every push is accounted for as held or dropped. Exact-fit records are all kept, before and after a wrap; an over-long line is cut; pushing 2,000 lines and walking them allocates nothing (the frame counter). Mutations that break the wrap, the end marker, or the exact-fit comparison each fail the test.
