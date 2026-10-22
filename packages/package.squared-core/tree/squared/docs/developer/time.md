# time &mdash; internals

Rewritten in Sprint 6.6b (2026-10-21) to the app model's "time as a value"
(spec §3): a 24-byte `Timepiece`, two clock functions, and
`DeadlineQueue<Value, N>` holding its own array. Nothing allocates.

Programmer counterpart: [../programmer/time.md](../programmer/time.md)

## Layout

```
time/
  duration.hpp time_point.hpp now.hpp timepiece.hpp deadline_queue.hpp time.hpp
  src/   now.cpp, timepiece.cpp
  test/  time_test.cpp
  Makefile
```

## What was removed, and why

Decided 2026-10-21 ("yes to all recommendations"), from the spec's table of
2026-10-15:

- **`Clock`** (a base class with a virtual `now()`) and **`ManualTimepiece`**:
  a vtable and a virtual call per reading, for test substitution only, which
  passing the time in gives for nothing.
- **Time scale and pause**: a `long double` multiply and carried fraction per
  advance (the old `Timepiece` was 48 bytes with a vptr); slow motion is one
  multiply in the model that wants it.
- Nothing outside the module used any of it: the old dispatcher, its one
  user, went in 6.2.

## Timepiece

Three members, 24 bytes (`static_assert`ed): the last time given, the step
the last tick reported, the tick count, all integer nanoseconds (so no drift
and no rounding until `seconds()` divides). Not started is the last time at
`TimePoint::min()`, so no flag is needed. `tick(now)` is a subtraction, a
comparison with `longest_step` (250 ms, a constant: a per-object limit would
cost 8 bytes each, priority 1; a build knob waits for a use, priority 4), and
a division to seconds. A `now` before the last time (a caller's mistake, or
two clocks mixed) reports a 0 step and moves on from `now`.

The first tick without `start` reports 0 (decided: as libGDX's first delta).
The model's runner calls `start` when `tick_every` is first set, so a
model's first tick measures a real interval.

**Its users:** the model's runner (`messaging::detail::TickClock` keeps the
next due time and the interval beside a `Timepiece`, 40 bytes), and the UI's
frame loop (Sprint 7). One type for both: the clamp and the measuring are
written once.

## The clocks

`now()` is `clock_gettime(CLOCK_MONOTONIC)` and `since_boot()` is
`clock_gettime(CLOCK_BOOTTIME)`, each in one translation unit so the header
pulls in no `<time.h>`. On Linux and Android both are vDSO calls (tens of
nanoseconds). `CLOCK_MONOTONIC` is what `std::chrono::steady_clock` reads on
both C++ libraries, so a mix of the two agrees. Both clocks are Linux's; a
new platform supplies these two functions.

## DeadlineQueue

A binary min-heap over its own `std::array<Entry, N>` (`std::push_heap` and
`std::pop_heap`, ordered by due time then a 64-bit sequence, so equal times
keep their order). Entry: due 8, sequence 8, the value (24 bytes for an
`int`); the queue is the array plus 16 (the size, the next sequence): 784
bytes for 32 `int`s.

| Operation | Time |
|---|---|
| `schedule_at`, `schedule_after` | O(log n) |
| `poll_due`, per value | O(log n) |
| `next_due` | O(1) |
| `cancel` | O(n): a scan for the ticket, then `make_heap` |
| `visit_ordered` | O(n²): a selection walk, allocating nothing (it was a `std::vector` of pointers per call); for the few timers a Console command shows |

- **The capacity a template parameter, the array its own** (decided
  2026-10-21, your call after 6.6b shipped storage given by the owner:
  "simplicity and ergonomics leads to less errors and we can also change it
  if it proves too slow"): `DeadlineQueue<Restock, 16> timers;` is the whole
  setup. It was a `std::vector` reserved in the constructor, then (6.6b) a
  span over storage the owner gave, which took two lines (the array, then
  the queue). The cost: one instance of the template per capacity and value
  type, and a capacity fixed at compile time. Same bytes as before
  (priority 1 untouched); priority 2 decided it.
- **Slots are cleared** when a value leaves (polled or cancelled): assigned
  `Value{}`, so a value whose move leaves its source intact (any trivially
  copyable type, or a handle) is not kept alive by the array.
- **`Value`** must be default-constructible (the array is made empty) and
  move without throwing (the `DeadlineValue` concept and a `static_assert`),
  so every operation is `noexcept` or throws only what the visitor throws.
- **Results are snake_case** (`scheduled`, `full`, `invalid_time`,
  `tickets_exhausted`), as messaging's are: the backlog's enum-spelling item,
  settled for this type. Scheduling returns `Scheduled { result, ticket }`
  instead of filling an out-pointer.
- **`schedule_after`** refuses a sum past the largest time before adding (the
  addition would be signed overflow, undefined; UBSan catches it if the check
  is removed).
- **Standard library:** `<algorithm>`'s heap functions, `std::array`,
  `std::optional`. Passed over: `std::priority_queue` (it owns a container,
  and offers neither cancel nor an ordered walk), `std::map`/`std::multimap`
  (a node per entry, §2.1).
- **Pattern:** none beyond the heap; values are not callbacks, so policy stays
  with the owner.

## Tested

`time/test/time_test.cpp`: the `Timepiece` with exact times (the first tick,
steps, the same time twice, the clamp, measuring on from a stall, a time that
goes back, `start`, the epoch); the clocks (monotonic over 1,000 reads, boot
time never behind); the queue's order with ties, `full`, the ordered walk, a
poll limit, cancels (gone, given back, the empty ticket), every
`invalid_time` case and exactly the largest time; owning values (a 100-byte
string cancelled into a visitor); a value whose move copies, counting its
live copies (none left in the slots once given back or cancelled); no allocation over 100 rounds of 64; 200,000
random schedules, cancels and polls against a `std::map`, with the ordered
walk compared every 1,000 steps. Clean under AddressSanitizer and
UndefinedBehaviorSanitizer; g++ and clang. Planted bugs caught: no clamp, the
first tick measured from the epoch, a negative step, `start` keeping the
count, ties reversed, a cancel that breaks the heap, a poll past its limit, a
slot left holding its value, the overflow check removed (by UBSan), an ordered
walk that repeats.

## Memory

| Type | Bytes |
|---|---|
| `Timepiece` | 24 |
| `DeadlineQueue<Value, N>` | 16 plus `N` entries |
| an entry | 16 plus the value, aligned: 24 for an `int` |

Nothing allocates; each is resident where its owner is (a model's timers in
the model, so in its `ModelRunner`).
