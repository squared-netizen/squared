#pragma once

#include <cstdint>

namespace sq::messaging {

/** @brief Application-owned correlation tag echoed by queued receipts. */
using CorrelationId = std::uint64_t;

}  // namespace sq::messaging
