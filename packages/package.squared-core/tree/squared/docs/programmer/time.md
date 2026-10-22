# time

The time between ticks (`Timepiece`), the two clocks it is read from, and a
model's timers (`DeadlineQueue`). Time is a value: a loop reads the clock
once and passes the time in, so a test passes its own times and needs no
clock at all.

Every type below lives in its own header. Include exactly the ones a
translation unit names; `squared/time/time.hpp` pulls in all of them
and exists for convenience, not for use inside headers of your own.

Developer counterpart: [../developer/time.md](../developer/time.md)

| Type or function | Header | Purpose |
|---|---|---|
| `Duration`, `TimePoint` | `squared/time/duration.hpp`, `squared/time/time_point.hpp` | `std::chrono::nanoseconds`: a span, and a time since a clock's start |
| `now()`, `since_boot()` | `squared/time/now.hpp` | The monotonic clock; the time since boot, deep sleep included |
| `Timepiece` | `squared/time/timepiece.hpp` | The step between ticks, clamped, and the tick count: 24 bytes |
| `DeadlineQueue<Value, N>` | `squared/time/deadline_queue.hpp` | A model's timers, at most `N` waiting at once |

## Timepiece

```cpp
sq::time::Timepiece time;                      // 24 bytes, one per loop
for( ;; ) {
    const double dt = time.tick( sq::time::now() );   // seconds since the last tick
    world.step( dt );
}
```

| Call | Does |
|---|---|
| `tick(now)` | a tick at `now`; returns the seconds since the last tick (or since `start`), at most `longest_step` (250 ms). The first tick with no `start` returns 0: there is nothing to measure from. A `now` earlier than the last time returns 0 |
| `start(now)` | measure the next tick from `now`; clears the step and the count |
| `step()`, `seconds()` | the last step, as a `Duration` or in seconds |
| `last()`, `ticks()`, `started()` | the last time given, the ticks since `start`, whether there is a time to measure from |

Preconditions: one thread uses a `Timepiece`. Failure modes: none; a step
longer than a quarter second is reported as a quarter second (a debugger, a
suspended app), so a model never takes one huge step.

Slow motion and pause are not part of it: multiply `dt`, or skip the step,
in the model that wants them.

## The clocks

| Call | Returns |
|---|---|
| `sq::time::now()` | nanoseconds on the monotonic clock: never goes back, does not count deep sleep. One call into the vDSO, no system call |
| `sq::time::since_boot()` | nanoseconds since the device started, deep sleep included: how long an app was away, for `foreground(elapsed)` |

Read the clock once a frame or a tick and pass the value on; everything else
takes the time as an argument.

## DeadlineQueue

A model's timers: values due at times, handed back in order once their time
has come. It allocates nothing: it holds an array of `N` entries, as many as
may be waiting at once.

```cpp
struct Shop {
    sq::time::DeadlineQueue<Restock, 16> timers;

    void receive( Telegraph &t, const Received &m ) {
        timers.schedule_after( sq::time::now(), std::chrono::seconds( 30 ), Restock{ 17 } );
    }
    std::optional<sq::time::TimePoint> wake_at() const { return timers.next_due(); }
    void due( Telegraph &t, sq::time::TimePoint now ) {
        timers.poll_due( now, 16, [&]( Restock r ) { restock( t, r ); } );
    }
};
```

The model's thread sleeps until `wake_at()`, then calls `due` (see
[messaging.md](messaging.md#the-models-thread)): no tick, no polling.

| Call | Does |
|---|---|
| `DeadlineQueue<Value, N>` | empty; holds at most `N` at once (`N` at least 1) |
| `schedule_at(due, value)` | due at `due`; returns a `Scheduled`: `result`, and the `ticket` when scheduled (true when it was) |
| `schedule_after(now, delay, value)` | due `delay` after `now` |
| `cancel(ticket)`, `cancel(ticket, visitor)` | remove it (the visitor gets the value first); false if it is not there |
| `poll_due(now, limit, visitor)` | hand each value due at `now` or before to `visitor`, in order, at most `limit`; returns how many |
| `next_due()` | when the first is due; nothing when empty |
| `visit_ordered(visitor)` | every entry in the order they come due: `visitor(due, ticket, value)` |
| `size()`, `capacity()` (`N`), `empty()` | |

Values due at the same time come back in the order they were scheduled.
`Value` must be default-constructible and move without throwing.

**`ScheduleResult`:** `scheduled`; `full` (every entry in use); `invalid_time`
(before the epoch, a negative delay, or past the largest time);
`tickets_exhausted` (2^64 - 1 entries over the queue's life).

Not bound to Lua.
