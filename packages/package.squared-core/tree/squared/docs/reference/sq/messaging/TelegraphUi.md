# class TelegraphUi

*Defined at [messaging/telegraph.hpp:270](../../../../messaging/telegraph.hpp#L270)*



**brief** The UI's end of the Telegraph: the Stage's, on the UI thread. An app reaches it through its screens, not directly.

 It sends `set` and `do`, asks lists for rows, takes what the model sent into the latest-value cache (`drain`), reads values back, binds the ones on screen, and passes Android's lifecycle to the model. It never waits: a full ring is `full`.



## Functions

### TelegraphUi

*public void TelegraphUi(const TelegraphUi & )*

*Defined at [messaging/telegraph.hpp:273](../../../../messaging/telegraph.hpp#L273)*

### TelegraphUi

*public void TelegraphUi(TelegraphUi && )*

*Defined at [messaging/telegraph.hpp:275](../../../../messaging/telegraph.hpp#L275)*

### do_

*public DoResult do_(const K & key)*

*Defined at [messaging/telegraph.hpp:302](../../../../messaging/telegraph.hpp#L302)*



**brief** `do <key>`: an intent, with no value. 

### do_

*public DoResult do_(const K & key, const V & value)*

*Defined at [messaging/telegraph.hpp:310](../../../../messaging/telegraph.hpp#L310)*



**brief** `do <key> <value>`. 

### do_

*public DoResult do_(const K & key, std::uint32_t uid)*

*Defined at [messaging/telegraph.hpp:319](../../../../messaging/telegraph.hpp#L319)*



**brief** `do <key>[uid]`, for a key with no value that takes a uid (a row's button). 

### drain

*public std::uint32_t drain(H & handler)*

*Defined at [messaging/telegraph.hpp:532](../../../../messaging/telegraph.hpp#L532)*



**brief** Take what the model sent: each `state` into the cache, each `event` to `handler.event(received)` if it has one. At most a ring's worth, so one frame never takes more. The telegrams taken.

### drain

*public std::uint32_t drain()*



**brief** `drain`, with events dropped. 

### find

*public std::optional<CachedValue> find(Address address)*



**brief** The latest value at `address`, as it came. 

### bind

*public bool bind(Address address)*



**brief** An actor shows `address`: its value is kept in the cache. 

### operator=

*public TelegraphUi & operator=(const TelegraphUi & )*

*Defined at [messaging/telegraph.hpp:274](../../../../messaging/telegraph.hpp#L274)*

### operator=

*public TelegraphUi & operator=(TelegraphUi && )*

*Defined at [messaging/telegraph.hpp:276](../../../../messaging/telegraph.hpp#L276)*

### ~TelegraphUi

*public void ~TelegraphUi()*

*Defined at [messaging/telegraph.hpp:277](../../../../messaging/telegraph.hpp#L277)*

### set

*public SendResult set(const K & key, const V & value)*

*Defined at [messaging/telegraph.hpp:282](../../../../messaging/telegraph.hpp#L282)*



**brief** `set <key> <value>`: the user changed a value. 

### set

*public SendResult set(const K & key, std::uint32_t uid, const V & value)*

*Defined at [messaging/telegraph.hpp:292](../../../../messaging/telegraph.hpp#L292)*



**brief** `set <key>[uid] <value>`. 

### unbind

*public void unbind(Address address)*

### background

*public void background()*



**brief** Android's lifecycle, for the model: it gets them at its next turn. 

### foreground

*public void foreground(std::uint64_t elapsed_milliseconds)*

### low_memory

*public void low_memory()*

### quit

*public void quit()*

### cache_stats

*public CacheStats cache_stats()*

### to_model_stats

*public RingStats to_model_stats()*

### to_ui_stats

*public RingStats to_ui_stats()*

### view

*public SendResult view(const K & rows, RowSpan span)*

*Defined at [messaging/telegraph.hpp:327](../../../../messaging/telegraph.hpp#L327)*



**brief** `do <list>.view (first last)`: the rows a list widget shows (`ListWindow`). 

### value

*public detail::value_of<K> value(const K & key)*

*Defined at [messaging/telegraph.hpp:349](../../../../messaging/telegraph.hpp#L349)*



**brief** The latest value of `key`, as its type (text: valid until the next `drain`). 

### value

*public detail::value_of<K> value(const K & key, std::uint32_t uid)*

*Defined at [messaging/telegraph.hpp:357](../../../../messaging/telegraph.hpp#L357)*



**brief** The latest value of `key` at `uid`, as its type. 



