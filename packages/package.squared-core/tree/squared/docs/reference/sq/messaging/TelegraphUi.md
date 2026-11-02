# class TelegraphUi

*Defined at [messaging/telegraph.cppm:304](../../../../messaging/telegraph.cppm#L304)*

## Functions

### TelegraphUi

*public void TelegraphUi(const TelegraphUi & )*

*Defined at [messaging/telegraph.cppm:307](../../../../messaging/telegraph.cppm#L307)*

### operator=

*public TelegraphUi & operator=(const TelegraphUi & )*

*Defined at [messaging/telegraph.cppm:308](../../../../messaging/telegraph.cppm#L308)*

### TelegraphUi

*public void TelegraphUi(TelegraphUi && )*

*Defined at [messaging/telegraph.cppm:309](../../../../messaging/telegraph.cppm#L309)*

### operator=

*public TelegraphUi & operator=(TelegraphUi && )*

*Defined at [messaging/telegraph.cppm:310](../../../../messaging/telegraph.cppm#L310)*

### set

*public SendResult set(const K & key, const V & value)*

*Defined at [messaging/telegraph.cppm:316](../../../../messaging/telegraph.cppm#L316)*

### set

*public SendResult set(const K & key, std::uint32_t uid, const V & value)*

*Defined at [messaging/telegraph.cppm:327](../../../../messaging/telegraph.cppm#L327)*

### do_

*public DoResult do_(const K & key)*

*Defined at [messaging/telegraph.cppm:338](../../../../messaging/telegraph.cppm#L338)*

### do_

*public DoResult do_(const K & key, const V & value)*

*Defined at [messaging/telegraph.cppm:347](../../../../messaging/telegraph.cppm#L347)*

### do_

*public DoResult do_(const K & key, std::uint32_t uid)*

*Defined at [messaging/telegraph.cppm:357](../../../../messaging/telegraph.cppm#L357)*

### register_names

*public void register_names(const K &... keys)*

*Defined at [messaging/telegraph.cppm:434](../../../../messaging/telegraph.cppm#L434)*

### view

*public SendResult view(const K & rows, RowSpan span)*

*Defined at [messaging/telegraph.cppm:675](../../../../messaging/telegraph.cppm#L675)*

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

*Defined at [messaging/telegraph.cppm:692](../../../../messaging/telegraph.cppm#L692)*

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

*Defined at [messaging/telegraph.cppm:311](../../../../messaging/telegraph.cppm#L311)*

### value

*public detail::value_of<K> value(const K & key)*

*Defined at [messaging/telegraph.cppm:407](../../../../messaging/telegraph.cppm#L407)*

### value

*public detail::value_of<K> value(const K & key, std::uint32_t uid)*

*Defined at [messaging/telegraph.cppm:416](../../../../messaging/telegraph.cppm#L416)*

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



