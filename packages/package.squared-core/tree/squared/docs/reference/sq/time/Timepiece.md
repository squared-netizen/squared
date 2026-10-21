# class Timepiece

*Defined at [time/timepiece.hpp:17](../../../../time/timepiece.hpp#L17)*



**brief** Mutable, pausable, scaled application time domain.

 Time is stored as signed 64-bit nanoseconds. Negative deltas are ignored and overflow saturates at the largest representable time point. This class is not thread-safe; one thread must own the domain.



Inherits from Clock



## Functions

### Timepiece

*public void Timepiece()*

*Defined at [time/timepiece.hpp:20](../../../../time/timepiece.hpp#L20)*

### now

*public TimePoint now()*



**brief** Read the current domain time.

**return** Nanoseconds since the domain's epoch.

### advance

*public void advance(Duration delta)*



**brief** Advance the domain by one frame or fixed step.

**delta** Desired elapsed time in nanoseconds. Negative values are ignored. Scaled by the current time-scale value.

### pause

*public void pause()*



**brief** Freeze the domain so time no longer advances. 

### resume

*public void resume()*



**brief** Resume a paused domain. 

### paused

*public bool paused()*



**brief** Report the paused state.

**return** true while the domain is frozen.

### set_time_scale

*public bool set_time_scale(double scale)*



**brief** Set a finite time scale in the inclusive range [0, 1024].

**scale** Multiplier applied to every advance delta.

**return** false when the requested scale is invalid; true otherwise.

### time_scale

*public double time_scale()*



**brief** Read the current time-scale multiplier.

**return** Active multiplier in the inclusive range [0, 1024].

### set_now

*protected void set_now(TimePoint value)*



**brief** Overwrite the current time for explicit controls and tests.

**value** Replacement time point in nanoseconds.



