module;

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

module sq.term;

import std;

namespace sq::term
{

namespace
{

// The program's arguments and environment as C strings, and what the child
// says when it cannot run the program: all made before fork, since only
// async-signal-safe calls may follow it in a process with threads.
struct Exec
{
        std::string text;
        std::vector<char *> argv;
        std::vector<char *> envp;
        /** What execve runs; null when the program was not found on PATH. */
        const char *path = nullptr;
        const char *directory = nullptr;
        const char *failure = nullptr;
        std::size_t failure_size = 0;
};

// The PATH the program is looked up on: the given environment's, when one
// is given, else this process's; with neither, the system's directories.
std::string_view search_path( const PtyConfig &config ) noexcept
{
    if( !config.environment.empty() ) {
        for( const std::string_view variable : config.environment ) {
            if( variable.starts_with( "PATH=" ) ) {
                return variable.substr( 5 );
            }
        }
    } else if( const char *path = ::getenv( "PATH" ) ) {
        return path;
    }
    return "/system/bin:/usr/bin:/bin";
}

// The program's path: as given when it names a directory (`/bin/sh`,
// `./run.sh`), else the first executable of that name on PATH, as a shell
// finds it. Done here, before fork: execvp is not async-signal-safe on every
// C library. Empty when nothing on PATH has it.
std::string resolve( const PtyConfig &config )
{
    if( config.program.find( '/' ) != std::string_view::npos ) {
        return std::string{ config.program };
    }
    std::string candidate;
    std::string_view path = search_path( config );
    while( true ) {
        const std::size_t colon = path.find( ':' );
        const std::string_view directory = path.substr( 0, colon );
        // An empty entry is the current directory, as POSIX has it.
        candidate.assign( directory.empty() ? "." : directory );
        candidate.push_back( '/' );
        candidate.append( config.program );
        if( ::access( candidate.c_str(), X_OK ) == 0 ) {
            return candidate;
        }
        if( colon == std::string_view::npos ) {
            return {};
        }
        path.remove_prefix( colon + 1 );
    }
}

Exec prepare( const PtyConfig &config )
{
    Exec exec;
    const std::string path = resolve( config );
    std::vector<std::size_t> argv_at;
    std::vector<std::size_t> envp_at;
    const auto add = [&exec]( std::string_view text ) {
        const std::size_t at = exec.text.size();
        exec.text.append( text );
        exec.text.push_back( '\0' );
        return at;
    };
    argv_at.push_back( add( config.program ) );
    for( const std::string_view argument : config.arguments ) {
        argv_at.push_back( add( argument ) );
    }
    for( const std::string_view variable : config.environment ) {
        envp_at.push_back( add( variable ) );
    }
    const std::size_t path_at = path.empty() ? 0 : add( path );
    const std::size_t directory_at = config.directory.empty() ? 0 : add( config.directory );
    std::string failure = "pty: cannot run ";
    failure.append( config.program );
    failure.append( ": " );
    const std::size_t failure_at = add( failure );
    // The pointers last: the text no longer moves.
    char *base = exec.text.data();
    for( const std::size_t at : argv_at ) {
        exec.argv.push_back( base + at );
    }
    exec.argv.push_back( nullptr );
    if( !config.environment.empty() ) {
        for( const std::size_t at : envp_at ) {
            exec.envp.push_back( base + at );
        }
        exec.envp.push_back( nullptr );
    }
    exec.path = path.empty() ? nullptr : base + path_at;
    exec.directory = config.directory.empty() ? nullptr : base + directory_at;
    exec.failure = base + failure_at;
    exec.failure_size = failure.size();
    return exec;
}

void say( const char *text, std::size_t size ) noexcept
{
    while( size > 0 ) {
        const ssize_t written = ::write( STDERR_FILENO, text, size );
        if( written <= 0 ) {
            return;
        }
        text += written;
        size -= static_cast<std::size_t>( written );
    }
}

void say( std::string_view text ) noexcept
{
    say( text.data(), text.size() );
}

// Why execve failed, in words, chosen without allocating.
std::string_view reason( int error ) noexcept
{
    switch( error ) {
        case ENOENT:
            return "not found\n";
        case EACCES:
            return "permission denied (on Android 10 and later, a program in an app's own "
                   "folder runs only in an app that targets SDK 28)\n";
        case ENOEXEC:
            return "not a program this device runs\n";
        case ENOTDIR:
            return "a part of its path is not a directory\n";
        case E2BIG:
            return "its arguments and environment are too long\n";
        case ENOMEM:
            return "out of memory\n";
        default:
            return "it could not be started\n";
    }
}

// The child, between fork and exec: async-signal-safe calls only.
[[noreturn]] void run_child( int slave, int max_fd, const Exec &exec ) noexcept
{
    ::setsid();
    ::ioctl( slave, TIOCSCTTY, 0 );
    ::dup2( slave, STDIN_FILENO );
    ::dup2( slave, STDOUT_FILENO );
    ::dup2( slave, STDERR_FILENO );
    // Nothing of this process's own reaches the program: an app's binder,
    // its sockets, its files.
    for( int fd = 3; fd < max_fd; ++fd ) {
        ::close( fd );
    }
    struct sigaction initial{};
    initial.sa_handler = SIG_DFL;
    ::sigemptyset( &initial.sa_mask );
    for( int signal = 1; signal < NSIG; ++signal ) {
        ::sigaction( signal, &initial, nullptr );
    }
    sigset_t none{};
    ::sigemptyset( &none );
    ::sigprocmask( SIG_SETMASK, &none, nullptr );
    if( exec.directory != nullptr && ::chdir( exec.directory ) != 0 ) {
        say( "pty: cannot change to the directory asked for; staying where it was\n" );
    }
    char *const *environment = exec.envp.empty() ? environ : exec.envp.data();
    int error = ENOENT; // not on PATH
    if( exec.path != nullptr ) {
        ::execve( exec.path, exec.argv.data(), environment );
        error = errno;
    }
    say( exec.failure, exec.failure_size );
    say( reason( error ) );
    ::_exit( 127 );
}

ProcessExit decode( int status ) noexcept
{
    if( WIFSIGNALED( status ) ) {
        return { .code = 0, .signal = WTERMSIG( status ) };
    }
    return { .code = WIFEXITED( status ) ? WEXITSTATUS( status ) : 0, .signal = 0 };
}

} // namespace

Pty::Pty( Pty &&other ) noexcept
    : master_( std::exchange( other.master_, -1 ) )
    , pid_( std::exchange( other.pid_, 0 ) )
    , status_( other.status_ )
    , exited_( other.exited_ )
    , closed_( other.closed_ )
{}

Pty &Pty::operator=( Pty &&other ) noexcept
{
    if( this != &other ) {
        hang_up();
        master_ = std::exchange( other.master_, -1 );
        pid_ = std::exchange( other.pid_, 0 );
        status_ = other.status_;
        exited_ = other.exited_;
        closed_ = other.closed_;
    }
    return *this;
}

Pty::~Pty()
{
    hang_up();
}

PtyError Pty::spawn( const PtyConfig &config )
{
    if( pid_ > 0 && !exited_ ) {
        return { PtyErrorCode::already_running, 0 };
    }
    if( config.program.empty() ) {
        return { PtyErrorCode::no_program, 0 };
    }
    hang_up();
    const int master = ::posix_openpt( O_RDWR | O_NOCTTY | O_CLOEXEC );
    if( master < 0 ) {
        return { PtyErrorCode::open_master, errno };
    }
    std::array<char, 128> name{};
    if( ::grantpt( master ) != 0 || ::unlockpt( master ) != 0 ||
        ::ptsname_r( master, name.data(), name.size() ) != 0 ) {
        const int error = errno;
        ::close( master );
        return { PtyErrorCode::open_master, error };
    }
    const int slave = ::open( name.data(), O_RDWR | O_NOCTTY | O_CLOEXEC );
    if( slave < 0 ) {
        const int error = errno;
        ::close( master );
        return { PtyErrorCode::open_slave, error };
    }
    // UTF-8 input (so the line editor erases a whole character), and no
    // XON/XOFF: Ctrl-S and Ctrl-Q reach the program.
    termios modes{};
    if( ::tcgetattr( slave, &modes ) == 0 ) {
        modes.c_iflag |= IUTF8;
        modes.c_iflag &= ~static_cast<tcflag_t>( IXON | IXOFF );
        ::tcsetattr( slave, TCSANOW, &modes );
    }
    winsize size{};
    size.ws_col = std::max<std::uint16_t>( config.columns, 1 );
    size.ws_row = std::max<std::uint16_t>( config.rows, 1 );
    ::ioctl( master, TIOCSWINSZ, &size );

    const Exec exec = prepare( config );
    int max_fd = 1024;
    rlimit files{};
    if( ::getrlimit( RLIMIT_NOFILE, &files ) == 0 && files.rlim_cur != RLIM_INFINITY ) {
        max_fd = static_cast<int>( std::min<rlim_t>( files.rlim_cur, 65536 ) );
    }
    const pid_t child = ::fork();
    if( child < 0 ) {
        const int error = errno;
        ::close( slave );
        ::close( master );
        return { PtyErrorCode::fork, error };
    }
    if( child == 0 ) {
        run_child( slave, max_fd, exec );
    }
    ::close( slave );
    ::fcntl( master, F_SETFL, ::fcntl( master, F_GETFL ) | O_NONBLOCK );
    master_ = master;
    pid_ = child;
    status_ = 0;
    exited_ = false;
    closed_ = false;
    return {};
}

std::size_t Pty::read( std::span<std::byte> into ) noexcept
{
    if( master_ < 0 || into.empty() ) {
        return 0;
    }
    for( ;; ) {
        const ssize_t got = ::read( master_, into.data(), into.size() );
        if( got > 0 ) {
            return static_cast<std::size_t>( got );
        }
        if( got < 0 && errno == EINTR ) {
            continue;
        }
        // EIO (Linux's answer once the other end is closed by every
        // process), or the end of the file: nothing more will come.
        if( got == 0 || errno != EAGAIN ) {
            closed_ = true;
        }
        return 0;
    }
}

std::size_t Pty::write( std::span<const std::byte> bytes, int timeout_ms ) noexcept
{
    if( master_ < 0 ) {
        return 0;
    }
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds( timeout_ms );
    std::size_t done = 0;
    while( done < bytes.size() ) {
        const ssize_t wrote = ::write( master_, bytes.data() + done, bytes.size() - done );
        if( wrote > 0 ) {
            done += static_cast<std::size_t>( wrote );
            continue;
        }
        if( wrote < 0 && errno == EINTR ) {
            continue;
        }
        if( wrote < 0 && errno != EAGAIN ) {
            break;
        }
        // Full: the program is not reading. Wait for room, until the deadline.
        const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now() );
        if( left.count() <= 0 ) {
            break;
        }
        pollfd waiting{ master_, POLLOUT, 0 };
        if( ::poll( &waiting, 1, static_cast<int>( left.count() ) ) <= 0 ) {
            break;
        }
    }
    return done;
}

void Pty::resize( std::uint16_t columns, std::uint16_t rows ) noexcept
{
    if( master_ < 0 ) {
        return;
    }
    winsize size{};
    size.ws_col = std::max<std::uint16_t>( columns, 1 );
    size.ws_row = std::max<std::uint16_t>( rows, 1 );
    ::ioctl( master_, TIOCSWINSZ, &size );
}

bool Pty::wait_readable( int timeout_ms ) const noexcept
{
    if( master_ < 0 ) {
        return false;
    }
    pollfd waiting{ master_, POLLIN, 0 };
    return ::poll( &waiting, 1, timeout_ms ) > 0;
}

std::optional<ProcessExit> Pty::poll_exit() noexcept
{
    if( exited_ ) {
        return decode( status_ );
    }
    if( pid_ <= 0 ) {
        return std::nullopt;
    }
    int status = 0;
    const pid_t reaped = ::waitpid( pid_, &status, WNOHANG );
    if( reaped == pid_ ) {
        exited_ = true;
        status_ = status;
        return decode( status );
    }
    if( reaped < 0 && errno == ECHILD ) {
        // Reaped by someone else (a SIGCHLD handler of the app's own).
        exited_ = true;
        status_ = 0;
        return decode( 0 );
    }
    return std::nullopt;
}

void Pty::hang_up() noexcept
{
    if( master_ >= 0 ) {
        ::close( master_ );
        master_ = -1;
        closed_ = true;
    }
    if( pid_ <= 0 || exited_ ) {
        return;
    }
    // SIGHUP to its process group, as a terminal closing does; a program
    // that is still there after 200 ms is killed.
    ::kill( -pid_, SIGHUP );
    for( int i = 0; i < 20; ++i ) {
        if( poll_exit() ) {
            return;
        }
        timespec pause{ 0, 10'000'000 };
        ::nanosleep( &pause, nullptr );
    }
    ::kill( -pid_, SIGKILL );
    ::kill( pid_, SIGKILL );
    int status = 0;
    while( ::waitpid( pid_, &status, 0 ) < 0 && errno == EINTR ) {
    }
    exited_ = true;
    status_ = status;
}

} // namespace sq::term
