# class TelegramRing

*Defined at [messaging/telegram_ring.hpp:56](../../../../messaging/telegram_ring.hpp#L56)*



**brief** One direction of the Telegraph: telegrams from one thread to one other, in order, with no lock and no allocation.

 Exactly one thread pushes and exactly one thread reads. A text or uid list is copied into the ring's data ring by `push`, and the telegram that arrives says where; it stays there until the telegram is popped. Over storage its owner gives it, sized at init.

 Neither side ever waits: a full ring says so (`RingResult::full`), and an empty one has no `front()`. Waking a consumer that sleeps is its owner's business.



## Functions

### TelegramRing

*public void TelegramRing(std::span<std::byte> slots, std::span<std::byte> data)*



**brief** A ring over `slots` (32 bytes a telegram) and `data` (text and uid lists). Both sizes are powers of two: at least 2 telegrams, and the data at least 16 bytes, or empty for a ring that carries no text. Asserted; the storage needs no alignment.

### TelegramRing

*public void TelegramRing(const TelegramRing & )*

*Defined at [messaging/telegram_ring.hpp:67](../../../../messaging/telegram_ring.hpp#L67)*

### operator=

*public TelegramRing & operator=(const TelegramRing & )*

*Defined at [messaging/telegram_ring.hpp:68](../../../../messaging/telegram_ring.hpp#L68)*

### TelegramRing

*public void TelegramRing(TelegramRing && )*

*Defined at [messaging/telegram_ring.hpp:69](../../../../messaging/telegram_ring.hpp#L69)*

### operator=

*public TelegramRing & operator=(TelegramRing && )*

*Defined at [messaging/telegram_ring.hpp:70](../../../../messaging/telegram_ring.hpp#L70)*

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

*Defined at [messaging/telegram_ring.hpp:71](../../../../messaging/telegram_ring.hpp#L71)*



