# class TickClock

*Defined at [messaging/model_runner.hpp:41](../../../../../messaging/model_runner.hpp#L41)*

 When a model's `tick` is due, and the seconds since the last one.



## Functions

### until_due

*public int until_due(std::uint32_t interval_ms)*



**brief** Milliseconds until the next tick: 0 when due, -1 when there is none. 

### take

*public double take()*



**brief** A tick happens now: the seconds since the last, at most a quarter second. 



