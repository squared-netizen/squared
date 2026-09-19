#pragma once

#include <squared/time/time_point.hpp>

namespace sq::time {

/**
 * @brief Read-only view of one application time domain.
 *
 * Reading a clock never advances it. The domain owner advances its Timepiece
 * once per frame or fixed simulation step. Borrowers must not retain the
 * clock past the Timepiece's lifetime.
 */
class Clock {
public:
    virtual ~Clock() = default;

    /**
     * @brief Return the current time in this domain.
     * @return Nanoseconds since the domain's epoch, monotonically
     * non-decreasing while the domain is not reset.
     */
    [[nodiscard]]
    virtual TimePoint now() const noexcept = 0;
};

} // namespace sq::time
