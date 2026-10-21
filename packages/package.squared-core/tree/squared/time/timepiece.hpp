#pragma once

#include <squared/time/duration.hpp>
#include <squared/time/time_point.hpp>

#include <chrono>
#include <cstdint>

namespace sq::time
{

/**
 * @brief The time between ticks, as a value: the last time it was given,
 * the step since the one before (clamped), and how many ticks there have
 * been. 24 bytes, no base class, nothing virtual, no clock of its own: the
 * loop reads the clock once (`now()`) and passes the time in.
 *
 * One per loop: the UI's frames, the model's ticks. Not shared between
 * threads.
 */
class Timepiece final
{
    public:
        /** @brief The longest step a tick reports: after a stall, a debugger or a suspended app. */
        static constexpr Duration longest_step = std::chrono::milliseconds( 250 );

        /**
         * @brief A tick at `now`: the seconds since the last tick (or since
         * `start`), at most a quarter second; 0 for the first tick when
         * there was no `start`, and 0 if `now` is earlier than the last time.
         */
        double tick( TimePoint now ) noexcept;

        /** @brief Count from `now`: the next tick measures from here. Clears the step and the count. */
        void start( TimePoint now ) noexcept;

        /** @brief The step the last tick reported, in nanoseconds. */
        [[nodiscard]] Duration step() const noexcept { return step_; }

        /** @brief The step the last tick reported, in seconds. */
        [[nodiscard]] double seconds() const noexcept;

        /** @brief The time the last tick (or `start`) was given. */
        [[nodiscard]] TimePoint last() const noexcept { return last_; }

        /** @brief Ticks since `start`, or since it was made. */
        [[nodiscard]] std::uint64_t ticks() const noexcept { return ticks_; }

        /** @brief Whether it has a time to measure from: a tick or `start` happened. */
        [[nodiscard]] bool started() const noexcept { return last_ != never; }

    private:
        static constexpr TimePoint never = TimePoint::min();

        TimePoint last_ = never;
        Duration step_ = Duration::zero();
        std::uint64_t ticks_ = 0;
};

static_assert( sizeof( Timepiece ) == 24, "Timepiece is 24 bytes: the app model's budget" );

} // namespace sq::time
