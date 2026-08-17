# Squared Time — Programmer Guide

Squared Time models explicit application time rather than a global wall clock.
A `Timepiece` belongs to one domain (an application, simulation, UI, or
editor-preview domain), and objects and messages borrow its read-only `Clock`
interface. `DeadlineQueue<T>` is the bounded, deterministic scheduling
primitive that Squared Messaging builds its delayed delivery on. Time is
portable and independent: it depends on no other Squared module.

## Package availability

| Module | Version | Requires |
| --- | --- | --- |
| `dev.squarednetizen.squared.time` | `0.6.0-dev.2` | (none) |

The CMake target is `squared_time`. Squared Messaging requires this module.

## Public API overview

| Type | Header | Purpose |
| --- | --- | --- |
| `squared::time::Duration` | `squared/time/timepiece.hpp` | `std::chrono::nanoseconds` alias. |
| `squared::time::TimePoint` | `squared/time/timepiece.hpp` | Nanoseconds since a domain epoch. |
| `squared::time::Clock` | `squared/time/timepiece.hpp` | Read-only view of one time domain. |
| `squared::time::Timepiece` | `squared/time/timepiece.hpp` | Mutable, pausable, scaled time domain. |
| `squared::time::ManualTimepiece` | `squared/time/timepiece.hpp` | Explicitly controllable time domain. |
| `squared::time::DeadlineQueue<T>` | `squared/time/deadline_queue.hpp` | Bounded deterministic deadline queue. |

## Timepiece

A `Timepiece` begins at `0ns`. `advance(delta)` advances the domain once per
frame or fixed step; the domain owner calls it, never the borrowers. Time is
stored as signed 64-bit nanoseconds, scaled sub-nanosecond fractions are
retained, negative deltas are ignored, and overflow saturates at the largest
representable time point.

```cpp
#include <squared/time/timepiece.hpp>

#include <chrono>

using namespace std::chrono_literals;

int main()
{
    squared::time::Timepiece clock;
    clock.advance(1s);              // clock.now() == 1s
    clock.pause();
    clock.advance(4s);              // frozen: still 1s
    clock.resume();
    clock.set_time_scale(0.5);      // accepted; returns true
    clock.advance(2s);              // 0.5 * 2s added: now 2s
    return 0;
}
```

- `advance(Duration delta)` — scaled by the current time scale. Negative
  deltas and a zero scale are ignored, and `advance` does nothing while
  paused.
- `pause()` freezes the domain; `resume()` unfreezes it; `paused()` reports
  the state.
- `set_time_scale(double scale)` accepts only finite values in the inclusive
  range `[0, 1024]` and returns `false` for anything else (NaN, infinity,
  negatives, values above 1024). `time_scale()` returns the active multiplier.
- `now()` returns the current `TimePoint`; reading never advances the clock.

### ManualTimepiece

`ManualTimepiece` adds `reset()` for deterministic simulations, editors, and
tests. It is a `Timepiece` first, so all pause/scale/advance behavior applies.

```cpp
#include <squared/time/timepiece.hpp>

#include <chrono>

using namespace std::chrono_literals;

squared::time::ManualTimepiece sim;
sim.reset(10s);          // now() == 10s
sim.reset();             // back to the domain epoch (0s)
```

## DeadlineQueue

`DeadlineQueue<T>` is a bounded, deterministic queue ordered by due time and
then insertion sequence, so equal deadlines fire in FIFO order. It stores
values, not callbacks, and never reads a clock or runs a thread: the owner
captures `clock.now()` once per update and passes it to `poll_due`.

```cpp
#include <squared/time/deadline_queue.hpp>
#include <squared/time/timepiece.hpp>

#include <chrono>
#include <cstdint>
#include <vector>

using namespace std::chrono_literals;

int main()
{
    squared::time::ManualTimepiece clock;
    squared::time::DeadlineQueue<int> queue(4);

    queue.schedule_at(3s, 30);                      // Scheduled
    squared::time::DeadlineQueue<int>::Ticket cancelled;
    queue.schedule_at(1s, 10, &cancelled);          // Scheduled, ticket set
    queue.schedule_at(1s, 11);                      // Scheduled, same deadline

    queue.cancel(cancelled);                        // removes the 10 entry

    clock.advance(3s);                              // all entries are due now
    std::vector<int> delivered;
    queue.poll_due(clock.now(), 4, [&delivered](int value) {
        delivered.push_back(value);                 // {11, 30}
    });
    return 0;
}
```

- `schedule_at(TimePoint due, Value value, Ticket* ticket = nullptr)`
  schedules at an exact domain time; `schedule_after(const Clock&, Duration
  delay, Value, Ticket* = nullptr)` reads the clock exactly once and computes
  `now + delay`. Both move the value through the queue.
- Results: `ScheduleResult::Scheduled`, `QueueFull` (at capacity),
  `InvalidTime` (negative due time, negative delay, or deadline overflow), or
  `TicketExhausted` (the monotonic ticket counter is exhausted).
- `cancel(Ticket)` removes an entry and reclaims its capacity, returning
  `true` when found. The `cancel(Ticket, visitor)` overload first hands the
  cancelled value to a visitor. A ticket cancels once; a zero ticket is empty
  and never matches.
- `poll_due(captured_now, limit, visitor)` delivers at most `limit` due
  values using one captured timestamp and returns the number delivered. The
  limit keeps one busy frame from draining an unbounded backlog; entries due
  later than `captured_now` stay queued.
- `size()`, `capacity()`, and `empty()` inspect queue state. `capacity()` is
  fixed at construction.

`DeadlineQueue` is not thread-safe: confine it to one thread or guard all
calls with a single external lock.

## Errors and failure behavior

Scheduling and cancellation report outcomes explicitly through
`ScheduleResult` and `bool` returns — no exceptions. Capacity pressure is
reported as `QueueFull`, invalid input as `InvalidTime`, and a depleted
ticket space as `TicketExhausted`. `Timepiece::set_time_scale` returns
`false` for out-of-range input; `advance` silently ignores negative deltas and
paused or zero-scaled domains by contract.

## Ownership and lifetime

A `DeadlineQueue` owns the values it holds until they are delivered, cancelled
(and, with the visitor overload, observed), or the queue is destroyed. Tickets
identify entries by an opaque monotonic sequence and remain valid until the
entry fires or is cancelled; they are not reused. A `Clock` is borrowed and
must outlive every borrower — do not retain a `Clock` past its `Timepiece`.
The domain owner advances one `Timepiece`; borrowers only read `now()`.

## Threading

All types are single-threaded by contract. One thread owns and advances a
`Timepiece`; other code reads `now()`. A `DeadlineQueue` must be confined to
one thread or externally locked. Nothing here starts a thread, polls in the
background, or advances a clock, so nanosecond precision does not imply
nanosecond update frequency.

## Lua bindings

None of the types in this package have a Lua 5.4 binding.

## Related documentation

- Package payload: [Time.md](../../../packages/squared-time/content/docs/Time.md)
- Implementation details: [Squared Time — Developer Guide](../developer/squared-time/README.md)
- Consumer documentation: [Squared Messaging — Programmer Guide](../squared-messaging/README.md)
- Documentation index: [Programmer documentation](../README.md)
