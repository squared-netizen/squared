#include <squared/time/timepiece.hpp>

#include <cmath>
#include <limits>

namespace squared::time {

TimePoint Timepiece::now() const noexcept
{
    return now_;
}

void Timepiece::advance(Duration delta) noexcept
{
    if (paused_ || delta.count() <= 0 || time_scale_ == 0.0) return;

    const long double scaled =
        static_cast<long double>(delta.count()) *
            static_cast<long double>(time_scale_) +
        fractional_nanoseconds_;
    const long double whole = std::floor(scaled);
    const auto maximum =
        std::numeric_limits<Duration::rep>::max();
    const long double remaining =
        static_cast<long double>(maximum - now_.count());

    if (whole >= remaining) {
        now_ = TimePoint(maximum);
        fractional_nanoseconds_ = 0.0L;
        return;
    }

    const auto increment = static_cast<Duration::rep>(whole);
    now_ += Duration(increment);
    fractional_nanoseconds_ = scaled - whole;
}

void Timepiece::pause() noexcept
{
    paused_ = true;
}

void Timepiece::resume() noexcept
{
    paused_ = false;
}

bool Timepiece::paused() const noexcept
{
    return paused_;
}

bool Timepiece::set_time_scale(double scale) noexcept
{
    if (!std::isfinite(scale) || scale < 0.0 || scale > 1024.0) {
        return false;
    }
    time_scale_ = scale;
    return true;
}

double Timepiece::time_scale() const noexcept
{
    return time_scale_;
}

void Timepiece::set_now(TimePoint value) noexcept
{
    now_ = value.count() < 0 ? Duration::zero() : value;
    fractional_nanoseconds_ = 0.0L;
}

void ManualTimepiece::reset(TimePoint value) noexcept
{
    set_now(value);
}

}  // namespace squared::time
