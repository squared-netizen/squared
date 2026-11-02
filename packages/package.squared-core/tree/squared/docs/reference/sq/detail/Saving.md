# class Saving

*Defined at [stage/save.cppm:111](../../../../stage/save.cppm#L111)*

## Functions

### Saving<M>

*public void Saving<M>(ModelFile file, Args &&... args)*

*Defined at [stage/save.cppm:115](../../../../stage/save.cppm#L115)*

### start

*public void start(messaging::Telegraph & telegraph)*

*Defined at [stage/save.cppm:120](../../../../stage/save.cppm#L120)*

### receive

*public void receive(messaging::Telegraph & telegraph, const messaging::Received & received)*

*Defined at [stage/save.cppm:139](../../../../stage/save.cppm#L139)*

### background

*public void background(messaging::Telegraph & telegraph)*

*Defined at [stage/save.cppm:145](../../../../stage/save.cppm#L145)*

### tick

*public void tick(messaging::Telegraph & telegraph, double seconds)*

*Defined at [stage/save.cppm:163](../../../../stage/save.cppm#L163)*

### low_memory

*public void low_memory(messaging::Telegraph & telegraph)*

*Defined at [stage/save.cppm:169](../../../../stage/save.cppm#L169)*

### foreground

*public void foreground(messaging::Telegraph & telegraph, std::uint64_t elapsed)*

*Defined at [stage/save.cppm:175](../../../../stage/save.cppm#L175)*

### wake_at

*public std::optional<time::TimePoint> wake_at()*

*Defined at [stage/save.cppm:183](../../../../stage/save.cppm#L183)*

### due

*public void due(messaging::Telegraph & telegraph, time::TimePoint now)*

*Defined at [stage/save.cppm:189](../../../../stage/save.cppm#L189)*



