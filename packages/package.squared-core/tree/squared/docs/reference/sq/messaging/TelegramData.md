# class TelegramData

*Defined at [messaging/telegram_data.hpp:19](../../../../messaging/telegram_data.hpp#L19)*



**brief** A data area for telegrams' text and uid lists, filled from the front: where `read_telegram` puts what it reads, and what a test or a replay file hands `write_telegram`. Over storage its owner gives it; it allocates nothing. (The Telegraph's rings keep their own.)



## Functions

### TelegramData

*public void TelegramData(std::span<std::byte> storage)*

*Defined at [messaging/telegram_data.hpp:22](../../../../messaging/telegram_data.hpp#L22)*

### append

*public std::optional<DataRef> append(std::span<const std::byte> bytes)*



**brief** Copy `bytes` in. Nothing when they do not fit. 

### at

*public std::span<const std::byte> at(DataRef where)*



**brief** The bytes at `where`; empty when `where` is outside what was appended. 

### bytes

*public std::span<const std::byte> bytes()*

*Defined at [messaging/telegram_data.hpp:33](../../../../messaging/telegram_data.hpp#L33)*



**brief** Everything appended, for `write_telegram`. 

### reserve

*public std::optional<DataRef> reserve(std::size_t length)*



**brief** Room for `length` bytes at the end, to be filled through `writable`: for a reader that decodes in place. Nothing when it does not fit.

### rewind

*public void rewind(std::size_t used)*

*Defined at [messaging/telegram_data.hpp:49](../../../../messaging/telegram_data.hpp#L49)*



**brief** Forget what was appended after the first `used` bytes. 

### clear

*public void clear()*

*Defined at [messaging/telegram_data.hpp:52](../../../../messaging/telegram_data.hpp#L52)*



**brief** Forget everything appended. 

### used

*public std::size_t used()*

*Defined at [messaging/telegram_data.hpp:54](../../../../messaging/telegram_data.hpp#L54)*

### capacity

*public std::size_t capacity()*

*Defined at [messaging/telegram_data.hpp:55](../../../../messaging/telegram_data.hpp#L55)*

### writable

*public std::span<std::byte> writable(DataRef where)*



**brief** The bytes at `where`, to fill; empty outside what was appended. 



