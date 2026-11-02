# class Save

*Defined at [stage/save.cppm:29](../../../stage/save.cppm#L29)*



**brief** What a model keeps when the app goes to the background: its `save( sq::Save &save )`, if it has one, writes the values it wants back.

 ```cpp void save( sq::Save &save ) noexcept {     save.state( settings_sound, sound_ );     save.state( settings_volume, volume_ ); } ```

 Each is kept as a `set` line (`set settings.volume 5.0`), and on the next start, before the model's `start`, each comes back to its `receive` as the UI's `set` would: a key the model takes `set` on is what it keeps, and nothing new is written to read them back.

 Over the model thread's stack (`capacity` bytes); it allocates nothing.



## Members

public static const std::size_t capacity



## Functions

### Save

*public void Save(std::span<char> out)*

*Defined at [stage/save.cppm:35](../../../stage/save.cppm#L35)*

### full

*public bool full()*

*Defined at [stage/save.cppm:68](../../../stage/save.cppm#L68)*



**brief** A value did not fit: the save is not written, and the last one stays. 

### lines

*public std::size_t lines()*

*Defined at [stage/save.cppm:65](../../../stage/save.cppm#L65)*



**brief** How many values are kept. 

### state

*public void state(const K & key, const V & value)*

*Defined at [stage/save.cppm:43](../../../stage/save.cppm#L43)*



**brief** Keep `value` for the key `key`, which takes `set`. 

### state

*public void state(const K & key, std::uint32_t uid, const V & value)*

*Defined at [stage/save.cppm:54](../../../stage/save.cppm#L54)*



**brief** Keep `value` for item `uid` of the key `key`, which takes `set` and a uid. 

### text

*public std::string_view text()*

*Defined at [stage/save.cppm:62](../../../stage/save.cppm#L62)*



**brief** The lines written, each ended by '**n** '. 

