# class Saving

*Defined at [stage/save.cppm:111](../../../../stage/save.cppm#L111)*



**brief** The model `M` as the App runs one that saves: the model's own hooks forwarded, the last save given back before its `start`, and `save` called after each `background` (a quit's too), on the model's thread.



## Functions

### Saving<M>

*public void Saving<M>(ModelFile file, Args &&... args)*

*Defined at [stage/save.cppm:115](../../../../stage/save.cppm#L115)*

### background

*public void background(messaging::Telegraph & telegraph)*

*Defined at [stage/save.cppm:145](../../../../stage/save.cppm#L145)*

### due

*public void due(messaging::Telegraph & telegraph, time::TimePoint now)*

*Defined at [stage/save.cppm:189](../../../../stage/save.cppm#L189)*

### foreground

*public void foreground(messaging::Telegraph & telegraph, std::uint64_t elapsed)*

*Defined at [stage/save.cppm:175](../../../../stage/save.cppm#L175)*

### low_memory

*public void low_memory(messaging::Telegraph & telegraph)*

*Defined at [stage/save.cppm:169](../../../../stage/save.cppm#L169)*

### receive

*public void receive(messaging::Telegraph & telegraph, const messaging::Received & received)*

*Defined at [stage/save.cppm:139](../../../../stage/save.cppm#L139)*

### start

*public void start(messaging::Telegraph & telegraph)*

*Defined at [stage/save.cppm:120](../../../../stage/save.cppm#L120)*

### tick

*public void tick(messaging::Telegraph & telegraph, double seconds)*

*Defined at [stage/save.cppm:163](../../../../stage/save.cppm#L163)*

### wake_at

*public std::optional<time::TimePoint> wake_at()*

*Defined at [stage/save.cppm:183](../../../../stage/save.cppm#L183)*

