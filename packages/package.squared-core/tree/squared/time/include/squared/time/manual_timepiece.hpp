#pragma once

#include <squared/time/duration.hpp>
#include <squared/time/time_point.hpp>
#include <squared/time/timepiece.hpp>

namespace sq::time {

/**
 * @brief Explicitly controlled Timepiece for simulations, editors, and tests.
 */
class ManualTimepiece final : public Timepiece {
public:
    /**
     * @brief Return the domain to a fixed time point.
     * @param value Replacement time point in nanoseconds.
     */
    void reset(TimePoint value = Duration::zero()) noexcept;
};

} // namespace sq::time
