# struct DataRef

*Defined at [messaging/src/data_ref.hpp:13](../../../../messaging/src/data_ref.hpp#L13)*



**brief** Where a telegram's text or uid list is: `length` bytes from `offset` in the data area that travels with it (the Telegraph's data ring, or a `TelegramData`). A uid list is 4 bytes per uid.



## Members

public std::uint32_t offset

public std::uint32_t length



