#include <squared/messaging/model_runner.hpp>

#include <pthread.h>

#include <chrono>
#include <limits>

namespace sq::messaging::detail
{

void name_this_thread( const char *name ) noexcept
{
    // Both glibc and bionic have it; a name over 15 characters is refused, not cut.
    static_cast<void>( ::pthread_setname_np( ::pthread_self(), name ) );
}

int milliseconds_until( time::TimePoint at, time::TimePoint now ) noexcept
{
    if( at <= now ) {
        return 0;
    }
    const auto milliseconds = std::chrono::ceil<std::chrono::milliseconds>( at - now ).count();
    constexpr auto longest = std::numeric_limits<int>::max();
    return milliseconds > longest ? longest : static_cast<int>( milliseconds );
}

int TickClock::until_due( std::uint32_t interval_ms, time::TimePoint now ) noexcept
{
    if( interval_ms == 0 ) {
        interval_ms_ = 0;
        return -1;
    }
    if( interval_ms != interval_ms_ ) {
        // A new interval (or the first): the first tick is a whole one away.
        interval_ms_ = interval_ms;
        time_.start( now );
        next_ = now + std::chrono::milliseconds( interval_ms );
    }
    return milliseconds_until( next_, now );
}

double TickClock::take( time::TimePoint now ) noexcept
{
    // The Timepiece clamps: a model that stalled (a debugger, a suspended
    // app) gets no huge step.
    const double seconds = time_.tick( now );
    const auto interval = std::chrono::milliseconds( interval_ms_ );
    next_ += interval;
    if( next_ <= now ) {
        next_ = now + interval; // behind: start the cadence again, no burst to catch up
    }
    return seconds;
}

} // namespace sq::messaging::detail
