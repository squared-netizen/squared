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

/**
 * @brief Mutable, pausable, scaled application time domain.
 *
 * Time is stored as signed 64-bit nanoseconds. Negative deltas are ignored and
 * overflow saturates at the largest representable time point. This class is
 * not thread-safe; one thread must own the domain.
 */
class Timepiece : public Clock {
public:
    Timepiece() noexcept = default;

    /**
     * @brief Read the current domain time.
     * @return Nanoseconds since the domain's epoch.
     */
    [[nodiscard]] TimePoint now() const noexcept override;

    /**
     * @brief Advance the domain by one frame or fixed step.
     * @param delta Desired elapsed time in nanoseconds. Negative values are
     * ignored. Scaled by the current time-scale value.
     */
    void advance(Duration delta) noexcept;

    /** @brief Freeze the domain so time no longer advances. */
    void pause() noexcept;

    /** @brief Resume a paused domain. */
    void resume() noexcept;

    /**
     * @brief Report the paused state.
     * @return true while the domain is frozen.
     */
    [[nodiscard]] bool paused() const noexcept;

    /**
     * @brief Set a finite time scale in the inclusive range [0, 1024].
     * @param scale Multiplier applied to every advance delta.
     * @return false when the requested scale is invalid; true otherwise.
     */
    [[nodiscard]] bool set_time_scale(double scale) noexcept;

    /**
     * @brief Read the current time-scale multiplier.
     * @return Active multiplier in the inclusive range [0, 1024].
     */
    [[nodiscard]] double time_scale() const noexcept;

protected:
    /**
     * @brief Overwrite the current time for explicit controls and tests.
     * @param value Replacement time point in nanoseconds.
     */
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
    /**
     * @brief Return the domain to a fixed time point.
     * @param value Replacement time point in nanoseconds.
     */
    void reset(TimePoint value = Duration::zero()) noexcept;
};

}  // namespace squared::time
