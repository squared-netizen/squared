#pragma once

#include <cstdint>

namespace sq::messaging
{

/**
 * @brief Where a telegram's text or uid list is: `length` bytes from `offset`
 * in the data area that travels with it (the Telegraph's data ring, or a
 * `TelegramData`). A uid list is 4 bytes per uid.
 */
struct DataRef
{
        std::uint32_t offset = 0;
        std::uint32_t length = 0;

        friend constexpr bool operator==( DataRef, DataRef ) noexcept = default;
};

} // namespace sq::messaging
