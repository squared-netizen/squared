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
void log_line(std::string_view line) noexcept;
```

Sends one line. Does nothing if no sink is open. Lines over 1000 bytes are
cut. It never blocks; if nothing is listening the line is dropped.

```cpp
sq::dev::log_line("loaded skin");
```

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
alloc: site 2 (frame 7): libc++_shared.so 0x8f1c4 < libdemo.so 0x52018 < libdemo.so 0x3e110 < libdemo.so 0x3d9a8
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
`std::vector` and `std::string` growth usually is. A frame in
`libc++_shared.so` is the C++ library's own code, and the APK's copy of it is
stripped. Look one frame further out, in your library, to see who called it.

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
