#pragma once

#include <cstdint>

namespace sq::messaging {

/** @brief Stable handle for one queued or delayed Telegram. */
struct DispatchHandle {
    /** @brief Non-zero monotonic token; zero means invalid/empty. */
    std::uint64_t value{0};

    /** @brief Return whether the handle references a pending delivery. */
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return value != 0;
    }
};

}  // namespace sq::messaging
