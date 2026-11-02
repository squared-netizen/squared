# class TelegraphUi

*Defined at [messaging/telegraph.cppm:304](../../../../messaging/telegraph.cppm#L304)*



**brief** The UI's end of the Telegraph: the Stage's, on the UI thread. An app reaches it through its screens, not directly.

 It sends `set` and `do`, asks lists for rows, takes what the model sent into the latest-value cache (`drain`), reads values back, binds the ones on screen, and passes Android's lifecycle to the model. It never waits: a full ring is `full`. Each send wakes the model's thread if it sleeps; when the UI sleeps itself, `wake_fd` wakes it for what the model sends.



## Functions

### TelegraphUi

*public void TelegraphUi(TelegraphUi && )*

*Defined at [messaging/telegraph.cppm:309](../../../../messaging/telegraph.cppm#L309)*

### TelegraphUi

*public void TelegraphUi(const TelegraphUi & )*

*Defined at [messaging/telegraph.cppm:307](../../../../messaging/telegraph.cppm#L307)*

### background

*public void background()*



**brief** Android's lifecycle, for the model: it gets them at its next turn. 

### bind

*public bool bind(Address address)*



**brief** An actor shows `address`: its value is kept in the cache. 

### cache_stats

*public CacheStats cache_stats()*

### cached_from

*public std::optional<BoundEntry> cached_from(std::uint32_t from)*



**brief** A walk over every value the cache holds, as `ValueCache::entry_from`: from 0, then from the last found plus 1. A value's text is valid until the next `drain`.

### do_

*public DoResult do_(const K & key)*

*Defined at [messaging/telegraph.cppm:338](../../../../messaging/telegraph.cppm#L338)*



**brief** `do <key>`: an intent, with no value. 

### do_

*public DoResult do_(const K & key, const V & value)*

*Defined at [messaging/telegraph.cppm:347](../../../../messaging/telegraph.cppm#L347)*



**brief** `do <key> <value>`. 

### do_

*public DoResult do_(const K & key, std::uint32_t uid)*

*Defined at [messaging/telegraph.cppm:357](../../../../messaging/telegraph.cppm#L357)*



**brief** `do <key>[uid]`, for a key with no value that takes a uid (a row's button). 

### drain

*public std::uint32_t drain()*



**brief** `drain`, with events dropped. 

### drain

*public std::uint32_t drain(H & handler)*

*Defined at [messaging/telegraph.cppm:692](../../../../messaging/telegraph.cppm#L692)*



**brief** Take what the model sent: each `state` into the cache, then to `handler.state(received)` if it has one (what shows it is told it changed); each `event` to `handler.event(received)` if it has one. At most a ring's worth, so one frame never takes more. The telegrams taken.

### find

*public std::optional<CachedValue> find(Address address)*



**brief** The latest value at `address`, as it came. 

### foreground

*public void foreground(std::uint64_t elapsed_milliseconds)*

### low_memory

*public void low_memory()*

### model_stats

*public ModelStats model_stats()*

### names

*public const NameTable * names()*

### operator=

*public TelegraphUi & operator=(TelegraphUi && )*

*Defined at [messaging/telegraph.cppm:310](../../../../messaging/telegraph.cppm#L310)*

### operator=

*public TelegraphUi & operator=(const TelegraphUi & )*

*Defined at [messaging/telegraph.cppm:308](../../../../messaging/telegraph.cppm#L308)*

### quit

*public void quit()*

### ready_to_sleep

*public bool ready_to_sleep()*



**brief** Before sleeping on `wake_fd`: false when the model has sent already (drain instead). True means sleep: the next send wakes it.

### register_names

*public void register_names(const K &... keys)*

*Defined at [messaging/telegraph.cppm:434](../../../../messaging/telegraph.cppm#L434)*



**brief** Add each key's name, for names only the model sends. Nothing without a table. 

### reset_view

*public SendResult reset_view(Name rows, std::string_view text)*



**brief** `do <list>.view (1 0)`: a new list widget, which knows nothing. The model forgets what it sent, so the next `view` is answered in full, and sends the count.

### send

*public SendResult send(const Telegram & telegram, std::span<const std::byte> data)*



**brief** Send a telegram as it is, read from its text (a replay): its text or uid list is in `data`, as `read_telegram` left it. Meant for `set` and `do`; nothing checks its type against a key.

### set

*public SendResult set(const K & key, const V & value)*

*Defined at [messaging/telegraph.cppm:316](../../../../messaging/telegraph.cppm#L316)*



**brief** `set <key> <value>`: the user changed a value. 

### set

*public SendResult set(const K & key, std::uint32_t uid, const V & value)*

*Defined at [messaging/telegraph.cppm:327](../../../../messaging/telegraph.cppm#L327)*



**brief** `set <key>[uid] <value>`. 

### set_names

*public void set_names(NameTable * names)*



**brief** Where the names of keys go, for the text form: a table the dev tools own (nullptr, the default, keeps none). Each key the UI uses (`set`, `do_`, `view`, `value`) is added the first time, and `register_names` adds others. Not owned; it must outlive its use.

### set_tap

*public void set_tap(Tap tap, void * context)*



**brief** Hand each telegram the UI sends, and each it drains, to `tap` (nullptr, the default: none). One at a time.

### to_model_stats

*public RingStats to_model_stats()*

### to_ui_stats

*public RingStats to_ui_stats()*

### unbind

*public void unbind(Address address)*

### value

*public detail::value_of<K> value(const K & key)*

*Defined at [messaging/telegraph.cppm:407](../../../../messaging/telegraph.cppm#L407)*



**brief** The latest value of `key`, as its type (text: valid until the next `drain`). 

### value

*public detail::value_of<K> value(const K & key, std::uint32_t uid)*

*Defined at [messaging/telegraph.cppm:416](../../../../messaging/telegraph.cppm#L416)*



**brief** The latest value of `key` at `uid`, as its type. 

### view

*public SendResult view(Name rows, std::string_view text, RowSpan span)*



**brief** The same, for a list known by its name and its dotted text (a layout's list node: `rows`, `layout.text( bind_text )`).

### view

*public SendResult view(const K & rows, RowSpan span)*

*Defined at [messaging/telegraph.cppm:675](../../../../messaging/telegraph.cppm#L675)*



**brief** `do <list>.view (first last)`: the rows a list widget shows (`ListWindow`). 

### wait_stats

*public WaitStats wait_stats()*

### wake_fd

*public int wake_fd()*



**brief** The descriptor that becomes readable when the model sends while the UI sleeps: poll it beside the others (Android's looper, a desktop event loop). -1 if it could not be made.

### woke

*public void woke()*



**brief** After sleeping, woken or not: the model no longer wakes the UI. 

### ~TelegraphUi

*public void ~TelegraphUi()*

*Defined at [messaging/telegraph.cppm:311](../../../../messaging/telegraph.cppm#L311)*

