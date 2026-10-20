# console &mdash; internals

The Console module (Sprint 4). Programmer counterpart: [../programmer/console.md](../programmer/console.md). Design: the app-model spec, "The Console: the escape hatch".

## Where it lives, and why there

`squared/console/`, a module of squared-core, built into `libsquared_console.a`. A project links it only when `CONSOLE` is `app` or `monitor` (`mk/pkg_squared_core.mk`); with `CONSOLE=0`, the default, the archive is built with the framework but never linked, and no `SQUARED_CONSOLE*` flag reaches the compiler, so the APK carries none of it.

It was planned as its own package, `package.squared-console` (decided 2026-10-15). It became a module instead (2026-10-16). `vendor_framework.sh`, and sqpg's packages, put squared-core's `squared/` in place whole, so a second package writing into `squared/console/` would be removed by the next vendoring. Overlaying packages, or a second include root that `make headers` and `make lint` would have to learn, is tooling for a second consumer that does not exist. As a module, the existing checks cover it unchanged. Priority 4 (extensibility) lost to 2 (one tree, one vendoring step); priority 1 was unaffected either way, since an unlinked archive costs nothing.

## Sizes

`console/src/limits.hpp` reads `SQUARED_CONSOLE_SCROLLBACK` (default 65536) and `SQUARED_CONSOLE_COMMANDS` (default 64), checks them with `static_assert`, and exposes them as `scrollback_bytes` and `command_slots`. The template passes both, from `CONSOLE_SCROLLBACK` and `CONSOLE_COMMANDS`, to the framework's build (in `CXXFLAGS_EXTRA`) and to the application's (in `SQ_CPPFLAGS`) alike, and includes them in the framework's flags stamp, so archive and application cannot disagree about a size. The framework gets them in every build, `CONSOLE=0` included, so switching `CONSOLE` changes nothing it compiles (2026-10-19; [build-and-toolchain.md](build-and-toolchain.md#building-only-what-changed)).

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
| the mirror: a function pointer and its context (2026-10-19, for the bridge) | 16 |
| the shown flag, padded | 8 |
| **`sizeof(Console)`** | **71,352** |

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
| `mem` | `src/system_commands.cpp` | `/proc/self/status` (`VmRSS`, `VmHWM`), `/proc/meminfo` (`MemAvailable`, `MemTotal`); before Linux 3.14, which has no `MemAvailable`, `MemFree` plus `Cached`, printed as "about" |
| `heap` | same | `mallinfo` (bionic, older glibc) or `mallinfo2` (glibc 2.33 and later): `uordblks`, `fordblks`, `hblkhd` |
| `threads` | same | `/proc/self/status` (`Threads`) |
| `fds` | same | `/proc/self/fd`, through `getdents64`; on Linux before 3.17, where a release Android app may list it only from its main thread, `/proc/self/task/<tid>/fd` (found on the 5.1 tablet, 2026-10-19) |
| `device` | same | system properties (`ro.product.manufacturer`, `.model`, `ro.build.version.release`, `.sdk`, `ro.product.cpu.abi`) on bionic; `uname` everywhere |
| `smaps` | same | `/proc/self/smaps`, streamed through 4 KB on the stack: `Rss`, `Pss`, `Private_Dirty`, `SwapPss` summed by kind of mapping |
| `gl` | `src/gles/gl_command.cpp`, gles backend | `glGetString` (version, renderer, vendor, GLSL), `glGetIntegerv` (texture, renderbuffer and viewport limits, attributes, texture units, extension count) |
| `display` | the Android template | its own state: pixels, dpi, the Console's cell and grid, the safe area |
| `app`, `quit` | the Android template | stop, start, restart the `App` in place; finish the activity |
| `alloc` | the Android template, `DEV_LOG` only | `sq::dev::report_allocations()`, which arrives through the dev log's listener |

`add_system_commands()` and `add_gl_command()` register them; the template adds `display` and `alloc` itself, because only it holds what they read (and `dev` is linked only with `DEV_LOG`; the Console must not depend on it). A table too small for them refuses the rest, and the template logs how many once.

**Nothing is allocated, nothing kept.** Each command reads into stack buffers (4 KB for a `/proc` file, 2 KB of directory records) and prints through the Console's `printf` buffer. No `fopen` (stdio allocates its buffer) and no `opendir` (allocates its `DIR`): `fds` calls `getdents64` straight into the stack and counts names not starting with `.`, less the descriptor reading them. Sizes print with integer arithmetic (`%lld.%lld MB`), not `%f`: bionic's float formatting can allocate in its dtoa. Priority 1 over 3 (a few more lines).

**One file for Android, Termux and desktop Linux.** The interfaces are Linux's. What differs is the C library: bionic has system properties and `mallinfo`; glibc deprecated `mallinfo` for `mallinfo2` in 2.33. The two `#if` blocks test the C library (`__BIONIC__`, `__GLIBC__`), not the platform, because Termux is the posix platform on bionic: a Makefile switch on `SQUARED_PLATFORM` would pick the wrong branch there. Priority 2 (one file) over 4.

**`copy`** (`src/copy_command.cpp`) selects lines (all, the error lines, the last *n*; never its own echo, which is the newest line when it runs) and streams them to a `ClipboardWriter`: three function pointers and a context, the same shape as the command table's entries, not a virtual interface, since there is one writer per program (§5). The template's writer builds a Java string through JNI in 512-unit pieces, so the whole scrollback is never copied into a second buffer: a 64 KB buffer for a command run a few times a session would be priority 1 spent on nothing. Tested with a collecting writer: all, errors, last 2 (the echo excluded), more than there are, bad arguments, no clipboard, a refusal, an empty scrollback.

**`smaps`, and why** (asked for 2026-10-19: the demo showed 146 MB resident and 28 MB of heap, which looked high for what it does). RSS counts every page a process touches, shared ones whole: on Android that includes the zygote's Java runtime and framework, mapped into every app. PSS divides shared pages among the processes sharing them, and private dirty is what this process alone costs the device. So the command sums the three by kind of mapping, judged by name, first match winning:

| Kind | Mapping names |
|---|---|
| this program | the library holding `add_system_commands` (`dladdr`), so the app's own code and data |
| native heap | `[anon:scudo:*]`, `[anon:libc_malloc*]`, `[anon:jemalloc*]`, `[anon:GWP-ASan*]`, `[heap]` |
| GPU driver | anything naming `mali`, `kgsl`, `adreno`, `dmabuf`, `gralloc` or `/egl/`, and `/dev/dri*`, `/dev/ion` |
| Java runtime | `[anon:dalvik-*]`, `*.art`, `*.oat`, `*.vdex`, `*.odex`, `*.jar`, `/apex/com.android.art/` |
| stacks | `[stack]`, `[anon:stack_and_tls*]`, `[anon:thread signal stack*]` |
| other .so | `*.so`, `*.so.*` |
| other files | any other path |
| other anon | the rest |

The native heap is still one figure for everyone who calls `malloc`: squared, the application, libc++, and the GPU driver's CPU side. The `alloc` command splits it: in `DEV_LOG` builds the replacement `operator new` keeps what it holds now (each block by `malloc_usable_size`, relaxed atomics, two counters and a peak), so heap less that figure is everyone else's. Cost: three atomic operations per allocation and free, in development builds only.

**Left for later.** `telegraph` and `screens` arrive with their subsystems (Sprints 6, 7); `smaps`, `libs`, `thermal` and `storage` from the spec are not built yet (`libs` belongs with the crash report, 4.6).

## Crash file

`console/crash_file.hpp`, `src/crash_file.cpp`; the trace itself is `sq::dev::write_crash_trace` (`dev/crash_trace.hpp`, header-only, so a Console build without `DEV_LOG` does not link `dev`, whose replacement `operator new` must stay out of release builds).

**In the handler**, in this order: `open` the file (`O_CREAT | O_TRUNC`, 0600; made at the crash, so an earlier report survives until the next one); the trace through a sink that `write`s each line; then "log: the last N lines" and the newest N lines of the scrollback, walked with the ordinary iterator. The log comes last on purpose: if the crash caught the scrollback mid-write, walking it may fault again, and then only that part is lost (the signal is blocked inside its handler, so a second fault kills the process instead of recursing). Everything it touches is static: the path (512 bytes), the Console pointer, the count, the six earlier `sigaction`s, a 16 KB alternate stack (installed for the calling thread only if it has none: `sigaltstack` is per thread).

**Handing it on.** The earlier handler (in a dev build, the dev log's; under it, Android's) is put back, then: a fault (`si_code > 0`) simply returns, so the instruction faults again and the earlier handler gets the real fault address; a sent signal (`abort`, `raise`, `si_code <= 0`) is raised again and arrives when the handler returns. This is the order breakpad uses; calling the earlier handler directly would hand it our stack and a stale context.

**At launch** the template shows the report before installing the handler, then `rename`s it to `crash-previous.txt`, so a report is shown once and kept for `crash`; a crash during the next run overwrites `crash.txt`, never the kept one.

**Tested** (`test/crash_file_test.cpp`, `make -C console test`) with real crashes in child processes: a null write (SIGSEGV, code 1, address 0; frames #0 and #1, so the unwinder got past the handler; END; the log lines) with a handler installed before ours, which ran (a marker file); `abort()` (SIGABRT, raised again); shown once, moved, then `crash`; no report kept; empty and over-long paths refused; `remove_crash_file` puts the earlier handler back.

**`line_reader.hpp`.** `for_each_line` reads a descriptor through a caller's buffer, a line at a time, the last line counting without a newline, a line longer than the buffer in pieces. `smaps` and the crash file use it; a template, so each caller's lambda inlines.

## Bridge

`console/bridge.hpp`, `src/bridge.cpp`, the wire format in `src/bridge_protocol.hpp`; the Termux end in `client/sqconsole.cpp`; the template's wiring in `sq_android/entry.cpp` and the build's in `mk/pkg_squared_core.mk` (Sprint 4, 4.7t part 1, 2026-10-19). Decided with you that day: no adb on the phone (Wireless Debugging needs Wi-Fi it rarely has), Termux is the monitor first, the `:console` monitor app comes later for where Termux is not.

**Why TCP on 127.0.0.1.** Two apps must talk: the dev app and Termux, each its own Linux user. A file-system socket needs a directory both can reach, and none exists (each app's private directory is its own; shared storage is any app's). An abstract-namespace Unix socket is subject to SELinux's per-app categories since Android 9, and the dev log already proved loopback inet sockets work between the two. Loopback cannot be reached from off the device. The cost is the `INTERNET` permission in CONSOLE builds, as in DEV_LOG builds; plain builds keep none.

**The secret, and what it does not do.** The build makes 16 random bytes once per build directory (`od -N16 /dev/urandom`, hex, in `build/dev/bridge.conf` beside the port) and compiles them in as two 64-bit numbers (`SQUARED_BRIDGE_SECRET_HIGH`/`_LOW`: no quotes through make and the shell). The client sends it first; a mismatch is closed with no answer, compared in constant time. Any app can read any installed APK, so an app that goes looking can find the secret: this keeps out the merely curious, which is what a dev build on your own phone needs, and it is said so in the programmer docs. A per-install pairing code shown in the Console was the stronger alternative; it needs the Console and Termux on screen together, and was not worth it for dev builds.

**Waiting on events.** The bridge does nothing by itself: `open` asks the owner to watch the listening socket (`BridgeHandlers::watch`), and the owner calls `ready(fd, readable, writable)`. The template maps `watch` to `ALooper_addFd` with a callback on the glue's looper, so the sockets are served inside the `ALooper_pollOnce` the loop already blocks in, costing nothing while idle. A callback makes `pollOnce` return `ALOOPER_POLL_CALLBACK` (-2), which ends the template's inner poll loop; the outer loop then calls `frame()`, which returns at once without a surface, and polls again: no spin, no change to the loop needed. Writing is watched only while output waits.

**The client waits on an event too.** `sqconsole` binds UDP on the same port number before trying to connect; `Bridge::open` sends one datagram there ("squared-bridge up"). So a client started first, or one whose app restarted, connects when the app is up, with no retry timer. `--run` (`make console CMD=`) waits at most `--wait` seconds (30), saying so at once.

**Connections.** At most one client and one pending connection. The newest connection is the pending one (an earlier silent one is dropped); when it gives the secret it becomes the client, and the old client is told "another connection took over" directly (past its queue) and closed. Bytes after the hello are its first commands, so a scripted client sends hello and commands in one write. Each command runs through `handlers.command` (the template: `console.run`), then `.` marks its end: `--run` counts those to know when to exit. Commands are at most 256 bytes (`input_bytes`); longer ones run cut, the rest dropped.

**Output.** `send` formats `<type> <text>\n` into a 16 KB ring and writes immediately (`MSG_NOSIGNAL | MSG_DONTWAIT`, so a vanished client is an error, not a SIGPIPE). A full ring drops the line and counts it; once there is room, a "bridge: N lines dropped" warning goes first. The Console's own scrollback keeps everything regardless. A newline inside a sent line becomes a space, so one line stays one protocol line; the Console's mirror already splits on newlines. Lines over 1 KB are cut.

**Memory.** `sizeof(Bridge)` is 16,968 bytes (`static_assert` at 17 KB): the 16 KB ring, 256 bytes of command, 160 of hello, 64 of secret, and bookkeeping. Inside the template's `Platform`, so no allocation; nothing at run time allocates (the test sends until 100 lines drop, counting allocations: none). The mirror adds 16 bytes to `Console` (now 71,352). On the client side, `sqconsole` is a tool, so `std::string` and `std::vector` are used freely (priority 1 does not apply).

**The template.** Opens it in the `Platform` constructor of a CONSOLE build, sets the Console's mirror to it, logs `bridge: listening on 127.0.0.1:<port>`; on connect replays the newest 200 lines and logs `bridge: Termux connected`; closes it first in the destructor. `make console-only` passes the extra `squared.console_only=true`; the constructor reads it (`getIntent().getBooleanExtra`, JNI) and sets `app_stopped`, so `attach` creates nothing until `app start`.

**Patterns.** The mirror is an Observer with one slot, a function pointer and a context (no `std::function`, no list). The `watch` handler is the Strategy the event loop supplies, again a function pointer. Both called per printed line, not per frame.

**Tested** (`test/bridge_test.cpp`, real sockets, `poll` standing in for the looper): secrets of the wrong form refused; the announcement arrives; a wrong secret and a non-hello closed with no answer and nothing run; "ok", the replay flag, a command and its `.`, the three line types, a newline flattened; a 300-byte command cut then the next whole; a quiet client with hello and command in one write taking over, the first told and closed; a non-reading client: lines dropped and counted with no allocation, then the drop notice; the client leaving; `close` stopping every watch. With a real Console: the replay, a command's echo, its error and its end. Then `sqconsole` itself, as a child process: started before the app, it waits, connects on the announcement, shows the replay, runs a typed command; the app's bridge closes and reopens, and it notices both and reconnects; `--run` takes over, runs two commands, prints only their lines (no replay), exits 0, and the interactive one is told; a wrong secret exits 1; Ctrl-D ends it. Planted bugs (any secret accepted, no announcement, `quiet` ignored) each fail it.

## Log file

`console/log_file.hpp`, `src/log_file.cpp`. `save_log` writes the newest N lines with `write(2)` straight from the scrollback to `path.tmp`, then `rename`s it over `path`: a reader (the next run) never sees half a file, and a crash mid-write leaves the last good one. No allocation (tested). The template saves 200 lines on `APP_CMD_STOP` (the app leaves the screen: before a swipe away or a kill for memory) and in the `Platform` destructor; at start, `keep_previous_log` renames `log.txt` to `log-previous.txt` before anything saves over it, and `log previous` shows it. Plain text: the levels are not kept (an error line is shown as info), which keeps the file readable when pulled off the device.

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

- **Ownership.** `Console` and `Renderer` are members of the entry's `Platform`, which is allocated once: about 88 KB inside that one allocation, nothing more; the `Bridge` adds 17 KB in a CONSOLE build (about 105 KB in all). They come after `graphics` in it, so the renderer's GL objects go before the context.
- **The log.** `SQ_LOGI` and `SQ_LOGW` format a line once into the stack. With `DEV_LOG`, it goes to the dev log with its level, and the dev log's listener, set to the Console while the platform exists, prints it: so every dev log line of the main thread reaches the Console, the application's own (`sq::dev::log_line`) and the allocation reports included. Without `DEV_LOG`, the template prints its lines into the Console directly; the application's lines then have nowhere to go but logcat. Warnings and errors keep their colour. Lines logged before the platform exists (the first, "starting") are not shown, nor are lines from other threads.
- **Commands.** The platform's constructor registers the built-ins (above) and shows the Console.
- **When.** The renderer is made as soon as the surface is ready, before the application's `create()`, because the Console is for when that goes wrong; after a resume that lost resources it is invalidated and made again. It draws after `App::render`, inside the frame-allocation count, so a console frame that allocated would be reported.
- **Where.** The upper half of the screen inside the safe area. The cell is 8 pixels for every 160 dpi of the screen's density, rounded (`AConfiguration_getDensity`): 24 on a 420 dpi phone, 45 columns across 1080 pixels.
- **Input.** In the template's `Platform` (`console_motion`, `console_key`), before the application sees anything:
  - **Three-finger tap** (decided 2026-10-17; edge swipes and corners collide with Android's own gestures): from the third finger down until every finger is up, the gesture is the Console's. The fingers already down are sent to the application as `PointerUp` far off screen, which ends their presses without a click (the GUI clicks only on an up inside the pressed widget); a fourth finger, a move past 2 cells, or more than 500 ms from the first finger down makes it not a tap.
  - **A gesture that starts on the Console** is all the Console's, down to the up, so nothing leaks to the application (4.2's `[x]` sent the application an unpaired up). Under the slop it is a tap: `[x]` closes, a hold of 500 ms on the input row pastes, anything else focuses (shows the keyboard). Past it, a drag: one row per cell of vertical movement, clamped with `scroll_limit`. Other fingers in the same event still go to the application.
  - **Keys:** backtick (no Ctrl, Alt or Meta) toggles it from anywhere. While it has focus every key is its own except volume and power: Back closes; Ctrl+V pastes; `Console::key` takes editing keys; anything else is turned into the character it types (`KeyCharacterMap`, through JNI) and inserted.
  - **Keyboard:** opening it, or tapping it, shows the soft keyboard; closing hides it unless the application's text field still wants it.
  - **The application's lifecycle** (asked for 2026-10-19). `app stop` calls `surface_destroyed()` and `dispose()`, then destroys the `App` and constructs a fresh one in place (`std::destroy_at`, `std::construct_at`), so a later start is a first start; `app start` runs the launch path (`create()`, the text-input service, `surface_created()`, the size). A `created` flag gates every call into the `App`, so a stopped one is never called; the frame clears the screen instead of rendering it. A failed `create()` in a Console build opens the Console rather than leaving a black window. `quit` sets a flag the main loop reads beside `quit_requested()`. All in-process: a crash still ends everything. The monitor (4.7) gives the same verbs across processes.
  - **The keyboard after Enter:** some keyboards take Enter as "done" and hide, since there is no text view to stay for; the Console asks for it again six frames (about 100 ms) after an Enter, once the keyboard's own hide has gone through. Asking when it is still up does nothing.
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
