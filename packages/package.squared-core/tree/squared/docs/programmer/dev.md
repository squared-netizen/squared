# dev &mdash; development aids

Not part of a release build. Linked only when a project asks for it.

Developer counterpart: [../developer/dev.md](../developer/dev.md)

Headers: `squared/dev/dev_log.hpp` (the log sink),
`squared/dev/frame_allocations.hpp` (the frame-allocation counter)

## Why it exists

On a phone with no computer attached there is no `adb logcat`, and Android
shows a crashed app as "keeps stopping" with no reason. The log sink sends each
log line, and a backtrace if the app dies, to a UDP port on the same device.
You read it in a terminal on that device. It uses the loopback address only: no
network, no Wi-Fi, and nothing leaves the device.

## The shortest use

In the Android template, one flag does everything:

```sh
make DEV_LOG=1 apk && make install      # build with the sink
nc -klu 5555                            # in Termux: print what arrives
```

Open the app in another window. Log lines appear as they are written. If the
app crashes, the last lines are:

```
CRASH SIGSEGV (11) code 1 addr 0x0
  #0 pc 0x1ba7 libdemo.so
  #1 pc 0x2c25 libdemo.so
  ...
END
```

Each `pc` is an offset into the named library. Turn it into a function and
line with the unstripped library from your build:

```sh
addr2line -f -C -e build/libdemo.so 0x1ba7
```

A normal build (`make apk`) contains none of this and does not request the
`INTERNET` permission. The template adds that permission to a copy of the
manifest only for `DEV_LOG` builds.

## LogSinkOptions

```cpp
struct LogSinkOptions {
    std::uint16_t port = 5555;  // UDP port on 127.0.0.1
    bool catch_crashes = true;  // report fatal signals
};
```

## open_log_sink

```cpp
[[nodiscard]] LogSinkStatus open_log_sink(const LogSinkOptions& options = {}) noexcept;
```

Opens the socket and, unless `catch_crashes` is false, installs handlers for
SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL and SIGTRAP.

| Returns | Meaning |
|---|---|
| `Opened` | Ready. |
| `AlreadyOpen` | A sink is already open; nothing changed. |
| `SocketFailed` | The OS refused the socket. On Android this is almost always the missing `INTERNET` permission. |

Failure is a returned value; nothing is thrown.

## log_line

```cpp
void log_line(std::string_view line, LogLevel level = LogLevel::Info) noexcept;
```

Sends one line to the sink, if one is open; lines over 1000 bytes are cut.
Also hands it, whole and with its level, to the listener, if one is set and
this is the thread that set it. It never blocks; if nothing is listening the
line is dropped. The sink sends the text alone: `level` is for the listener
(the in-app Console colours warnings and errors).

```cpp
sq::dev::log_line("loaded skin");
sq::dev::log_line("skin: no font named 'title'; using the default", sq::dev::LogLevel::Warning);
```

`LogLevel` is `Info`, `Warning` or `Error`.

## set_log_listener

```cpp
using LogListener = void (*)(void* context, LogLevel level, std::string_view line) noexcept;
void set_log_listener(LogListener listener, void* context) noexcept;
```

Every later `log_line()` made on the calling thread is also handed to
`listener`, with `context`; `nullptr` stops it. One listener at a time.
Lines written on other threads go to the sink only, so the listener needs no
lock; the crash handler's lines never reach it. The listener must not call
`log_line()` itself.

The Android template sets one in `CONSOLE` builds with `DEV_LOG`, so the
dev log shows in the in-app Console ([console.md](console.md)).

## write_crash_trace

`#include <squared/dev/crash_trace.hpp>`, header-only: usable without linking this module.

```cpp
using CrashSink = void (*)(void* context, const char* text, std::size_t length) noexcept;
void write_crash_trace(int signal, const siginfo_t* info, void* ucontext,
                       CrashSink sink, void* context) noexcept;
```

From inside a fatal-signal handler: hands `sink` the report a line at a time (`CRASH <name> (<n>) code <c> addr <a>`, frame 0 from the saved registers, up to 23 unwound frames as `  #n pc 0xOFFSET library`, `END`). The sink must be async-signal-safe (`write`, `sendto`). The dev log's crash report and the Console's crash file both use it.

## close_log_sink

```cpp
void close_log_sink() noexcept;
```

Closes the socket and restores the signal handlers it replaced.

## Frame allocations

A frame should not touch the heap: squared allocates at load time and reuses
during frames. Static checks cannot tell a load-time `push_back` from a
frame-time one, so a development build counts them.

The Android template does it for you in `DEV_LOG` builds: every frame's
`update()` and `render()` are counted, and frames that allocate are reported
over the log sink.

```
alloc: site 1 (frame 3): libdemo.so 0x4a5f0 < libdemo.so 0x51c2c < libdemo.so 0x3e110 < libdemo.so 0x3d9a8
alloc: frame 3: 2 allocations, 96 bytes (the first frame that allocated)
alloc: site 2 (frame 7): libdemo.so 0x8f1c4 < libdemo.so 0x52018 < libdemo.so 0x3e110 < libdemo.so 0x3d9a8
alloc: 41 of the last 120 frames allocated: 82 allocations, 3936 bytes; most in one frame: 2
alloc: by site: 1:41 2:41
```

- **`frame` and summary lines.** The first arrives the first time any frame
  allocates. After that, at most one summary every 120 frames (two seconds at
  60 frames a second), and only when a frame in that window allocated. A
  program whose frames never allocate prints nothing.
- **`site` lines** name each place that allocates, once, the first time it
  does. A site is the four innermost frames above `operator new`, innermost
  first: the code that called `new` (often a container growing), then what
  called that. The number is the site's id for the rest of the run.
- **`by site`** follows each summary: the window's busiest sites as
  `id:allocations`, most first.

Turn a site into function names and source lines with the unstripped library
from your build, one offset or several at once:

```sh
addr2line -f -C -i -e build/libdemo.so 0x4a5f0 0x51c2c 0x3e110
```

`-i` matters. It shows the functions inlined at that address, which is where
`std::vector` and `std::string` growth usually is. The C++ library is linked
into your library, so its frames resolve the same way, from the same
unstripped `build/libdemo.so`; the APK's copy is stripped. A frame inside
libc++ is usually a container growing: look one frame further out to see who
asked it to.

To count something else, bracket it yourself:

```cpp
sq::dev::frame_begin();
level.update(delta);
const sq::dev::FrameAllocations used = sq::dev::frame_end();
// used.count, used.bytes
```

### FrameAllocations

```cpp
struct FrameAllocations {
    std::uint64_t count = 0;  // calls to operator new, every form
    std::uint64_t bytes = 0;  // bytes those calls asked for
};
```

### frame_begin

```cpp
void frame_begin() noexcept;
```

Starts counting allocations made by the calling thread. Calling it again before
`frame_end()` restarts the count.

### frame_end

```cpp
FrameAllocations frame_end() noexcept;
```

Stops counting and returns the count. Reports through the log sink as above
when one is open. Returns zeros when `frame_begin()` was not called on this
thread.

### report_allocations

```cpp
void report_allocations() noexcept;
```

Reports every frame counted so far through `log_line()`: one summary line;
what `operator new` holds now, on every thread (squared's and your own heap,
apart from the GPU driver's and the C library's direct `malloc`); then a
`site` line for every site found.

```text
alloc: 1840 frames counted, 3 allocated: 6 allocations, 288 bytes; most in one frame: 2
alloc: operator new holds 4120 KB in 1873 blocks now (peak 6212 KB), every thread
alloc: site 1 (frame 3): libdemo.so 0x4a5f0 < libdemo.so 0x51c2c < ...
```

The Console's `alloc` command runs it. Call it on the thread that counts
frames.

### What it counts

- Every `operator new` the counted thread makes, including the ones inside the
  C++ library (a `std::string` growing, a `std::vector` reallocating).
- Not other threads: a loader or audio thread allocating meanwhile is not a
  frame allocation.
- Not `malloc` called directly, and not the GPU driver.
- At most 64 distinct sites are named per run. Later ones are still counted
  and show as `unnamed` in the `by site` line.

Linking this part of the module replaces the global `operator new` and
`operator delete` for the whole process. That is why it exists only in a
development build.

## Lua

Not bound. These are native debugging aids.

## Notes

- After reporting, the crash handler restores the default action and re-raises
  the signal, so Android still produces its own report and the process still
  dies the way it would have.
- A crash that happens before `open_log_sink` runs is not reported. The
  Android template opens the sink when the library loads, to cover startup.
