# class Timepiece

*Defined at [time/timepiece.cppm:19](../../../../time/timepiece.cppm#L19)*



**brief** The time between ticks, as a value: the last time it was given, the step since the one before (clamped), and how many ticks there have been. 24 bytes, no base class, nothing virtual, no clock of its own: the loop reads the clock once (`now()`) and passes the time in.

 One per loop: the UI's frames, the model's ticks. Not shared between threads.



## Members

public static duration longest_step



## Functions

### last

*public TimePoint last()*

*Defined at [time/timepiece.cppm:42](../../../../time/timepiece.cppm#L42)*



**brief** The time the last tick (or `start`) was given. 

### seconds

*public double seconds()*



**brief** The step the last tick reported, in seconds. 

### start

*public void start(TimePoint now)*



**brief** Count from `now`: the next tick measures from here. Clears the step and the count. 

### started

*public bool started()*

*Defined at [time/timepiece.cppm:48](../../../../time/timepiece.cppm#L48)*



**brief** Whether it has a time to measure from: a tick or `start` happened. 

### step

*public Duration step()*

*Defined at [time/timepiece.cppm:36](../../../../time/timepiece.cppm#L36)*



**brief** The step the last tick reported, in nanoseconds. 

### tick

*public double tick(TimePoint now)*



**brief** A tick at `now`: the seconds since the last tick (or since `start`), at most a quarter second; 0 for the first tick when there was no `start`, and 0 if `now` is earlier than the last time.

### ticks

*public std::uint64_t ticks()*

*Defined at [time/timepiece.cppm:45](../../../../time/timepiece.cppm#L45)*



**brief** Ticks since `start`, or since it was made. 

