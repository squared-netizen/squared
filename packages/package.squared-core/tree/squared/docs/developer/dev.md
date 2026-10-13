# dev &mdash; internals

Loopback UDP log sink and fatal-signal reporter.

Programmer counterpart: [../programmer/dev.md](../programmer/dev.md)

- Public types: 2 (`LogSinkOptions`, `LogSinkStatus`) and three functions
- Translation units: 1
- Header: `dev_log.hpp` at the module root

## Layout

```
dev/
  dev_log.hpp
  src/dev_log.cpp
  test/dev_log_test.cpp
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

## Complexity

`log_line`: O(n) in the line length (one copy into the kernel). Handler:
O(frames), at most 24.

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

## Known limitations

- A crash before `open_log_sink` runs is not reported.
- Stripped libraries give offsets but no names; keep the unstripped `.so`
  from `build/`.
- UDP can drop packets under load. Loopback with a listener rarely does.
- The handler does not chain to a previous handler; it re-raises the default.

## Testing

`make -C dev test` sends a line, checks truncation and the closed state, then
forks a child that dereferences null. It checks the report arrives in order,
ends with `END`, and that the child still dies of SIGSEGV.
