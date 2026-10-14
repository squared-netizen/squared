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
  Makefile
```

## How it works

One UDP socket, non-blocking, `SOCK_CLOEXEC`, with its destination fixed to
`127.0.0.1:<port>`. `log_line` is one `sendto` and nothing else.

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
they are exported from the application's `.so`. On Android the dynamic linker
looks a symbol up in the executable, then in the loaded library's own group in
load order: the application library before `libc++_shared.so`. So libc++'s
own calls to `operator new` (a `std::string` growing out of line) reach the
replacements too. The host test checks the same thing against libstdc++.

**Reporting** is done by `frame_end()` on the frame thread, after disarming,
into a 160-byte stack buffer with `std::to_chars`, then `log_line()`. The
first allocating frame is reported at once; after that, one summary per 120
frames, only when one of them allocated.

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
| counter state (flags, tallies, report window) | about 100 |
| report line | on the frame thread's stack, 160 |

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

`make -C dev test` runs two programs.

- `dev_log_test` sends a line, checks truncation and the closed state, then
  forks a child that dereferences null. It checks the report arrives in order,
  ends with `END`, and that the child still dies of SIGSEGV.
- `frame_allocations_test` counts a frame that allocates nothing (0), a
  `make_unique` (1 allocation, 8 bytes), a string grown inside the shared C++
  library (counted), an over-aligned `new` (counted, aligned), and a vector
  built on another thread during the frame (not counted). Then it opens a sink
  and checks the summary line for a window with three allocating frames, and
  that a clean window sends nothing.
