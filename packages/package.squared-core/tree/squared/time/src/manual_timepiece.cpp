#include <squared/time/manual_timepiece.hpp>

#include <squared/time/time_point.hpp>

namespace sq::time {

void ManualTimepiece::reset(TimePoint value) noexcept
{
    set_now(value);
}

} // namespace sq::time
