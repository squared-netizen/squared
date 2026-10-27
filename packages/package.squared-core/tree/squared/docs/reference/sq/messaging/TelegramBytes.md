# struct TelegramBytes

*Defined at [messaging/telegram.hpp:99](../../../../messaging/telegram.hpp#L99)*



**brief** A telegram's 32 bytes, both ways, for reading telegrams that were stored (`valid()` then says whether they are one).



## Functions

### of

*public static std::array<std::byte, 32> of(const Telegram & telegram)*

### from

*public static Telegram from(const std::array<std::byte, 32> & bytes)*



