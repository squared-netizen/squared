#pragma once

#include <squared/time/time_point.hpp>

namespace sq::time
{

/**
 * @brief The monotonic clock, now: nanoseconds from an arbitrary start,
 * never going back, and not counting the time the device spent in deep
 * sleep. One call into the vDSO, with no system call. Read it once a frame or
 * a tick and pass the value in; tests pass their own times instead.
 */
[[nodiscard]] TimePoint now() noexcept;

/**
 * @brief Nanoseconds since the device started, deep sleep included: how
 * long an app was away, for `foreground(elapsed)`.
 */
[[nodiscard]] TimePoint since_boot() noexcept;

} // namespace sq::time
