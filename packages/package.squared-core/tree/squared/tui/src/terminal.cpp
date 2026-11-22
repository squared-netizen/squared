module;

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

module sq.tui;

import std;

namespace sq::tui
{

namespace
{

/** The signals after which the terminal is put back before the process goes. */
constexpr std::array<int, 9> fatal_signals{ SIGSEGV, SIGABRT, SIGBUS,  SIGFPE, SIGILL,
                                            SIGTERM, SIGHUP,  SIGQUIT, SIGINT };

/**
 * What putting the terminal back needs, where a signal handler can reach
 * it: the mode it had and the bytes that turn off what was turned on. One
 * Terminal at a time, so one of these.
 */
struct Saved
{
        std::atomic<bool> active{ false };
        int input = 0;
        int output = 1;
        termios mode{};
        std::array<char, 96> restore{};
        std::size_t restore_size = 0;
        std::array<int, 2> wake{ -1, -1 };
        std::array<struct sigaction, fatal_signals.size()> previous{};
        struct sigaction previous_winch{};
        std::terminate_handler previous_terminate = nullptr;
};

Saved saved;
std::atomic<bool> resized{ false };

/** Async-signal-safe: a write and a tcsetattr. */
void restore_now() noexcept
{
    if( !saved.active.exchange( false ) ) {
        return;
    }
    std::size_t sent = 0;
    while( sent < saved.restore_size ) {
        const ssize_t n =
            ::write( saved.output, saved.restore.data() + sent, saved.restore_size - sent );
        if( n < 0 && errno == EINTR ) {
            continue;
        }
        if( n <= 0 ) {
            break;
        }
        sent += static_cast<std::size_t>( n );
    }
    ::tcsetattr( saved.input, TCSAFLUSH, &saved.mode );
}

extern "C" void on_fatal( int signal )
{
    restore_now();
    for( std::size_t i = 0; i < fatal_signals.size(); ++i ) {
        if( fatal_signals[i] == signal ) {
            ::sigaction( signal, &saved.previous[i], nullptr );
        }
    }
    ::raise( signal );
}

extern "C" void on_winch( int /*signal*/ )
{
    const int was = errno;
    resized.store( true );
    if( saved.wake[1] >= 0 ) {
        const char byte = 1;
        [[maybe_unused]] const ssize_t n = ::write( saved.wake[1], &byte, 1 );
    }
    errno = was;
}

extern "C" void on_exit_restore()
{
    restore_now();
}

void on_terminate()
{
    restore_now();
    if( saved.previous_terminate != nullptr ) {
        saved.previous_terminate();
    }
    std::abort();
}

void append( std::string_view text ) noexcept
{
    const std::size_t n = std::min( text.size(), saved.restore.size() - saved.restore_size );
    std::copy_n( text.data(), n, saved.restore.data() + saved.restore_size );
    saved.restore_size += n;
}

} // namespace

std::string_view describe( TerminalError error ) noexcept
{
    switch( error ) {
        case TerminalError::not_a_terminal:
            return "not a terminal: the program's input is a pipe or a file";
        case TerminalError::no_mode:
            return "the terminal's mode could not be read or set";
        case TerminalError::already_open:
            return "a Terminal is open already in this program";
    }
    return "unknown";
}

std::expected<Terminal, TerminalError> Terminal::open( const TerminalOptions &options )
{
    if( saved.active.load() ) {
        return std::unexpected( TerminalError::already_open );
    }
    if( ::isatty( options.input ) == 0 ) {
        return std::unexpected( TerminalError::not_a_terminal );
    }
    termios mode{};
    if( ::tcgetattr( options.input, &mode ) != 0 ) {
        return std::unexpected( TerminalError::no_mode );
    }
    termios raw = mode;
    raw.c_iflag &=
        ~static_cast<tcflag_t>( IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON );
    raw.c_lflag &= ~static_cast<tcflag_t>( ECHO | ECHONL | ICANON | ISIG | IEXTEN );
    raw.c_cflag &= ~static_cast<tcflag_t>( CSIZE | PARENB );
    raw.c_cflag |= CS8;
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if( ::tcsetattr( options.input, TCSAFLUSH, &raw ) != 0 ) {
        return std::unexpected( TerminalError::no_mode );
    }

    // What to put back, in the reverse order of turning it on.
    saved.input = options.input;
    saved.output = options.output;
    saved.mode = mode;
    saved.restore_size = 0;
    std::string enter;
    if( options.alternate_screen ) {
        enter += "\x1b[?1049h";
    }
    enter += "\x1b[?25l";
    if( options.paste ) {
        enter += "\x1b[?2004h";
    }
    if( options.mouse ) {
        enter += "\x1b[?1000h\x1b[?1002h\x1b[?1006h";
    }
    if( options.focus ) {
        enter += "\x1b[?1004h";
    }
    if( options.focus ) {
        append( "\x1b[?1004l" );
    }
    if( options.mouse ) {
        append( "\x1b[?1006l\x1b[?1002l\x1b[?1000l" );
    }
    if( options.paste ) {
        append( "\x1b[?2004l" );
    }
    append( "\x1b[0m\x1b[?25h" );
    if( options.alternate_screen ) {
        append( "\x1b[?1049l" );
    }

    if( saved.wake[0] < 0 && ::pipe2( saved.wake.data(), O_CLOEXEC | O_NONBLOCK ) != 0 ) {
        saved.wake = { -1, -1 };
    }
    struct sigaction fatal{};
    fatal.sa_handler = on_fatal;
    ::sigemptyset( &fatal.sa_mask );
    for( std::size_t i = 0; i < fatal_signals.size(); ++i ) {
        ::sigaction( fatal_signals[i], &fatal, &saved.previous[i] );
    }
    struct sigaction winch{};
    winch.sa_handler = on_winch;
    winch.sa_flags = SA_RESTART;
    ::sigemptyset( &winch.sa_mask );
    ::sigaction( SIGWINCH, &winch, &saved.previous_winch );
    static const bool at_exit = std::atexit( on_exit_restore ) == 0;
    ( void )at_exit;
    saved.previous_terminate = std::set_terminate( on_terminate );
    saved.active.store( true );

    Terminal terminal;
    terminal.open_ = true;
    terminal.input_ = options.input;
    terminal.output_ = options.output;
    terminal.read_size();
    terminal.write( enter );
    return terminal;
}

Terminal::Terminal( Terminal &&other ) noexcept
    : open_( std::exchange( other.open_, false ) )
    , input_( other.input_ )
    , output_( other.output_ )
    , columns_( other.columns_ )
    , rows_( other.rows_ )
    , decoder_( std::move( other.decoder_ ) )
    , queue_( std::move( other.queue_ ) )
    , queue_front_( other.queue_front_ )
    , renderer_( std::move( other.renderer_ ) )
{}

Terminal::~Terminal()
{
    if( !open_ ) {
        return;
    }
    restore_now();
    for( std::size_t i = 0; i < fatal_signals.size(); ++i ) {
        ::sigaction( fatal_signals[i], &saved.previous[i], nullptr );
    }
    ::sigaction( SIGWINCH, &saved.previous_winch, nullptr );
    std::set_terminate( saved.previous_terminate );
}

bool Terminal::read_size() noexcept
{
    winsize size{};
    if( ::ioctl( output_, TIOCGWINSZ, &size ) != 0 || size.ws_col == 0 || size.ws_row == 0 ) {
        return false;
    }
    columns_ = size.ws_col;
    rows_ = size.ws_row;
    return true;
}

bool Terminal::write( std::string_view bytes ) noexcept
{
    std::size_t sent = 0;
    while( sent < bytes.size() ) {
        const ssize_t n = ::write( output_, bytes.data() + sent, bytes.size() - sent );
        if( n < 0 && errno == EINTR ) {
            continue;
        }
        if( n <= 0 ) {
            return false;
        }
        sent += static_cast<std::size_t>( n );
    }
    return true;
}

bool Terminal::show( const Screen &screen )
{
    const std::string_view bytes = renderer_.render( screen );
    return bytes.empty() || write( bytes );
}

std::optional<Event> Terminal::next( std::chrono::milliseconds timeout )
{
    using clock = std::chrono::steady_clock;
    // How long an escape may wait for the rest of a sequence before it is
    // the Escape key: long enough for a slow connection, short enough not
    // to be felt.
    constexpr std::chrono::milliseconds escape_wait{ 25 };
    const bool forever = timeout.count() < 0;
    const clock::time_point deadline =
        clock::now() + ( forever ? std::chrono::milliseconds{ 0 } : timeout );
    std::array<char, 4096> buffer{};
    for( ;; ) {
        if( queue_front_ < queue_.size() ) {
            Event event = std::move( queue_[queue_front_++] );
            if( queue_front_ == queue_.size() ) {
                queue_.clear();
                queue_front_ = 0;
            }
            return event;
        }
        if( resized.exchange( false ) && read_size() ) {
            renderer_.invalidate();
            return Resize{ columns_, rows_ };
        }
        int wait = -1;
        if( !forever ) {
            const auto left =
                std::chrono::duration_cast<std::chrono::milliseconds>( deadline - clock::now() );
            wait = static_cast<int>( std::max<std::int64_t>( left.count(), 0 ) );
        }
        const bool escape_pending = decoder_.waiting();
        if( escape_pending && ( wait < 0 || wait > escape_wait.count() ) ) {
            wait = static_cast<int>( escape_wait.count() );
        }
        std::array<pollfd, 2> fds{ { { input_, POLLIN, 0 }, { saved.wake[0], POLLIN, 0 } } };
        const int ready = ::poll( fds.data(), saved.wake[0] >= 0 ? 2 : 1, wait );
        if( ready < 0 && errno == EINTR ) {
            continue;
        }
        if( ready <= 0 ) {
            if( escape_pending ) {
                decoder_.flush( queue_ );
                continue;
            }
            if( !forever && clock::now() >= deadline ) {
                return std::nullopt;
            }
            continue;
        }
        if( saved.wake[0] >= 0 && ( fds[1].revents & POLLIN ) != 0 ) {
            while( ::read( saved.wake[0], buffer.data(), buffer.size() ) > 0 ) {
            }
        }
        if( ( fds[0].revents & ( POLLIN | POLLHUP ) ) != 0 ) {
            const ssize_t n = ::read( input_, buffer.data(), buffer.size() );
            if( n == 0 || ( n < 0 && errno != EINTR && errno != EAGAIN ) ) {
                return std::nullopt; // the terminal has gone
            }
            if( n > 0 ) {
                decoder_.feed( std::string_view{ buffer.data(), static_cast<std::size_t>( n ) },
                               queue_ );
            }
        }
    }
}

} // namespace sq::tui
