#include <squared/time/timepiece.hpp>

#include <chrono>

namespace sq::time
{

double Timepiece::tick( TimePoint now ) noexcept
{
    if( !started() || now < last_ ) {
        step_ = Duration::zero();
    } else {
        const Duration step = now - last_;
        step_ = step < longest_step ? step : longest_step;
    }
    last_ = now;
    ++ticks_;
    return seconds();
}

void Timepiece::start( TimePoint now ) noexcept
{
    last_ = now;
    step_ = Duration::zero();
    ticks_ = 0;
}

double Timepiece::seconds() const noexcept
{
    return std::chrono::duration<double>( step_ ).count();
}

} // namespace sq::time
