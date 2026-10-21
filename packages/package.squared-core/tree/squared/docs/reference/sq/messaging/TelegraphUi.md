# class TelegraphUi

*Defined at [messaging/telegraph.hpp:310](../../../../messaging/telegraph.hpp#L310)*



**brief** The UI's end of the Telegraph: the Stage's, on the UI thread. An app reaches it through its screens, not directly.

 It sends `set` and `do`, asks lists for rows, takes what the model sent into the latest-value cache (`drain`), reads values back, binds the ones on screen, and passes Android's lifecycle to the model. It never waits: a full ring is `full`. Each send wakes the model's thread if it sleeps; when the UI sleeps itself, `wake_fd` wakes it for what the model sends.



## Functions

### TelegraphUi

*public void TelegraphUi(const TelegraphUi & )*

*Defined at [messaging/telegraph.hpp:313](../../../../messaging/telegraph.hpp#L313)*

### operator=

*public TelegraphUi & operator=(const TelegraphUi & )*

*Defined at [messaging/telegraph.hpp:314](../../../../messaging/telegraph.hpp#L314)*

### TelegraphUi

*public void TelegraphUi(TelegraphUi && )*

*Defined at [messaging/telegraph.hpp:315](../../../../messaging/telegraph.hpp#L315)*

### operator=

*public TelegraphUi & operator=(TelegraphUi && )*

*Defined at [messaging/telegraph.hpp:316](../../../../messaging/telegraph.hpp#L316)*

### ready_to_sleep

*public bool ready_to_sleep()*



**brief** Before sleeping on `wake_fd`: false when the model has sent already (drain instead). True means sleep: the next send wakes it.

### ~TelegraphUi

*public void ~TelegraphUi()*

*Defined at [messaging/telegraph.hpp:317](../../../../messaging/telegraph.hpp#L317)*

### set

*public SendResult set(const K & key, const V & value)*

*Defined at [messaging/telegraph.hpp:322](../../../../messaging/telegraph.hpp#L322)*



**brief** `set <key> <value>`: the user changed a value. 

### set

*public SendResult set(const K & key, std::uint32_t uid, const V & value)*

*Defined at [messaging/telegraph.hpp:332](../../../../messaging/telegraph.hpp#L332)*



**brief** `set <key>[uid] <value>`. 

### do_

*public DoResult do_(const K & key)*

*Defined at [messaging/telegraph.hpp:342](../../../../messaging/telegraph.hpp#L342)*



**brief** `do <key>`: an intent, with no value. 

### do_

*public DoResult do_(const K & key, const V & value)*

*Defined at [messaging/telegraph.hpp:350](../../../../messaging/telegraph.hpp#L350)*



**brief** `do <key> <value>`. 

### do_

*public DoResult do_(const K & key, std::uint32_t uid)*

*Defined at [messaging/telegraph.hpp:359](../../../../messaging/telegraph.hpp#L359)*



**brief** `do <key>[uid]`, for a key with no value that takes a uid (a row's button). 

### drain

*public std::uint32_t drain()*



**brief** `drain`, with events dropped. 

### find

*public std::optional<CachedValue> find(Address address)*



**brief** The latest value at `address`, as it came. 

### bind

*public bool bind(Address address)*



**brief** An actor shows `address`: its value is kept in the cache. 

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

*Defined at [messaging/telegraph.hpp:367](../../../../messaging/telegraph.hpp#L367)*



**brief** `do <list>.view (first last)`: the rows a list widget shows (`ListWindow`). 

### value

*public detail::value_of<K> value(const K & key)*

*Defined at [messaging/telegraph.hpp:389](../../../../messaging/telegraph.hpp#L389)*



**brief** The latest value of `key`, as its type (text: valid until the next `drain`). 

### value

*public detail::value_of<K> value(const K & key, std::uint32_t uid)*

*Defined at [messaging/telegraph.hpp:397](../../../../messaging/telegraph.hpp#L397)*



**brief** The latest value of `key` at `uid`, as its type. 

### drain

*public std::uint32_t drain(H & handler)*

*Defined at [messaging/telegraph.hpp:606](../../../../messaging/telegraph.hpp#L606)*



**brief** Take what the model sent: each `state` into the cache, each `event` to `handler.event(received)` if it has one. At most a ring's worth, so one frame never takes more. The telegrams taken.

### wake_fd

*public int wake_fd()*



**brief** The descriptor that becomes readable when the model sends while the UI sleeps: poll it beside the others (Android's looper, a desktop event loop). -1 if it could not be made.

### woke

*public void woke()*



**brief** After sleeping, woken or not: the model no longer wakes the UI. 

### wait_stats

*public WaitStats wait_stats()*



