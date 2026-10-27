# class Saving

*Defined at [stage/save.hpp:124](../../../../stage/save.hpp#L124)*



**brief** The model `M` as the App runs one that saves: the model's own hooks forwarded, the last save given back before its `start`, and `save` called after each `background` (a quit's too), on the model's thread.



## Functions

### Saving<M>

*public void Saving<M>(ModelFile file, Args &&... args)*

*Defined at [stage/save.hpp:128](../../../../stage/save.hpp#L128)*

### start

*public void start(messaging::Telegraph & telegraph)*

*Defined at [stage/save.hpp:133](../../../../stage/save.hpp#L133)*

### receive

*public void receive(messaging::Telegraph & telegraph, const messaging::Received & received)*

*Defined at [stage/save.hpp:152](../../../../stage/save.hpp#L152)*

### background

*public void background(messaging::Telegraph & telegraph)*

*Defined at [stage/save.hpp:158](../../../../stage/save.hpp#L158)*

### tick

*public void tick(messaging::Telegraph & telegraph, double seconds)*

*Defined at [stage/save.hpp:176](../../../../stage/save.hpp#L176)*

### low_memory

*public void low_memory(messaging::Telegraph & telegraph)*

*Defined at [stage/save.hpp:182](../../../../stage/save.hpp#L182)*

### foreground

*public void foreground(messaging::Telegraph & telegraph, std::uint64_t elapsed)*

*Defined at [stage/save.hpp:188](../../../../stage/save.hpp#L188)*

### wake_at

*public std::optional<time::TimePoint> wake_at()*

*Defined at [stage/save.hpp:196](../../../../stage/save.hpp#L196)*

### due

*public void due(messaging::Telegraph & telegraph, time::TimePoint now)*

*Defined at [stage/save.hpp:202](../../../../stage/save.hpp#L202)*



