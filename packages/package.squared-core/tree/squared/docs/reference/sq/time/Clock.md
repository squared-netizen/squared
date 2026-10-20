# class Clock

*Defined at [time/clock.hpp:15](../../../../time/clock.hpp#L15)*



**brief** Read-only view of one application time domain.

 Reading a clock never advances it. The domain owner advances its Timepiece once per frame or fixed simulation step. Borrowers must not retain the clock past the Timepiece's lifetime.



## Functions

### now

*public int now()*



**brief** Return the current time in this domain.

**return** Nanoseconds since the domain's epoch, monotonically non-decreasing while the domain is not reset.

### ~Clock

*public void ~Clock()*

*Defined at [time/clock.hpp:18](../../../../time/clock.hpp#L18)*



