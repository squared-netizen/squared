# namespace time

## Records

* [DeadlineQueue](DeadlineQueue.md)
* [Timepiece](Timepiece.md)


## Functions

### now

*TimePoint now()*



**brief** The monotonic clock, now: nanoseconds from an arbitrary start, never going back, and not counting the time the device spent in deep sleep. One call into the vDSO, with no system call. Read it once a frame or a tick and pass the value in; tests pass their own times instead.

### since_boot

*TimePoint since_boot()*



**brief** Nanoseconds since the device started, deep sleep included: how long an app was away, for `foreground(elapsed)`.

