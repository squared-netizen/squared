#include <squared/time/now.hpp>

#include <time.h> // NOLINT(modernize-deprecated-headers): clock_gettime and the clock ids are POSIX

#include <chrono>

namespace sq::time
{

namespace
{

TimePoint read( clockid_t clock ) noexcept
{
    timespec value{};
    // Cannot fail for these clocks on Linux or Android: both ids exist and
    // the pointer is valid.
    static_cast<void>( ::clock_gettime( clock, &value ) );
    return std::chrono::seconds( value.tv_sec ) + std::chrono::nanoseconds( value.tv_nsec );
}

} // namespace

TimePoint now() noexcept
{
    return read( CLOCK_MONOTONIC );
}

TimePoint since_boot() noexcept
{
    return read( CLOCK_BOOTTIME );
}

} // namespace sq::time
