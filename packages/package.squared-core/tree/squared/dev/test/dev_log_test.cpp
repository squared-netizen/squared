#include <squared/dev/dev_log.hpp>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cassert>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <string>
#include <vector>

namespace
{

// A listener on an OS-chosen loopback port, standing in for `nc -klu`.
struct Listener
{
        int fd = -1;
        std::uint16_t port = 0;

        Listener()
        {
            fd = ::socket( AF_INET, SOCK_DGRAM, 0 );
            sockaddr_in a{};
            a.sin_family = AF_INET;
            a.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
            a.sin_port = 0;
            assert( ::bind( fd, reinterpret_cast<sockaddr *>( &a ), sizeof( a ) ) == 0 );
            socklen_t len = sizeof( a );
            assert( ::getsockname( fd, reinterpret_cast<sockaddr *>( &a ), &len ) == 0 );
            port = ntohs( a.sin_port );
            timeval timeout{ 2, 0 };
            ::setsockopt( fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof( timeout ) );
        }
        ~Listener() { ::close( fd ); }

        // Every datagram that arrives before the timeout, or until END.
        std::vector<std::string> collect( bool stop_at_end ) const
        {
            std::vector<std::string> lines;
            char buffer[1100];
            while( true ) {
                const ssize_t n = ::recv( fd, buffer, sizeof( buffer ), 0 );
                if( n < 0 ) {
                    break;
                }
                lines.emplace_back( buffer, static_cast<std::size_t>( n ) );
                if( stop_at_end && lines.back() == "END" ) {
                    break;
                }
                if( !stop_at_end ) {
                    break;
                }
            }
            return lines;
        }
};

} // namespace

int main()
{
    using namespace sq::dev;

    // Closed sink: sending is a no-op, not a crash.
    log_line( "nobody is listening" );

    Listener listener;

    // A line arrives intact, and a second open is refused.
    assert( open_log_sink( { .port = listener.port, .catch_crashes = false } ) ==
            LogSinkStatus::Opened );
    assert( open_log_sink( { .port = listener.port } ) == LogSinkStatus::AlreadyOpen );
    log_line( "hello from the device" );
    auto got = listener.collect( false );
    assert( got.size() == 1 && got[0] == "hello from the device" );

    // Long lines are cut, not dropped.
    log_line( std::string( 5000, 'x' ) );
    got = listener.collect( false );
    assert( got.size() == 1 && got[0].size() == 1000 );
    close_log_sink();

    // After close, nothing is sent.
    log_line( "dropped" );
    {
        timeval quick{ 0, 200000 };
        ::setsockopt( listener.fd, SOL_SOCKET, SO_RCVTIMEO, &quick, sizeof( quick ) );
        char b[16];
        assert( ::recv( listener.fd, b, sizeof( b ), 0 ) < 0 );
        timeval timeout{ 2, 0 };
        ::setsockopt( listener.fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof( timeout ) );
    }

    // A real crash in a child: the report arrives, and the child still dies
    // of the signal, so the system's own report is not suppressed.
    const pid_t child = ::fork();
    assert( child >= 0 );
    if( child == 0 ) {
        if( open_log_sink( { .port = listener.port } ) != LogSinkStatus::Opened ) {
            _exit( 90 );
        }
        log_line( "about to crash" );
        volatile int *null_pointer = nullptr;
        *null_pointer = 1;
        _exit( 91 );
    }
    const auto report = listener.collect( true );
    int status = 0;
    ::waitpid( child, &status, 0 );

    assert( WIFSIGNALED( status ) && WTERMSIG( status ) == SIGSEGV );
    assert( report.size() >= 4 );
    assert( report[0] == "about to crash" );
    assert( report[1].rfind( "CRASH SIGSEGV (11)", 0 ) == 0 );
    assert( report[2].rfind( "  #0 pc ", 0 ) == 0 );
    // The unwinder got past the handler: a trace of frame 0 alone is the
    // failure a noexcept callback caused once (crash_trace.hpp).
    assert( report.size() >= 5 && report[3].rfind( "  #1 pc ", 0 ) == 0 );
    assert( report.back() == "END" );

    std::printf( "dev log: all assertions passed\n" );
    for( const auto &line : report ) {
        std::printf( "  %s\n", line.c_str() );
    }
    return 0;
}
