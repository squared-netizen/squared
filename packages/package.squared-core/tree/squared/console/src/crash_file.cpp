// The Console's crash file: a report written from a signal handler, read back
// on the next launch. Every function the handler calls is a write() of a
// fixed buffer, except the trace's dladdr and unwinder (crash_trace.hpp).

#include <squared/console/crash_file.hpp>
#include <squared/console/line_reader.hpp>
#include <squared/dev/crash_trace.hpp>

#include <fcntl.h>
#include <unistd.h>

#include <array>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>

namespace sq::console
{

namespace
{

constexpr std::array<int, 6> fatal_signals = { SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL, SIGTRAP };

// Static: the handler reads them, and may not allocate or look anything up.
std::array<char, 512> g_path{};
const Console *g_console = nullptr;
std::size_t g_log_lines = 0;
std::array<struct sigaction, fatal_signals.size()> g_previous{};
bool g_installed = false;
// For this thread's stack overflows, when nothing else gave it an alternate stack.
alignas( 16 ) std::array<char, std::size_t{ 16 } * 1024> g_alt_stack{};

void write_all( int fd, const char *data, std::size_t length ) noexcept
{
    while( length > 0 ) {
        const ssize_t wrote = ::write( fd, data, length );
        if( wrote <= 0 ) {
            return;
        }
        data += wrote;
        length -= static_cast<std::size_t>( wrote );
    }
}

void write_line( void *context, const char *text, std::size_t length ) noexcept
{
    const int fd = *static_cast<const int *>( context );
    write_all( fd, text, length );
    write_all( fd, "\n", 1 );
}

void on_fatal_signal( int signal, siginfo_t *info, void *ucontext )
{
    int fd = ::open( g_path.data(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600 );
    if( fd >= 0 ) {
        sq::dev::write_crash_trace( signal, info, ucontext, write_line, &fd );
        if( g_console != nullptr && g_log_lines > 0 ) {
            // Last, so a scrollback caught mid-write can cost only this part.
            sq::dev::CrashLine head;
            head.add( "log: the last " );
            head.add_decimal( g_log_lines );
            head.add( " lines" );
            write_line( &fd, head.text, head.length );
            const Scrollback &lines = g_console->lines();
            for( auto it = lines.last( g_log_lines ); it != lines.end(); ++it ) {
                const std::string_view text = ( *it ).text;
                write_all( fd, "  ", 2 );
                write_line( &fd, text.data(), text.size() );
            }
        }
        ::close( fd );
    }

    // Hand the signal on: put back what handled it before, then let it happen
    // again. A fault from an instruction recurs by itself when this returns,
    // with its own address; a signal that was sent (abort, raise) is sent again.
    for( std::size_t i = 0; i < fatal_signals.size(); ++i ) {
        if( fatal_signals[i] == signal ) {
            ::sigaction( signal, &g_previous[i], nullptr );
        }
    }
    if( info == nullptr || info->si_code <= 0 ) {
        ::raise( signal );
    }
}

void print_file( Console &console, int fd ) noexcept
{
    std::array<char, 1024> buffer{};
    for_each_line( fd, buffer,
                   [&]( std::string_view line ) { console.print( line, Level::error ); } );
}

void crash( Console &console, const Args &args, void *context ) noexcept
{
    // On purpose, to see the report work: a write through null, or abort().
    if( args.text( 1 ) == "segv" ) {
        console.print( "crash: writing through a null pointer", Level::warning );
        volatile int *nowhere = nullptr;
        *nowhere = 1; // NOLINT(clang-analyzer-core.NullDereference): asked for
        return;
    }
    if( args.text( 1 ) == "abort" ) {
        console.print( "crash: abort()", Level::warning );
        std::abort();
    }
    const auto *path = static_cast<const char *>( context );
    const int fd = ::open( path, O_RDONLY | O_CLOEXEC );
    if( fd < 0 ) {
        console.print( "crash: no report kept; the last run did not crash", Level::info );
        return;
    }
    print_file( console, fd );
    ::close( fd );
}

} // namespace

bool install_crash_file( const char *path, const Console *console, std::size_t log_lines ) noexcept
{
    const std::size_t length = std::strlen( path );
    if( length == 0 || length >= g_path.size() ) {
        return false;
    }
    remove_crash_file();
    std::memcpy( g_path.data(), path, length + 1 );
    g_console = console;
    g_log_lines = log_lines;

    stack_t current{};
    if( ::sigaltstack( nullptr, &current ) == 0 && ( current.ss_flags & SS_DISABLE ) != 0 ) {
        stack_t stack{};
        stack.ss_sp = g_alt_stack.data();
        stack.ss_size = g_alt_stack.size();
        ::sigaltstack( &stack, nullptr );
    }

    struct sigaction action = {};
    action.sa_sigaction = on_fatal_signal;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset( &action.sa_mask );
    bool all = true;
    for( std::size_t i = 0; i < fatal_signals.size(); ++i ) {
        all = ::sigaction( fatal_signals[i], &action, &g_previous[i] ) == 0 && all;
    }
    g_installed = true;
    return all;
}

void remove_crash_file() noexcept
{
    if( !g_installed ) {
        return;
    }
    for( std::size_t i = 0; i < fatal_signals.size(); ++i ) {
        ::sigaction( fatal_signals[i], &g_previous[i], nullptr );
    }
    g_installed = false;
    g_console = nullptr;
}

bool show_crash_file( Console &console, const char *path, const char *previous_path ) noexcept
{
    const int fd = ::open( path, O_RDONLY | O_CLOEXEC );
    if( fd < 0 ) {
        return false;
    }
    console.print( "the last run crashed; its report follows, and `crash` shows it again",
                   Level::error );
    print_file( console, fd );
    ::close( fd );
    if( std::rename( path, previous_path ) != 0 ) {
        ::unlink( path );
    }
    return true;
}

Commands::AddResult add_crash_command( Console &console, const char *previous_path ) noexcept
{
    // The table keeps the context as void*; the command only reads through it.
    return console.add( "crash", "crash [segv | abort]: the last crash report again, or crash now",
                        crash, const_cast<char *>( previous_path ) );
}

} // namespace sq::console
