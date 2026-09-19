# time

Clock domains and a bounded deadline queue.

Every type below lives in its own header. Include exactly the ones a
translation unit names; `squared/time/time.hpp` pulls in all of them
and exists for convenience, not for use inside headers of your own.

Developer counterpart: [../developer/time.md](../developer/time.md)

| Type | Header | Purpose |
|---|---|---|
| `Clock` | `squared/time/clock.hpp` | Read-only view of one application time domain |
| `DeadlineQueue` | `squared/time/deadline_queue.hpp` | Bounded deterministic queue used by higher-level schedulers |
| `Duration` | `squared/time/duration.hpp` |  |
| `ManualTimepiece` | `squared/time/manual_timepiece.hpp` | Explicitly controlled Timepiece for simulations, editors, and tests |
| `TimePoint` | `squared/time/time_point.hpp` |  |
| `Timepiece` | `squared/time/timepiece.hpp` | Mutable, pausable, scaled application time domain |
