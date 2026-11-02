# struct TelegramBytes

*Defined at [messaging/telegram.cppm:94](../../../../messaging/telegram.cppm#L94)*



**brief** A telegram's 32 bytes, both ways, for reading telegrams that were stored (`valid()` then says whether they are one).



## Functions

### from

*public static Telegram from(const std::array<std::byte, 32> & bytes)*

### of

*public static std::array<std::byte, 32> of(const Telegram & telegram)*

