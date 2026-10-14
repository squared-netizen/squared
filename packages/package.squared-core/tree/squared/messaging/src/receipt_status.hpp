#pragma once

#include <cstdint>

namespace sq::messaging
{

/** @brief Outcome of delivering one Telegram to its subscribers. */
enum class ReceiptStatus : std::uint8_t { Handled, Unhandled, ReceiverUnavailable, Cancelled };

} // namespace sq::messaging
