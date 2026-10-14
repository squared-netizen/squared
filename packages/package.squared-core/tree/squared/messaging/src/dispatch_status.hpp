#pragma once

#include <cstdint>

namespace sq::messaging
{

/** @brief Outcome category of one dispatch operation. */
enum class DispatchStatus : std::uint8_t {
    Queued,
    InvalidTelegram,
    InvalidDelay,
    QueueFull,
    HandleExhausted
};

} // namespace sq::messaging
