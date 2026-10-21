#include <squared/messaging/src/wake.hpp>

#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>

namespace sq::messaging
{

Wake::Wake() noexcept
    : fd_( ::eventfd( 0, EFD_NONBLOCK | EFD_CLOEXEC ) )
{}

Wake::~Wake()
{
    if( fd_ >= 0 ) {
        ::close( fd_ );
    }
}

void Wake::signal() const noexcept
{
    if( fd_ < 0 ) {
        return;
    }
    const std::uint64_t one = 1;
    // A full counter (2^64 - 2 signals unread) is still a signal: EAGAIN is fine.
    while( ::write( fd_, &one, sizeof( one ) ) < 0 && errno == EINTR ) {
    }
}

void Wake::clear() const noexcept
{
    if( fd_ < 0 ) {
        return;
    }
    std::uint64_t count = 0;
    while( ::read( fd_, &count, sizeof( count ) ) < 0 && errno == EINTR ) {
    }
}

void Wake::wait( int timeout_ms ) const noexcept
{
    pollfd watched{ fd_, POLLIN, 0 };
    // With no descriptor (it could not be made), poll on nothing: a timed sleep.
    while( ::poll( &watched, fd_ >= 0 ? 1 : 0, timeout_ms ) < 0 && errno == EINTR ) {
    }
}

} // namespace sq::messaging
