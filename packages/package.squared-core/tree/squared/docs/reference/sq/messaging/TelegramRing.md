# class TelegramRing

*Defined at [messaging/telegram_ring.cppm:51](../../../../messaging/telegram_ring.cppm#L51)*

## Functions

### TelegramRing

*public void TelegramRing(std::span<std::byte> slots, std::span<std::byte> data)*



**brief** A ring over `slots` (32 bytes a telegram) and `data` (text and uid lists). Both sizes are powers of two: at least 2 telegrams, and the data at least 16 bytes, or empty for a ring that carries no text. Asserted; the storage needs no alignment.

### TelegramRing

*public void TelegramRing(const TelegramRing & )*

*Defined at [messaging/telegram_ring.cppm:62](../../../../messaging/telegram_ring.cppm#L62)*

### operator=

*public TelegramRing & operator=(const TelegramRing & )*

*Defined at [messaging/telegram_ring.cppm:63](../../../../messaging/telegram_ring.cppm#L63)*

### TelegramRing

*public void TelegramRing(TelegramRing && )*

*Defined at [messaging/telegram_ring.cppm:64](../../../../messaging/telegram_ring.cppm#L64)*

### operator=

*public TelegramRing & operator=(TelegramRing && )*

*Defined at [messaging/telegram_ring.cppm:65](../../../../messaging/telegram_ring.cppm#L65)*

### push

*public RingResult push(const Telegram & telegram, std::span<const std::byte> data)*



**brief** Send `telegram`. A `text` or `uids` telegram takes its value from `data` (its own `DataRef` is replaced by where the bytes went); any other type takes no data. A uid list is 4 bytes a uid.

### front

*public std::optional<Received> front()*



**brief** The oldest telegram not yet popped; nothing when the ring is empty. 

### pop

*public void pop()*



**brief** Done with the front telegram and its data. The ring must not be empty. 

### stats

*public RingStats stats()*



**brief** The ring's sizes and the highest fill so far. 

### ~TelegramRing

*public void ~TelegramRing()*

*Defined at [messaging/telegram_ring.cppm:66](../../../../messaging/telegram_ring.cppm#L66)*



