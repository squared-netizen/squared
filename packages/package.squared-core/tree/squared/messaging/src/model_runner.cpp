#include <squared/messaging/model_runner.hpp>

#include <pthread.h>

namespace sq::messaging::detail
{

void name_this_thread( const char *name ) noexcept
{
    // Both glibc and bionic have it; a name over 15 characters is refused, not cut.
    static_cast<void>( ::pthread_setname_np( ::pthread_self(), name ) );
}

int TickClock::until_due( std::uint32_t interval_ms ) noexcept
{
    if( interval_ms == 0 ) {
        interval_ms_ = 0;
        return -1;
    }
    const Clock::time_point now = Clock::now();
    if( interval_ms != interval_ms_ ) {
        // A new interval (or the first): the first tick is a whole one away.
        interval_ms_ = interval_ms;
        last_ = now;
        next_ = now + std::chrono::milliseconds( interval_ms );
    }
    if( now >= next_ ) {
        return 0;
    }
    return static_cast<int>( std::chrono::ceil<std::chrono::milliseconds>( next_ - now ).count() );
}

double TickClock::take() noexcept
{
    // A model that stalled (a debugger, a suspended app) gets no huge step.
    constexpr double longest = 0.25;
    const Clock::time_point now = Clock::now();
    const double seconds = std::chrono::duration<double>( now - last_ ).count();
    last_ = now;
    const auto interval = std::chrono::milliseconds( interval_ms_ );
    next_ += interval;
    if( next_ <= now ) {
        next_ = now + interval; // behind: start the cadence again, no burst to catch up
    }
    return seconds < longest ? seconds : longest;
}

} // namespace sq::messaging::detail
