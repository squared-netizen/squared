# class TickClock

*Defined at [messaging/model_runner.cppm:59](../../../../../messaging/model_runner.cppm#L59)*

 When a model's `tick` is due; its steps measured by a `time::Timepiece`.



## Functions

### take

*public double take(time::TimePoint now)*



**brief** A tick happens at `now`: the seconds since the last, at most a quarter second. 

### until_due

*public int until_due(std::uint32_t interval_ms, time::TimePoint now)*



**brief** Milliseconds from `now` until the next tick: 0 when due, -1 when there is none. 

