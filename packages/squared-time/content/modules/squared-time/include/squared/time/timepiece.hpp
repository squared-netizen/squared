#pragma once

#include <chrono>
#include <cstdint>

namespace squared::time {

using Duration = std::chrono::nanoseconds;
using TimePoint = std::chrono::nanoseconds;

/**
 * @brief Read-only view of one application time domain.
 *
 * Reading a clock never advances it. The domain owner advances its Timepiece
 * once per frame or fixed simulation step.
 */
class Clock {
public:
    virtual ~Clock() = default;

    [[nodiscard]]
    virtual TimePoint now() const noexcept = 0;
};

/**
 * @brief Mutable, pausable, scaled application time domain.
 *
 * Time is stored as signed 64-bit nanoseconds. Negative deltas are ignored and
 * overflow saturates at the largest representable time point.
 */
class Timepiece : public Clock {
public:
    Timepiece() noexcept = default;

    [[nodiscard]] TimePoint now() const noexcept override;
    void advance(Duration delta) noexcept;

    void pause() noexcept;
    void resume() noexcept;
    [[nodiscard]] bool paused() const noexcept;

    /**
     * @brief Set a finite time scale in the inclusive range [0, 1024].
     * @return false when the requested scale is invalid.
     */
    [[nodiscard]] bool set_time_scale(double scale) noexcept;
    [[nodiscard]] double time_scale() const noexcept;

protected:
    void set_now(TimePoint value) noexcept;

private:
    TimePoint now_{Duration::zero()};
    long double fractional_nanoseconds_{0.0L};
    double time_scale_{1.0};
    bool paused_{false};
};

/**
 * @brief Explicitly controlled Timepiece for simulations, editors, and tests.
 */
class ManualTimepiece final : public Timepiece {
public:
    void reset(TimePoint value = Duration::zero()) noexcept;
};

}  // namespace squared::time
