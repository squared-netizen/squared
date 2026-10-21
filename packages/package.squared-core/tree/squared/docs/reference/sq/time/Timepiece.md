# class Timepiece

*Defined at [time/timepiece.hpp:21](../../../../time/timepiece.hpp#L21)*



**brief** The time between ticks, as a value: the last time it was given, the step since the one before (clamped), and how many ticks there have been. 24 bytes, no base class, nothing virtual, no clock of its own: the loop reads the clock once (`now()`) and passes the time in.

 One per loop: the UI's frames, the model's ticks. Not shared between threads.



## Members

public static duration longest_step



## Functions

### tick

*public double tick(TimePoint now)*



**brief** A tick at `now`: the seconds since the last tick (or since `start`), at most a quarter second; 0 for the first tick when there was no `start`, and 0 if `now` is earlier than the last time.

### start

*public void start(TimePoint now)*



**brief** Count from `now`: the next tick measures from here. Clears the step and the count. 

### seconds

*public double seconds()*



**brief** The step the last tick reported, in seconds. 

### step

*public Duration step()*

*Defined at [time/timepiece.hpp:38](../../../../time/timepiece.hpp#L38)*



**brief** The step the last tick reported, in nanoseconds. 

### last

*public TimePoint last()*

*Defined at [time/timepiece.hpp:44](../../../../time/timepiece.hpp#L44)*



**brief** The time the last tick (or `start`) was given. 

### ticks

*public std::uint64_t ticks()*

*Defined at [time/timepiece.hpp:47](../../../../time/timepiece.hpp#L47)*



**brief** Ticks since `start`, or since it was made. 

### started

*public bool started()*

*Defined at [time/timepiece.hpp:50](../../../../time/timepiece.hpp#L50)*



**brief** Whether it has a time to measure from: a tick or `start` happened. 



