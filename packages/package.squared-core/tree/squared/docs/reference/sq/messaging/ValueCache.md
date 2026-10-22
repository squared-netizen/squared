# class ValueCache

*Defined at [messaging/value_cache.hpp:67](../../../../messaging/value_cache.hpp#L67)*



**brief** The latest value of each address, on the UI side: a screen built later shows current values at once.

 An address an actor is bound to (`bind`) keeps its entry; the others are evicted when room is needed, roughly oldest first. Text and uid lists up to 16 bytes are held in the entry; longer ones in a text area, which is compacted when it fills. Over storage its owner gives it, sized at init; nothing allocates. One thread only.



## Functions

### ValueCache

*public void ValueCache(std::span<std::byte> entries, std::span<std::byte> index, std::span<std::byte> text)*



**brief** A cache over `entries` (32 bytes an entry, a power of two of them, 2 to 32,768), `index` (`index_bytes`) and `text` (text and uid lists over 16 bytes; may be empty). Asserted; the storage needs no alignment.

### index_bytes

*public static std::size_t index_bytes(std::size_t entries)*

*Defined at [messaging/value_cache.hpp:71](../../../../messaging/value_cache.hpp#L71)*



**brief** Index bytes a cache of `entries` entries needs: 4 an entry. 

### ValueCache

*public void ValueCache(const ValueCache & )*

*Defined at [messaging/value_cache.hpp:85](../../../../messaging/value_cache.hpp#L85)*

### operator=

*public ValueCache & operator=(const ValueCache & )*

*Defined at [messaging/value_cache.hpp:86](../../../../messaging/value_cache.hpp#L86)*

### ValueCache

*public void ValueCache(ValueCache && )*

*Defined at [messaging/value_cache.hpp:87](../../../../messaging/value_cache.hpp#L87)*

### operator=

*public ValueCache & operator=(ValueCache && )*

*Defined at [messaging/value_cache.hpp:88](../../../../messaging/value_cache.hpp#L88)*

### store

*public CacheResult store(const Telegram & telegram, std::span<const std::byte> data)*



**brief** Make `telegram`'s value the latest for its address. A `text` or `uids` value is taken from `data` (as a ring delivers it); any other type takes no data. On `dropped` or `too_large` the address keeps the value it had.

### find

*public std::optional<CachedValue> find(Address address)*



**brief** The latest value of `address`; nothing if none is cached. 

### bind

*public bool bind(Address address)*



**brief** An actor shows `address`: its entry is kept (made now if there is none, with no value yet). False when there is no room (every entry bound) or 255 actors already show it.

### unbind

*public void unbind(Address address)*



**brief** One actor fewer shows `address`. It must be bound. 

### bindings

*public std::uint32_t bindings(Address address)*



**brief** How many binds `address` has; 0 when it has none or no entry. 

### bound_from

*public std::optional<BoundEntry> bound_from(std::uint32_t from)*



**brief** The first bound entry numbered `from` or after; nothing when there is none. A walk over the bound entries: from 0, then from the last one found plus 1. Unbinding the one found does not disturb it.

### entry_from

*public std::optional<BoundEntry> entry_from(std::uint32_t from)*



**brief** The first entry in use numbered `from` or after, bound or not: a walk over every address the cache holds, as `bound_from` walks the bound ones (the Console's `telegraph state`).

### stats

*public CacheStats stats()*

### ~ValueCache

*public void ~ValueCache()*

*Defined at [messaging/value_cache.hpp:89](../../../../messaging/value_cache.hpp#L89)*



