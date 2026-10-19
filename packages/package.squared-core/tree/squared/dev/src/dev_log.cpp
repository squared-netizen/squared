#include <squared/dev/crash_trace.hpp>
#include <squared/dev/dev_log.hpp>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace sq::dev
{
namespace
{

// Resident cost, all static storage and all present only in a build that links
// this module: the alternate stack dominates.
constexpr std::size_t kLineMax = 1000;
constexpr std::size_t kAltStackBytes = std::size_t{ 16 } * 1024U;
constexpr int kSignals[] = { SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL, SIGTRAP };
constexpr std::size_t kSignalCount = sizeof( kSignals ) / sizeof( kSignals[0] );

std::atomic<int> g_fd{ -1 };
sockaddr_in g_address{};
alignas( 16 ) char g_alt_stack[kAltStackBytes];
struct sigaction g_previous[kSignalCount];
bool g_handlers_installed = false;

// The listener, its context, and the one thread allowed to call it. The
// listener and context are only read on that thread; other threads only read
// the flag and the owner, so neither needs a lock.
std::atomic<bool> g_listening{ false };
std::atomic<pthread_t> g_listener_thread{};
LogListener g_listener = nullptr;
void *g_listener_context = nullptr;

void send_bytes( const char *data, std::size_t length ) noexcept
{
    const int fd = g_fd.load( std::memory_order_relaxed );
    if( fd < 0 ) {
        return;
    }
    // A full socket buffer or an absent listener drops the datagram; that is
    // the right outcome for a diagnostic.
    ( void )::sendto( fd, data, length, 0, reinterpret_cast<const sockaddr *>( &g_address ),
                      sizeof( g_address ) );
}

// One datagram a line of the crash trace (crash_trace.hpp).
void send_crash_line( void *, const char *text, std::size_t length ) noexcept
{
    send_bytes( text, length < kLineMax ? length : kLineMax );
}

void on_fatal_signal( int sig, siginfo_t *info, void *ucontext )
{
    write_crash_trace( sig, info, ucontext, send_crash_line, nullptr );

    // Hand the signal back to the system so it still produces its own report
    // and the process still dies the way it would have.
    ::signal( sig, SIG_DFL );
    ::raise( sig );
}

void install_handlers() noexcept
{
    stack_t stack{};
    stack.ss_sp = g_alt_stack;
    stack.ss_size = sizeof( g_alt_stack );
    stack.ss_flags = 0;
    ( void )::sigaltstack( &stack, nullptr );

    struct sigaction action = {};
    action.sa_sigaction = on_fatal_signal;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset( &action.sa_mask );
    for( std::size_t i = 0; i < kSignalCount; ++i ) {
        ( void )::sigaction( kSignals[i], &action, &g_previous[i] );
    }
    g_handlers_installed = true;
}

void restore_handlers() noexcept
{
    if( !g_handlers_installed ) {
        return;
    }
    for( std::size_t i = 0; i < kSignalCount; ++i ) {
        ( void )::sigaction( kSignals[i], &g_previous[i], nullptr );
    }
    g_handlers_installed = false;
}

} // namespace

LogSinkStatus open_log_sink( const LogSinkOptions &options ) noexcept
{
    if( g_fd.load() >= 0 ) {
        return LogSinkStatus::AlreadyOpen;
    }

    const int fd = ::socket( AF_INET, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0 );
    if( fd < 0 ) {
        return LogSinkStatus::SocketFailed;
    }

    std::memset( &g_address, 0, sizeof( g_address ) );
    g_address.sin_family = AF_INET;
    g_address.sin_port = htons( options.port );
    g_address.sin_addr.s_addr = htonl( INADDR_LOOPBACK );

    g_fd.store( fd );
    if( options.catch_crashes ) {
        install_handlers();
    }
    return LogSinkStatus::Opened;
}

void log_line( std::string_view line, LogLevel level ) noexcept
{
    if( g_fd.load( std::memory_order_relaxed ) >= 0 ) {
        send_bytes( line.data(), line.size() < kLineMax ? line.size() : kLineMax );
    }
    if( g_listening.load( std::memory_order_acquire ) &&
        pthread_equal( g_listener_thread.load( std::memory_order_relaxed ), pthread_self() ) !=
            0 ) {
        g_listener( g_listener_context, level, line );
    }
}

void set_log_listener( LogListener listener, void *context ) noexcept
{
    g_listening.store( false, std::memory_order_release );
    g_listener = listener;
    g_listener_context = context;
    if( listener != nullptr ) {
        g_listener_thread.store( pthread_self(), std::memory_order_relaxed );
        g_listening.store( true, std::memory_order_release );
    }
}

void close_log_sink() noexcept
{
    restore_handlers();
    const int fd = g_fd.exchange( -1 );
    if( fd >= 0 ) {
        ::close( fd );
    }
}

} // namespace sq::dev
