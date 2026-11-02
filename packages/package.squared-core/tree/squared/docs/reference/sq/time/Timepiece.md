# class Timepiece

*Defined at [time/timepiece.cppm:19](../../../../time/timepiece.cppm#L19)*

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

*Defined at [time/timepiece.cppm:36](../../../../time/timepiece.cppm#L36)*

### last

*public TimePoint last()*

*Defined at [time/timepiece.cppm:42](../../../../time/timepiece.cppm#L42)*

### ticks

*public std::uint64_t ticks()*

*Defined at [time/timepiece.cppm:45](../../../../time/timepiece.cppm#L45)*

### started

*public bool started()*

*Defined at [time/timepiece.cppm:48](../../../../time/timepiece.cppm#L48)*



