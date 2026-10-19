# dev &mdash; internals

Loopback UDP log sink, fatal-signal reporter and frame-allocation counter.

Programmer counterpart: [../programmer/dev.md](../programmer/dev.md)

- Public types: 3 (`LogSinkOptions`, `LogSinkStatus`, `FrameAllocations`) and
  five functions
- Translation units: 2
- Headers: `dev_log.hpp` and `frame_allocations.hpp` at the module root

## Layout

```
dev/
  dev_log.hpp
  frame_allocations.hpp
  src/dev_log.cpp
  src/frame_allocations.cpp
  test/dev_log_test.cpp
  test/frame_allocations_test.cpp
  test/log_listener_test.cpp
  Makefile
```

## How it works

One UDP socket, non-blocking, `SOCK_CLOEXEC`, with its destination fixed to
`127.0.0.1:<port>`. `log_line` is one `sendto`, then the listener, if any.

**What operator new holds.** Every block the replacement `operator new` hands out is added, at the size the allocator reports for it (`malloc_usable_size`, so slack included), to a live byte count, a block count and a peak; `operator delete` subtracts. Relaxed atomics: the figures are read by `report_allocations()`, not synchronised on. It separates the heap squared and the application keep from the GPU driver's and the C library's own `malloc`, which the system's heap figures lump together. Cost: three atomic operations per allocation and free, in development builds only.

**The listener** is a function pointer, a context and the thread that set it
(`set_log_listener`). `log_line` calls it only on that thread: the flag and
the owner are atomics any thread may read, while the pointer and context are
read only by the owner, so no lock is needed and a line from a loader thread
cannot reach the Console, which one thread uses. It is what the in-app
Console reads the dev log through; a function pointer and `void*`, not a
`std::function` or an observer list (§2.1, §5): there is one reader. The crash
handler builds its packets with `send_line` and never calls it: the Console
is not async-signal-safe. `LogLevel` travels with the line to the listener;
the UDP packet stays the bare text, so `nc` shows what it always did.

The crash handler runs on an alternate signal stack (so a stack overflow can be
reported) and builds its packets with fixed-size character appends. It sends:

1. `CRASH <name> (<number>) code <si_code> addr <si_addr>`
2. frame 0: the faulting program counter, read from the saved registers
3. frames 1 to 23: `_Unwind_Backtrace` from inside the handler
4. `END`

Each frame is printed as an offset from its library's load address
(`dladdr`'s `dli_fbase`), which is what `addr2line -e lib.so` expects. Then it
sets the default action and re-raises the signal.

The faulting pc is read directly because unwinding out of a signal handler does
not always reach the faulting frame. Frame 1 is normally the handler itself.

## The frame-allocation counter

The third lint layer ([lint.md](lint.md)). `frame_allocations.cpp` defines all
twenty replaceable allocation functions ([new.delete]): eight forms of
`operator new` (plain, array, aligned, `nothrow`) and twelve of
`operator delete`. Each `new` counts, then allocates with `malloc`, or with
`posix_memalign` when over-aligned; each `delete` calls `free`. Failure follows
the standard: the new handler, then `std::bad_alloc` (an abort under
`-fno-exceptions`).

Counting is two relaxed atomic loads and, while armed, a `pthread_self()`
compare:

- `g_armed` and `g_thread` say whether counting is on and for which thread.
  `frame_begin()` stores the thread, then arms; `frame_end()` disarms.
- The tallies are plain integers: only the counted thread writes them, and
  only that thread reads them in `frame_end()`.

`pthread_self()` and not a `thread_local` flag: thread-local storage in a
library loaded with `dlopen` (which is how Android loads the application) is
set up lazily, and on older API levels through emulated TLS; either can
allocate on first use, which here would be inside `operator new`.

**Whole process.** The replacements are declared
`[[gnu::visibility( "default" )]]`, against the build's hidden default, so
they are exported from the application's `.so`. On Android the C++ runtime is
linked into that `.so` (since 3.3: `libc++_static.a`, see
[build-and-toolchain.md](build-and-toolchain.md#android-target-sysroot-and-runtime)),
and libc++ defines its own `operator new` weak, as the standard's replacement
rule requires. So the static link binds libc++'s own calls to `operator new`
(a `std::string` growing out of line) to these replacements, before anything
runs. (With the shared runtime this used to depend on the dynamic linker's
search order, the application library ahead of `libc++_shared.so`.) The host
test checks the same thing against libstdc++.

**Call sites.** Each `operator new` passes its own return address
(`__builtin_return_address( 0 )`, expanded in each definition through one
macro, because a helper would report its own frame). While counting, the
allocation walks the stack with `_Unwind_Backtrace`, skips frames until it
reaches that address (the counter's own frames, however many inlining left),
and keeps the four frames from there outwards. That is the site's key. Sites
live in a fixed table of 64, searched linearly; the 65th distinct site and
beyond are counted as `unnamed`. A guard flag stops the walk from recursing if
an unwinder ever allocated. LLVM's libunwind grows its frame cache with
`malloc`, which is not counted.

The walk costs a few microseconds per allocation, paid only on the frame
thread while armed, and only in a development build. A frame that allocates
ten times pays it ten times; that is the price of naming them.

**Reporting** is done by `frame_end()` on the frame thread, after disarming,
into a 256-byte stack buffer with `std::to_chars`, then `log_line()`:

1. one `site` line for each site first seen in this frame, each frame as
   `library 0xoffset` (from `dladdr`), the form `addr2line -e` takes;
2. the first allocating frame, once;
3. every 120 frames, when one of them allocated, the summary and then the
   window's sites by count (`id:count`, at most eight distinct counts, found by
   repeated passes over the table: no sort, no copy).

**Linking.** `entry.cpp` calls `frame_begin()`, which pulls
`frame_allocations.o` out of `libsquared_dev.a`, and the replacements come with
it. A project that links the archive but never calls the counter does not get
the replacements.

## Complexity

`log_line`: O(n) in the line length (one copy into the kernel). Handler:
O(frames), at most 24. Counter: O(1) per allocation and per frame.

## Memory profile

Static storage only, present only in a build that links this archive. Nothing
is allocated at run time, in `open_log_sink` or anywhere else.

| Item | Bytes |
|---|---:|
| alternate signal stack | 16384 |
| previous handlers (6 `sigaction`) | about 200 to 900, by libc |
| destination `sockaddr_in` | 16 |
| socket fd | 4 |
| per-crash line buffer | on the alternate stack, 1001 |
| counter state (flags, tallies, report window, totals since the start) | about 130 |
| listener (flag, owner thread, function, context) | 32 |
| what operator new holds: bytes, blocks, peak (atomics) | 24 |
| site table: 64 sites of 56 bytes (4 frames, 2 counts, 2 flags) | 3584 |
| report line | on the frame thread's stack, 256 |

Priority 1 governs: the module is opt-in and costs nothing in a release build,
where the archive is not linked.

## Decisions and deviations

- **POSIX only, one implementation.** Sockets, `sigaction`, `dladdr` and
  `_Unwind_Backtrace` exist on Linux and Android alike, so there is no
  platform branch. This is why it sits in the framework and not in the
  Android porting layer.
- **One architecture switch.** `fault_pc` reads the saved program counter, and
  that register has a different name per CPU (`aarch64`, `x86_64`, `arm`,
  `i386`). An unknown CPU reports 0 and the unwound frames still follow. This
  is the only `#if` in the module and it selects an architecture, not a
  platform.
- **Not async-signal-safe throughout.** `dladdr` and `_Unwind_Backtrace` are
  not on the async-signal-safe list. They are used here because the process is
  already dying and a diagnostic that might fail beats none. The handler
  itself uses no allocation, stdio or locks. Do not use this module in
  a release build.
- **No `std::function`, no `std::string`, no allocation.** Lines are built in
  fixed buffers.
- **Not an observer or a logger framework.** One free function. A logging
  layer, if one is ever wanted, would call it; this does not grow into one.
- **Replacing operator new, not hooking malloc.** A malloc hook would count
  the GPU driver and every system library. `operator new` is what squared and
  the application call, and what the rule "no allocation in the frame loop" is
  about.
- **The rules allow it here only.** The replacements are the 4 `raw-new`, 12
  `raw-delete` and 1 `exceptions` matches accepted in
  `tools/lint-baseline.txt`, each with its reason, and the `malloc`/`free`
  calls sit in one `NOLINTBEGIN` block.

## Known limitations

- A crash before `open_log_sink` runs is not reported.
- Stripped libraries give offsets but no names; keep the unstripped `.so`
  from `build/`.
- UDP can drop packets under load. Loopback with a listener rarely does.
- The handler does not chain to a previous handler; it re-raises the default.
- The counter reports the first allocating frame once. If no sink is open at
  that moment the line is lost; the template opens the sink when the library
  loads, before any frame.
- `malloc` called directly, and allocations by the GPU driver, are not
  counted.
- Input handling runs outside `frame()` in the template, so what it allocates
  is not counted.

## Testing

`make -C dev test` runs three programs.

- `log_listener_test`: lines reach the listener with their level and whole
  (3000 bytes, past the sink's cut), with no sink open; another thread's line
  does not; `report_allocations()` sends the summary (`2 frames counted, 1
  allocated: 1 allocations, 8 bytes; most in one frame: 1`) and the site line
  `frame_end()` sent; once cleared, nothing arrives.

- `dev_log_test` sends a line, checks truncation and the closed state, then
  forks a child that dereferences null. It checks the report arrives in order,
  ends with `END`, and that the child still dies of SIGSEGV.
- `frame_allocations_test` counts a frame that allocates nothing (0), a
  `make_unique` (1 allocation, 8 bytes), a string grown inside the shared C++
  library (counted), an over-aligned `new` (counted, aligned), and a vector
  built on another thread during the frame (not counted). Then it opens a sink
  and runs two windows: two new sites, each named once with this program's
  name and an offset; the summary; the by-site line with the right counts;
  then a window where a known site allocates again, which reports its id and
  names nothing new; and a clean window, which sends nothing.

The offsets in the `site` lines were checked with
`addr2line -f -C -i -e build/release/frame_allocations_test`: they resolve to
`std::make_unique`, inlined into the test's `allocate_here()`, and to `main`.
