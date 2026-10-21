# class TickClock

*Defined at [messaging/model_runner.hpp:61](../../../../../messaging/model_runner.hpp#L61)*

 When a model's `tick` is due; its steps measured by a `time::Timepiece`.



## Functions

### until_due

*public int until_due(std::uint32_t interval_ms, time::TimePoint now)*



**brief** Milliseconds from `now` until the next tick: 0 when due, -1 when there is none. 

### take

*public double take(time::TimePoint now)*



**brief** A tick happens at `now`: the seconds since the last, at most a quarter second. 



