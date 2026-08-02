---
title: Time Domains
tags:
  - application
  - scheduling
  - time
---

# Time Domains

`squared::time` models explicit application time rather than exposing a global
wall clock. One `Timepiece` belongs to an application, simulation, UI, or
editor-preview domain. Objects and messages borrow its read-only `Clock`
interface; they do not own or update individual clocks.

The domain owner calls `advance(delta)` once per frame or fixed step.
`pause()` freezes the domain, and `set_time_scale()` accepts finite values from
zero through 1024. Time is stored as signed 64-bit nanoseconds, scaled
sub-nanosecond fractions are retained, and overflow saturates.

`ManualTimepiece` adds `reset()` for deterministic simulations, editors, and
tests.

## Deadline queue

`DeadlineQueue<T>` is the bounded, deterministic primitive beneath the future
Telegram scheduler. It stores values rather than callbacks, has a fixed
logical capacity, and orders equal deadlines by insertion sequence.

The owner captures `clock.now()` once per update and passes that value to
`poll_due(captured_now, limit, visitor)`. The delivery limit prevents one busy
frame from draining an unbounded backlog. Scheduling reports queue-full,
invalid-time, and ticket-exhaustion errors explicitly; tickets support
cancellation.

The queue never starts a thread, polls in the background, or advances a clock.
Nanosecond precision therefore does not imply nanosecond update frequency.

## Generated diagnostic

The small square below the blue/green lifecycle sentinel alternates magenta
and cyan once per application-time second. It freezes while the SDL
application is backgrounded and resumes without catching up to elapsed wall
time.
