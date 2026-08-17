# Squared Time — Developer Guide

Squared Time provides portable, independent time-domain primitives: a
pausable and scaled `Timepiece`, a read-only `Clock` view for borrowers, and
a bounded deterministic `DeadlineQueue<T>` that higher-level schedulers
(Squared Messaging) build on. Time is the framework's single source of
deterministic scheduling; it depends on no other Squared module and no
third-party library.

- Programmer counterpart: [Squared Time — Programmer Guide](../programmer/squared-time/README.md)
- Package payload: [Time.md](../../packages/squared-time/content/docs/Time.md)
- Documentation index: [Developer documentation](../README.md)
- Diagrams:
  - [Timepiece pause/scale/update states](timepiece-states.dot)
  - [Deadline schedule/cancel/fire flow](deadline-flow.dot)
  - [Time-to-Messaging dependency](time-messaging-dependency.dot)

## Dependency boundary

The manifest declares no `module.requires` entry: the package requires no
other Squared module and no third-party library. The CMake target
`squared_time` is a `STATIC` library compiling `src/timepiece.cpp` with only
`cxx_std_20` and the `include/` directory. `timepiece.hpp` includes only
`<chrono>` and `<cstdint>`; `deadline_queue.hpp` adds `<algorithm>`,
`<cstddef>`, `<cstdint>`, `<limits>`, `<utility>`, and `<vector>`.

The dependency edge runs in one direction only: Squared Messaging requires
`dev.squarednetizen.squared.time` and `message_dispatcher.hpp` includes
`deadline_queue.hpp` and `timepiece.hpp`. Time never includes or references
Messaging. No other package currently consumes Time.

## Architecture

| Component | Source | Responsibility |
| --- | --- | --- |
| `squared::time::Clock` | `include/squared/time/timepiece.hpp` | Read-only `now()` interface for borrowers. |
| `squared::time::Timepiece` | `include/squared/time/timepiece.hpp` | Paused/scaled domain state and `advance`. |
| `squared::time::ManualTimepiece` | `include/squared/time/timepiece.hpp` | `reset()` for deterministic control. |
| `squared::time::DeadlineQueue<T>` | `include/squared/time/deadline_queue.hpp` | Bounded min-heap of `(due, sequence, value)` entries. |
| `Timepiece` methods | `src/timepiece.cpp` | Implementation of advance/scale/pause arithmetic. |

## Ownership and threading

- `Timepiece` owns `now_` (a `TimePoint`), a `long double`
  `fractional_nanoseconds_` carry, the `double` `time_scale_`, and a `bool
  paused_`. `ManualTimepiece` exposes `reset()`, which clamps negative input
  to the epoch.
- `DeadlineQueue<T>` owns its entries in a `std::vector<Entry>`, the fixed
  `capacity_`, and the monotonic `next_sequence_`. Values are moved through
  the queue; delivery and cancellation hand the stored value to a visitor or
  destroy it.
- Borrowers hold a non-owning `Clock&`. The domain owner must keep the
  `Timepiece` alive for the duration of every borrow; there is no shared
  ownership.
- All types are single-threaded: one thread owns and advances the domain,
  and a queue must be confined to one thread or externally locked. No type
  starts a thread or owns background work.

## Invariants and failure behavior

- `time_scale_` is always in the closed range `[0, 1024]` and finite;
  `set_time_scale` rejects everything else with `false` without mutating the
  stored value.
- `advance` is a no-op when paused, when `delta.count() <= 0`, or when the
  scale is zero. Positive scaled deltas are computed in `long double`
  precision, the whole part is added to `now_`, and the fractional remainder
  is carried into the next advance. Overflow saturates `now_` at the largest
  representable `Duration::rep` and clears the carry.
- `now()` is monotonically non-decreasing between resets because negative
  deltas are ignored and `reset` clamps to the epoch.
- `DeadlineQueue` invariants: `entries_.size() <= capacity_`; every ticket
  is a distinct positive `next_sequence_` value; a zero `Ticket` never
  matches; equal due times are ordered by ascending sequence (insertion
  order).
- Scheduling failures are reported, never swallowed: `QueueFull` at capacity,
  `InvalidTime` for negative due/delay or deadline overflow beyond the
  `TimePoint` range, `TicketExhausted` when `next_sequence_` reaches
  `uint64_t::max()`.
- `poll_due` delivers at most `limit` entries, never delivers an entry due
  after `captured_now`, and returns the delivered count; a partially drained
  call leaves the remaining entries in the queue.

## Data structures and complexity

- `DeadlineQueue` keeps its `std::vector<Entry>` in heap order using
  `std::push_heap`/`std::pop_heap` with a `Later` comparator that orders by
  `due` ascending and then `sequence` ascending. Push and pop are O(log n);
  the queue never reallocates beyond the `reserve(capacity)` done at
  construction.
- `cancel` is a linear scan for the ticket's sequence followed by `erase` and
  `std::make_heap`, O(n). This is the accepted cost of ticket-based removal
  from a heap; cancellation is not the hot path.
- `visit_ordered` sorts an array of entry pointers by `(due, sequence)` —
  O(n log n) time and O(n) temporary pointer space — to provide deterministic
  delivery-order inspection without mutating the heap.
- `Timepiece` is O(1): a few scalar members and constant-time arithmetic.

## Algorithms and execution order

`Timepiece::advance(delta)`:

1. Return early when paused, `delta.count() <= 0`, or `time_scale_ == 0.0`.
2. Compute `scaled = delta * scale + fractional_nanoseconds_` in
   `long double`.
3. Take `whole = floor(scaled)`; if `whole >= max_rep - now_.count()`,
   saturate `now_` to the maximum and clear the carry.
4. Otherwise add `whole` nanoseconds to `now_` and keep
   `scaled - whole` as the carry.

`DeadlineQueue` scheduling and firing:

1. `schedule_at`: reject negative `due`; reject at capacity; reject when the
   ticket counter is exhausted; assign `++next_sequence_`, push the entry, and
   reheapify with `push_heap` (O(log n)).
2. `schedule_after`: read `clock.now()` exactly once, validate that
   `current >= 0` and `delay <= max - current`, then delegate to
   `schedule_at(current + delay, ...)`.
3. `poll_due`: while the heap front is due (or earlier than `captured_now`),
   `pop_heap`, move the back entry out, and either visit it (if under
   `limit`) or push it back and stop; return the delivered count. This is the
   deterministic per-update drain that Messaging caps.
4. `cancel`: linear scan for the sequence, `erase`, reheapify with
   `make_heap`, and (with the visitor overload) observe the removed value
   before it is destroyed.

## Design patterns

- **State (simplified)** — `Timepiece` models running/paused as a boolean
  state guarded at the top of `advance`. A full State pattern with transition
  objects was rejected as overkill for two states; the boolean gate is the
  actual implementation.
- **Borrowed read-only interface (Clock)** — consumers depend on the
  `Clock` abstraction, not on the concrete `Timepiece`, so a domain can be
  replaced without touching borrowers; the domain owner alone advances time.
- **Priority queue** — `DeadlineQueue` is a textbook binary heap over due
  time, extended with a monotonic sequence to make equal deadlines
  deterministic (stable FIFO). This is the guarantee Messaging's delivery
  order relies on.
- **Policy-free value storage** — the queue stores values, not callbacks, so
  the timing policy stays in the higher-level subsystem (Messaging decides
  what due values mean); the queue never invokes application code on its own.

Alternatives rejected: a wall-clock/`steady_clock`-backed timer (rejected —
nondeterministic and untestable); a callback-registry queue (rejected — the
queue would own policy and complicate cancellation); an unbounded queue
(rejected — `TODO.md` and the tests require explicit capacity); a sorted
`std::set` keyed by time (rejected — duplicate deadlines require insertion
order, which a multiset does not give cheaply).

## Limitations and technical debt

- Cancellation is O(n) and each cancelled entry reheapifies the whole heap;
  acceptable now, but a hash index on tickets would make it O(log n) if
  cancel-heavy workloads appear.
- `TODO.md` still lists cancellation handles and pause/scale tests as "Next",
  but the working tree already implements tickets, `cancel`, pause, scaling,
  and deadline-overflow tests; the TODO list is stale relative to the code
  and should be reconciled before the next release.
- `visit_ordered` allocates a pointer array per call; large pending sets pay
  temporary O(n) space during inspection.
- Large-delta and saturation tests for `advance` are not yet comprehensive;
  the overflow behavior is implemented but only indirectly exercised.
- Repeating schedules are explicitly out of scope (`TODO.md` "Later"); any
  future repetition must not weaken deterministic ordering.
- Nanosecond storage plus `long double` carry is deterministic on a given
  platform but should be treated as implementation-defined across hardware
  if cross-platform bit-exactness is ever required.
