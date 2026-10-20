// The bridge against real sockets on 127.0.0.1, the event loop played by
// poll(): the secret, commands in and lines out, a client taking over, a slow
// client (lines dropped and counted, nothing allocated), closing. Then the
// Termux end, sqconsole, run for real: --run, and interactive with its
// reconnect when the app's bridge opens again.

#include <squared/console/bridge.hpp>
#include <squared/console/console.hpp>
#include <squared/console/log_file.hpp>
#include <squared/dev/frame_allocations.hpp>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#ifndef SQCONSOLE_PATH
#error "SQCONSOLE_PATH: the test is built by console/Makefile, which names the sqconsole it built"
#endif

namespace
{

using sq::console::Bridge;
using sq::console::BridgeStatus;
using sq::console::Level;

constexpr std::string_view secret = "0123456789abcdef0123456789abcdef";

/// The owner's side: which sockets to watch, a poll() over them, and what the
/// bridge asked for.
struct Loop
{
        struct Watched
        {
                int fd;
                bool write;
        };
        std::vector<Watched> watched;
        std::vector<std::string> commands;
        int connects = 0;
        bool last_replay = false;
        Bridge *bridge = nullptr;
        sq::console::Console *console = nullptr;

        static void watch( void *context, int fd, bool read, bool write ) noexcept
        {
            auto &self = *static_cast<Loop *>( context );
            for( auto at = self.watched.begin(); at != self.watched.end(); ++at ) {
                if( at->fd == fd ) {
                    self.watched.erase( at );
                    break;
                }
            }
            if( read || write ) {
                self.watched.push_back( { fd, write } );
            }
        }
        static void command( void *context, std::string_view line ) noexcept
        {
            auto &self = *static_cast<Loop *>( context );
            self.commands.emplace_back( line );
            if( self.console != nullptr ) {
                self.console->run( line );
            }
        }
        static void connected( void *context, bool replay ) noexcept
        {
            auto &self = *static_cast<Loop *>( context );
            ++self.connects;
            self.last_replay = replay;
            if( replay && self.console != nullptr ) {
                for( const auto line : self.console->lines() ) {
                    self.bridge->send( line.level, line.text );
                }
            }
        }

        [[nodiscard]] sq::console::BridgeHandlers handlers()
        {
            return { .watch = watch, .command = command, .connected = connected, .context = this };
        }

        /// One round: wait up to `ms` for a watched socket, hand each ready one on.
        void pump( int ms = 50 )
        {
            std::vector<pollfd> fds;
            fds.reserve( watched.size() );
            for( const Watched &w : watched ) {
                fds.push_back(
                    { w.fd, static_cast<short>( POLLIN | ( w.write ? POLLOUT : 0 ) ), 0 } );
            }
            if( fds.empty() ) {
                return;
            }
            if( ::poll( fds.data(), fds.size(), ms ) <= 0 ) {
                return;
            }
            for( const pollfd &p : fds ) {
                if( p.revents != 0 ) {
                    bridge->ready( p.fd, ( p.revents & ( POLLIN | POLLHUP | POLLERR ) ) != 0,
                                   ( p.revents & POLLOUT ) != 0 );
                }
            }
        }
};

sockaddr_in loopback( std::uint16_t port )
{
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons( port );
    address.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
    return address;
}

int connect_client( std::uint16_t port )
{
    const int fd = ::socket( AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0 );
    assert( fd >= 0 );
    const sockaddr_in address = loopback( port );
    const int connected =
        ::connect( fd, reinterpret_cast<const sockaddr *>( &address ), sizeof( address ) );
    assert( connected == 0 );
    return fd;
}

void say( int fd, std::string_view text )
{
    const ssize_t wrote = ::send( fd, text.data(), text.size(), MSG_NOSIGNAL );
    assert( wrote == static_cast<ssize_t>( text.size() ) );
}

/// What a person types into sqconsole: its stdin is a pipe, not a socket.
void type_into( int pipe, std::string_view text )
{
    const ssize_t wrote = ::write( pipe, text.data(), text.size() );
    assert( wrote == static_cast<ssize_t>( text.size() ) );
}

/// Everything the client has, after pumping the loop a few times. "" and
/// closed=true at the end of the stream.
std::string hear( Loop &loop, int fd, bool *closed = nullptr, int rounds = 6 )
{
    std::string got;
    for( int round = 0; round < rounds; ++round ) {
        loop.pump( 20 );
        std::array<char, 4096> buffer{};
        for( ;; ) {
            const ssize_t n = ::recv( fd, buffer.data(), buffer.size(), MSG_DONTWAIT );
            if( n == 0 && closed != nullptr ) {
                *closed = true;
            }
            if( n <= 0 ) {
                break;
            }
            got.append( buffer.data(), static_cast<std::size_t>( n ) );
        }
    }
    return got;
}

std::uint16_t free_port()
{
    const int fd = ::socket( AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0 );
    sockaddr_in address = loopback( 0 );
    socklen_t length = sizeof( address );
    const int bound =
        ::bind( fd, reinterpret_cast<const sockaddr *>( &address ), sizeof( address ) );
    const int named = ::getsockname( fd, reinterpret_cast<sockaddr *>( &address ), &length );
    assert( bound == 0 && named == 0 );
    ::close( fd );
    return ntohs( address.sin_port );
}

std::string hello( std::string_view given, bool quiet = false )
{
    return std::string( "squared-bridge 1 " ) + std::string( given ) + ( quiet ? " quiet" : "" ) +
           "\n";
}

bool contains( const std::string &text, std::string_view part )
{
    return text.find( part ) != std::string::npos;
}

void protocol_tests()
{
    Bridge bridge;
    Loop loop;
    loop.bridge = &bridge;

    // Secrets that are not 16 to 64 letters and digits are refused.
    assert( bridge.open( { .secret = "short" }, loop.handlers() ) == BridgeStatus::bad_secret );
    assert( bridge.open( { .secret = "0123456789abcdef!" }, loop.handlers() ) ==
            BridgeStatus::bad_secret );
    assert( !bridge.listening() );

    // The announcement reaches a waiting client on the same port number.
    const std::uint16_t port = free_port();
    const int announce = ::socket( AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0 );
    const sockaddr_in at = loopback( port );
    assert( ::bind( announce, reinterpret_cast<const sockaddr *>( &at ), sizeof( at ) ) == 0 );
    assert( bridge.open( { .port = port, .secret = secret }, loop.handlers() ) ==
            BridgeStatus::listening );
    assert( bridge.port() == port && bridge.listening() && !bridge.connected() );
    assert( bridge.open( { .port = port, .secret = secret }, loop.handlers() ) ==
            BridgeStatus::already_open );
    {
        pollfd p{ announce, POLLIN, 0 };
        assert( ::poll( &p, 1, 1000 ) == 1 );
        std::array<char, 64> datagram{};
        const ssize_t n = ::recv( announce, datagram.data(), datagram.size(), 0 );
        assert( std::string_view( datagram.data(), static_cast<std::size_t>( n ) ) ==
                "squared-bridge up" );
        ::close( announce );
    }
    assert( loop.watched.size() == 1 ); // the listener only

    // Nothing is sent, nor kept, with no client.
    bridge.send( Level::info, "nobody hears this" );

    // A wrong secret is closed without an answer, and runs nothing.
    {
        const int fd = connect_client( port );
        say( fd, hello( "0123456789abcdef0123456789abcdeX" ) + "help\n" );
        bool closed = false;
        const std::string got = hear( loop, fd, &closed );
        assert( got.empty() && closed );
        assert( loop.commands.empty() && loop.connects == 0 && !bridge.connected() );
        ::close( fd );
    }
    // Not a hello at all: closed too.
    {
        const int fd = connect_client( port );
        say( fd, "GET / HTTP/1.0\n" );
        bool closed = false;
        assert( hear( loop, fd, &closed ).empty() && closed );
        ::close( fd );
    }

    // The right secret: "ok", the replay asked for, then commands and lines.
    const int first = connect_client( port );
    say( first, hello( secret ) );
    std::string got = hear( loop, first );
    assert( got == "ok\n" );
    assert( bridge.connected() && loop.connects == 1 && loop.last_replay );
    say( first, "help\n" );
    got = hear( loop, first );
    assert( loop.commands.size() == 1 && loop.commands[0] == "help" );
    assert( got == ".\n" ); // no Console here: only the end of the command
    bridge.send( Level::error, "shader did not link" );
    bridge.send( Level::warning, "slow frame" );
    bridge.send( Level::info, "two\nlines? no: one, as given" );
    got = hear( loop, first );
    assert( contains( got, "e shader did not link\n" ) && contains( got, "w slow frame\n" ) );
    assert( contains( got, "i two lines? no: one, as given\n" ) );

    // A command longer than 256 bytes runs cut, once; the next is whole.
    say( first, std::string( 300, 'x' ) + "\nmem\r\n" );
    got = hear( loop, first );
    assert( loop.commands.size() == 3 );
    assert( loop.commands[1] == std::string( Bridge::command_bytes, 'x' ) );
    assert( loop.commands[2] == "mem" );

    // A quiet client takes over: the first is told and closed; the hello and
    // a command in one write both count.
    const int second = connect_client( port );
    say( second, hello( secret, true ) + "fds\n" );
    got = hear( loop, second );
    assert( got == "ok\n.\n" );
    assert( loop.connects == 2 && !loop.last_replay && loop.commands.back() == "fds" );
    bool closed = false;
    const std::string told = hear( loop, first, &closed );
    assert( contains( told, "w bridge: another connection took over\n" ) && closed );
    ::close( first );

    // A client that stops reading: lines are dropped and counted, nothing
    // allocates, and it is told once it reads again.
    // The kernel buffers a few MB on loopback before the bridge's own 16 KB
    // fills, so send until a line is dropped.
    const std::string line( 900, 'y' );
    int sent = 0;
    sq::dev::frame_begin();
    for( ; sent < 200000 && bridge.dropped() < 100; ++sent ) {
        bridge.send( Level::info, line );
    }
    const sq::dev::FrameAllocations used = sq::dev::frame_end();
    assert( used.count == 0 );
    assert( bridge.dropped() >= 100 );
    const std::uint64_t dropped = bridge.dropped();
    std::string drained;
    for( int round = 0; round < 40 && !contains( drained, "lines dropped" ); ++round ) {
        drained += hear( loop, second, nullptr, 2 );
        bridge.send( Level::info, "after" );
    }
    assert( contains( drained, "w bridge: " ) && contains( drained, "lines dropped" ) );
    std::printf( "bridge: %llu of %d lines dropped while the client did not read; told\n",
                 static_cast<unsigned long long>( dropped ), sent );

    // The client goes: the bridge notices, and keeps listening.
    ::close( second );
    hear( loop, -1, nullptr, 3 );
    loop.pump( 50 );
    assert( !bridge.connected() && bridge.listening() );

    // close() stops every watch.
    const int third = connect_client( port ); // pending, never says hello
    loop.pump( 50 );
    assert( loop.watched.size() == 2 );
    bridge.close();
    assert( loop.watched.empty() && !bridge.listening() );
    ::close( third );
}

/// The Console's lines reach the client through set_mirror, and its
/// commands run: the echo, the output, the end mark.
void console_tests()
{
    static sq::console::Console console;
    Bridge bridge;
    Loop loop;
    loop.bridge = &bridge;
    loop.console = &console;
    console.print( "before the client" );
    console.set_mirror(
        []( void *b, Level level, std::string_view text ) noexcept {
            static_cast<Bridge *>( b )->send( level, text );
        },
        &bridge );
    assert( bridge.open( { .port = 0, .secret = secret, .announce = false }, loop.handlers() ) ==
            BridgeStatus::listening );
    const int fd = connect_client( bridge.port() );
    say( fd, hello( secret ) );
    std::string got = hear( loop, fd );
    assert( got.rfind( "ok\n", 0 ) == 0 && contains( got, "i before the client\n" ) );
    say( fd, "nosuch\n" );
    got = hear( loop, fd );
    assert( contains( got, "i > nosuch\n" ) && contains( got, "e " ) && contains( got, ".\n" ) );
    ::close( fd );
    hear( loop, -1, nullptr, 2 );
    console.set_mirror( nullptr, nullptr );
    bridge.close();
}

/// save_log, keep_previous_log, show_log, `log previous`.
void log_file_tests()
{
    static sq::console::Console console;
    char directory[] = "/tmp/sq_log_test_XXXXXX";
    assert( ::mkdtemp( directory ) != nullptr );
    const std::string log = std::string( directory ) + "/log.txt";
    const std::string previous = std::string( directory ) + "/log-previous.txt";
    for( int i = 0; i < 50; ++i ) {
        console.printf( Level::info, "line %d", i );
    }
    sq::dev::frame_begin();
    assert( sq::console::save_log( console, log.c_str(), 10 ) );
    assert( sq::dev::frame_end().count == 0 );
    assert( ::access( ( log + ".tmp" ).c_str(), F_OK ) != 0 );
    assert( sq::console::keep_previous_log( log.c_str(), previous.c_str() ) );
    assert( !sq::console::keep_previous_log( log.c_str(), previous.c_str() ) );
    assert( sq::console::add_log_command( console, previous.c_str() ) ==
            sq::console::Commands::AddResult::added );
    console.lines().clear();
    console.run( "log previous" );
    std::vector<std::string> shown;
    for( const auto line : console.lines() ) {
        shown.emplace_back( line.text );
    }
    assert( shown.size() == 12 );
    assert( shown[1] == "log: how the last run ended" && shown[2] == "line 40" &&
            shown[11] == "line 49" );
    console.lines().clear();
    console.run( "log" );
    assert( ( *console.lines().last( 1 ) ).level == Level::error );
    ::unlink( previous.c_str() );
    console.run( "log previous" );
    assert( ( *console.lines().last( 1 ) ).text == "log: no earlier run's log is kept" );
    assert( !sq::console::save_log( console, "/nonexistent-dir/x/log.txt", 5 ) );
    ::rmdir( directory );
}

/// A child process with its stdin and stdout piped to us.
struct Child
{
        pid_t pid = -1;
        int in = -1;
        int out = -1;
};

Child spawn( const std::vector<std::string> &arguments )
{
    std::array<int, 2> to{};
    std::array<int, 2> from{};
    assert( ::pipe( to.data() ) == 0 && ::pipe( from.data() ) == 0 );
    const pid_t pid = ::fork();
    assert( pid >= 0 );
    if( pid == 0 ) {
        ::dup2( to[0], STDIN_FILENO );
        ::dup2( from[1], STDOUT_FILENO );
        ::dup2( from[1], STDERR_FILENO );
        ::close( to[1] );
        ::close( from[0] );
        std::vector<char *> argv;
        argv.reserve( arguments.size() + 1 );
        for( const std::string &a : arguments ) {
            argv.push_back( const_cast<char *>( a.c_str() ) );
        }
        argv.push_back( nullptr );
        ::execv( argv[0], argv.data() );
        _exit( 127 );
    }
    ::close( to[0] );
    ::close( from[1] );
    ::fcntl( from[0], F_SETFL, O_NONBLOCK );
    return { pid, to[1], from[0] };
}

/// Pump the bridge while collecting the child's output, until `want` appears.
bool await( Loop &loop, const Child &child, std::string &got, std::string_view want,
            int seconds = 5 )
{
    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds( seconds );
    while( std::chrono::steady_clock::now() < until ) {
        if( contains( got, want ) ) {
            return true;
        }
        loop.pump( 10 );
        std::array<char, 4096> buffer{};
        const ssize_t n = ::read( child.out, buffer.data(), buffer.size() );
        if( n > 0 ) {
            got.append( buffer.data(), static_cast<std::size_t>( n ) );
        } else if( loop.watched.empty() ) {
            std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
        }
    }
    return contains( got, want );
}

void client_tests()
{
    static sq::console::Console console;
    Bridge bridge;
    Loop loop;
    loop.bridge = &bridge;
    loop.console = &console;
    console.add( "say", "say <word>",
                 []( sq::console::Console &c, const sq::console::Args &a, void * ) {
                     c.printf( Level::warning, "said %.*s", static_cast<int>( a.text( 1 ).size() ),
                               a.text( 1 ).data() );
                 } );
    console.set_mirror(
        []( void *b, Level level, std::string_view text ) noexcept {
            static_cast<Bridge *>( b )->send( level, text );
        },
        &bridge );

    const std::uint16_t port = free_port();
    char conf[] = "/tmp/sq_bridge_conf_XXXXXX";
    const int conf_fd = ::mkstemp( conf );
    assert( conf_fd >= 0 );
    const std::string text = "port=" + std::to_string( port ) +
                             "\nsecret=" + std::string( secret ) + "\npackage=com.example.test\n";
    assert( ::write( conf_fd, text.data(), text.size() ) == static_cast<ssize_t>( text.size() ) );
    ::close( conf_fd );

    // Interactive, started before the app: it waits, and connects when the
    // bridge opens and announces itself.
    Child interactive = spawn( { SQCONSOLE_PATH, "--conf", conf } );
    std::string seen;
    assert( await( loop, interactive, seen, "waiting for the app to start" ) );
    console.print( "the app's first line" );
    assert( bridge.open( { .port = port, .secret = secret }, loop.handlers() ) ==
            BridgeStatus::listening );
    assert( await( loop, interactive, seen, "connected to com.example.test" ) );
    assert( await( loop, interactive, seen, "the app's first line" ) ); // the replay
    type_into( interactive.in, "say hello\n" );
    assert( await( loop, interactive, seen, "said hello" ) );

    // The app goes, and comes back: it notices both, and reconnects by itself.
    bridge.close();
    assert( await( loop, interactive, seen, "the app went away" ) );
    seen.clear();
    assert( bridge.open( { .port = port, .secret = secret }, loop.handlers() ) ==
            BridgeStatus::listening );
    assert( await( loop, interactive, seen, "connected to com.example.test" ) );
    type_into( interactive.in, "say again\n" );
    assert( await( loop, interactive, seen, "said again" ) );

    // --run while the interactive one is connected: it takes over, runs both
    // commands, prints only their output, and exits 0.
    Child run = spawn(
        { SQCONSOLE_PATH, "--conf", conf, "--run", "say one", "--run", "say two", "--wait", "5" } );
    std::string ran;
    assert( await( loop, run, ran, "said two" ) );
    for( int i = 0; i < 50; ++i ) {
        loop.pump( 10 );
        int status = 0;
        if( ::waitpid( run.pid, &status, WNOHANG ) == run.pid ) {
            assert( WIFEXITED( status ) && WEXITSTATUS( status ) == 0 );
            run.pid = -1;
            break;
        }
    }
    assert( run.pid == -1 );
    assert( contains( ran, "> say one" ) && contains( ran, "said one" ) );
    assert( !contains( ran, "the app's first line" ) ); // quiet: no replay
    assert( await( loop, interactive, seen, "another connection took over" ) );

    // A wrong secret in the conf: refused, exit 1.
    {
        char bad[] = "/tmp/sq_bridge_bad_XXXXXX";
        const int fd = ::mkstemp( bad );
        const std::string wrong =
            "port=" + std::to_string( port ) + "\nsecret=ffffffffffffffffffffffffffffffff\n";
        assert( ::write( fd, wrong.data(), wrong.size() ) == static_cast<ssize_t>( wrong.size() ) );
        ::close( fd );
        Child refused = spawn( { SQCONSOLE_PATH, "--conf", bad, "--run", "say x", "--wait", "2" } );
        std::string said;
        assert( await( loop, refused, said, "refused" ) );
        int status = 0;
        for( int i = 0; i < 300 && ::waitpid( refused.pid, &status, WNOHANG ) == 0; ++i ) {
            loop.pump( 10 );
        }
        assert( WIFEXITED( status ) && WEXITSTATUS( status ) == 1 );
        ::unlink( bad );
    }

    ::close( interactive.in ); // Ctrl-D
    int status = 0;
    for( int i = 0; i < 300 && ::waitpid( interactive.pid, &status, WNOHANG ) == 0; ++i ) {
        loop.pump( 10 );
    }
    assert( WIFEXITED( status ) && WEXITSTATUS( status ) == 0 );
    console.set_mirror( nullptr, nullptr );
    bridge.close();
    ::unlink( conf );
}

} // namespace

int main()
{
    protocol_tests();
    console_tests();
    log_file_tests();
    client_tests();
    std::printf( "bridge: all assertions passed  sizeof(Bridge)=%zu\n", sizeof( Bridge ) );
    return 0;
}
