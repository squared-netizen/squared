#pragma once

namespace sq::messaging {

/** @brief Outcome category of one dispatch operation. */
enum class DispatchStatus {
    Queued,
    InvalidTelegram,
    InvalidDelay,
    QueueFull,
    HandleExhausted
};

}  // namespace sq::messaging
