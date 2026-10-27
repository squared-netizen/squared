# class TelegraphUi

*Defined at [messaging/telegraph.hpp:313](../../../../messaging/telegraph.hpp#L313)*



**brief** The UI's end of the Telegraph: the Stage's, on the UI thread. An app reaches it through its screens, not directly.

 It sends `set` and `do`, asks lists for rows, takes what the model sent into the latest-value cache (`drain`), reads values back, binds the ones on screen, and passes Android's lifecycle to the model. It never waits: a full ring is `full`. Each send wakes the model's thread if it sleeps; when the UI sleeps itself, `wake_fd` wakes it for what the model sends.



## Functions

### TelegraphUi

*public void TelegraphUi(const TelegraphUi & )*

*Defined at [messaging/telegraph.hpp:316](../../../../messaging/telegraph.hpp#L316)*

### operator=

*public TelegraphUi & operator=(const TelegraphUi & )*

*Defined at [messaging/telegraph.hpp:317](../../../../messaging/telegraph.hpp#L317)*

### TelegraphUi

*public void TelegraphUi(TelegraphUi && )*

*Defined at [messaging/telegraph.hpp:318](../../../../messaging/telegraph.hpp#L318)*

### operator=

*public TelegraphUi & operator=(TelegraphUi && )*

*Defined at [messaging/telegraph.hpp:319](../../../../messaging/telegraph.hpp#L319)*

### set

*public SendResult set(const K & key, const V & value)*

*Defined at [messaging/telegraph.hpp:325](../../../../messaging/telegraph.hpp#L325)*



**brief** `set <key> <value>`: the user changed a value. 

### set

*public SendResult set(const K & key, std::uint32_t uid, const V & value)*

*Defined at [messaging/telegraph.hpp:336](../../../../messaging/telegraph.hpp#L336)*



**brief** `set <key>[uid] <value>`. 

### do_

*public DoResult do_(const K & key)*

*Defined at [messaging/telegraph.hpp:347](../../../../messaging/telegraph.hpp#L347)*



**brief** `do <key>`: an intent, with no value. 

### do_

*public DoResult do_(const K & key, const V & value)*

*Defined at [messaging/telegraph.hpp:356](../../../../messaging/telegraph.hpp#L356)*



**brief** `do <key> <value>`. 

### do_

*public DoResult do_(const K & key, std::uint32_t uid)*

*Defined at [messaging/telegraph.hpp:366](../../../../messaging/telegraph.hpp#L366)*



**brief** `do <key>[uid]`, for a key with no value that takes a uid (a row's button). 

### register_names

*public void register_names(const K &... keys)*

*Defined at [messaging/telegraph.hpp:443](../../../../messaging/telegraph.hpp#L443)*



**brief** Add each key's name, for names only the model sends. Nothing without a table. 

### view

*public SendResult view(const K & rows, RowSpan span)*

*Defined at [messaging/telegraph.hpp:684](../../../../messaging/telegraph.hpp#L684)*



**brief** `do <list>.view (first last)`: the rows a list widget shows (`ListWindow`). 

### view

*public SendResult view(Name rows, std::string_view text, RowSpan span)*



**brief** The same, for a list known by its name and its dotted text (a layout's list node: `rows`, `layout.text( bind_text )`).

### reset_view

*public SendResult reset_view(Name rows, std::string_view text)*



**brief** `do <list>.view (1 0)`: a new list widget, which knows nothing. The model forgets what it sent, so the next `view` is answered in full, and sends the count.

### send

*public SendResult send(const Telegram & telegram, std::span<const std::byte> data)*



**brief** Send a telegram as it is, read from its text (a replay): its text or uid list is in `data`, as `read_telegram` left it. Meant for `set` and `do`; nothing checks its type against a key.

### drain

*public std::uint32_t drain(H & handler)*

*Defined at [messaging/telegraph.hpp:701](../../../../messaging/telegraph.hpp#L701)*



**brief** Take what the model sent: each `state` into the cache, then to `handler.state(received)` if it has one (what shows it is told it changed); each `event` to `handler.event(received)` if it has one. At most a ring's worth, so one frame never takes more. The telegrams taken.

### drain

*public std::uint32_t drain()*



**brief** `drain`, with events dropped. 

### find

*public std::optional<CachedValue> find(Address address)*



**brief** The latest value at `address`, as it came. 

### set_names

*public void set_names(NameTable * names)*



**brief** Where the names of keys go, for the text form: a table the dev tools own (nullptr, the default, keeps none). Each key the UI uses (`set`, `do_`, `view`, `value`) is added the first time, and `register_names` adds others. Not owned; it must outlive its use.

### names

*public const NameTable * names()*

### set_tap

*public void set_tap(Tap tap, void * context)*



**brief** Hand each telegram the UI sends, and each it drains, to `tap` (nullptr, the default: none). One at a time.

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

### ~TelegraphUi

*public void ~TelegraphUi()*

*Defined at [messaging/telegraph.hpp:320](../../../../messaging/telegraph.hpp#L320)*

### value

*public detail::value_of<K> value(const K & key)*

*Defined at [messaging/telegraph.hpp:416](../../../../messaging/telegraph.hpp#L416)*



**brief** The latest value of `key`, as its type (text: valid until the next `drain`). 

### value

*public detail::value_of<K> value(const K & key, std::uint32_t uid)*

*Defined at [messaging/telegraph.hpp:425](../../../../messaging/telegraph.hpp#L425)*



**brief** The latest value of `key` at `uid`, as its type. 

### wake_fd

*public int wake_fd()*



**brief** The descriptor that becomes readable when the model sends while the UI sleeps: poll it beside the others (Android's looper, a desktop event loop). -1 if it could not be made.

### ready_to_sleep

*public bool ready_to_sleep()*



**brief** Before sleeping on `wake_fd`: false when the model has sent already (drain instead). True means sleep: the next send wakes it.

### woke

*public void woke()*



**brief** After sleeping, woken or not: the model no longer wakes the UI. 

### cache_stats

*public CacheStats cache_stats()*

### to_model_stats

*public RingStats to_model_stats()*

### to_ui_stats

*public RingStats to_ui_stats()*

### wait_stats

*public WaitStats wait_stats()*

### model_stats

*public ModelStats model_stats()*

### cached_from

*public std::optional<BoundEntry> cached_from(std::uint32_t from)*



**brief** A walk over every value the cache holds, as `ValueCache::entry_from`: from 0, then from the last found plus 1. A value's text is valid until the next `drain`.



