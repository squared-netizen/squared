# class Save

*Defined at [stage/save.hpp:42](../../../stage/save.hpp#L42)*



**brief** What a model keeps when the app goes to the background: its `save( sq::Save &save )`, if it has one, writes the values it wants back.

 ```cpp void save( sq::Save &save ) noexcept {     save.state( settings_sound, sound_ );     save.state( settings_volume, volume_ ); } ```

 Each is kept as a `set` line (`set settings.volume 5.0`), and on the next start, before the model's `start`, each comes back to its `receive` as the UI's `set` would: a key the model takes `set` on is what it keeps, and nothing new is written to read them back.

 Over the model thread's stack (`capacity` bytes); it allocates nothing.



## Members

public static const std::size_t capacity



## Functions

### Save

*public void Save(std::span<char> out)*

*Defined at [stage/save.hpp:48](../../../stage/save.hpp#L48)*

### state

*public void state(const K & key, const V & value)*

*Defined at [stage/save.hpp:56](../../../stage/save.hpp#L56)*



**brief** Keep `value` for the key `key`, which takes `set`. 

### state

*public void state(const K & key, std::uint32_t uid, const V & value)*

*Defined at [stage/save.hpp:67](../../../stage/save.hpp#L67)*



**brief** Keep `value` for item `uid` of the key `key`, which takes `set` and a uid. 

### text

*public std::string_view text()*

*Defined at [stage/save.hpp:75](../../../stage/save.hpp#L75)*



**brief** The lines written, each ended by '**n** '. 

### lines

*public std::size_t lines()*

*Defined at [stage/save.hpp:78](../../../stage/save.hpp#L78)*



**brief** How many values are kept. 

### full

*public bool full()*

*Defined at [stage/save.hpp:81](../../../stage/save.hpp#L81)*



**brief** A value did not fit: the save is not written, and the last one stays. 



