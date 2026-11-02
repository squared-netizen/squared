# class TelegramData

*Defined at [messaging/telegram_data.cppm:15](../../../../messaging/telegram_data.cppm#L15)*



**brief** A data area for telegrams' text and uid lists, filled from the front: where `read_telegram` puts what it reads, and what a test or a replay file hands `write_telegram`. Over storage its owner gives it; it allocates nothing. (The Telegraph's rings keep their own.)



## Functions

### TelegramData

*public void TelegramData(std::span<std::byte> storage)*

*Defined at [messaging/telegram_data.cppm:18](../../../../messaging/telegram_data.cppm#L18)*

### append

*public std::optional<DataRef> append(std::span<const std::byte> bytes)*



**brief** Copy `bytes` in. Nothing when they do not fit. 

### at

*public std::span<const std::byte> at(DataRef where)*



**brief** The bytes at `where`; empty when `where` is outside what was appended. 

### bytes

*public std::span<const std::byte> bytes()*

*Defined at [messaging/telegram_data.cppm:29](../../../../messaging/telegram_data.cppm#L29)*



**brief** Everything appended, for `write_telegram`. 

### capacity

*public std::size_t capacity()*

*Defined at [messaging/telegram_data.cppm:51](../../../../messaging/telegram_data.cppm#L51)*

### clear

*public void clear()*

*Defined at [messaging/telegram_data.cppm:48](../../../../messaging/telegram_data.cppm#L48)*



**brief** Forget everything appended. 

### reserve

*public std::optional<DataRef> reserve(std::size_t length)*



**brief** Room for `length` bytes at the end, to be filled through `writable`: for a reader that decodes in place. Nothing when it does not fit.

### rewind

*public void rewind(std::size_t used)*

*Defined at [messaging/telegram_data.cppm:45](../../../../messaging/telegram_data.cppm#L45)*



**brief** Forget what was appended after the first `used` bytes. 

### used

*public std::size_t used()*

*Defined at [messaging/telegram_data.cppm:50](../../../../messaging/telegram_data.cppm#L50)*

### writable

*public std::span<std::byte> writable(DataRef where)*



**brief** The bytes at `where`, to fill; empty outside what was appended. 

