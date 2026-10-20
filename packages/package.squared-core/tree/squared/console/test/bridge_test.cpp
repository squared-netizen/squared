// The bridge against real sockets on 127.0.0.1, the event loop played by
// poll(): the secret, commands in and lines out, a client taking over, a slow
// client (lines dropped and counted, nothing allocated), closing. Then the
// Termux end, sqconsole, run for real: --run, and interactive with its
// reconnect when the app's bridge opens again.

#include <squared/console/bridge.hpp>
#include <squared/console/console.hpp>
#include <squared/console/file_commands.hpp>
#include <squared/console/log_file.hpp>
#include <squared/console/system_commands.hpp>
#include <squared/dev/frame_allocations.hpp>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
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
        static void note( void *context, Level level, std::string_view line ) noexcept
        {
            auto &self = *static_cast<Loop *>( context );
            if( self.console != nullptr ) {
                self.console->print( line, level );
            } else {
                self.bridge->send( level, line );
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
            return { .watch = watch,
                     .command = command,
                     .connected = connected,
                     .note = note,
                     .context = this };
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

/// A mkdtemp/mkstemp pattern in the temporary directory: $TMPDIR, as Termux
/// sets it (Android has no /tmp), else /tmp.
std::string temporary( std::string_view name )
{
    const char *directory = std::getenv( "TMPDIR" );
    return std::string( directory != nullptr && *directory != '\0' ? directory : "/tmp" ) + "/" +
           std::string( name ) + "_XXXXXX";
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
    std::string directory = temporary( "sq_log_test" );
    assert( ::mkdtemp( directory.data() ) != nullptr );
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
    ::rmdir( directory.c_str() );
}

std::string random_bytes( std::size_t size )
{
    std::string bytes( size, '\0' );
    std::uint32_t state = 12345;
    for( char &c : bytes ) {
        state = state * 1664525U + 1013904223U;
        c = static_cast<char>( state >> 24 );
    }
    return bytes;
}

void write_whole( const std::string &path, const std::string &bytes )
{
    const int fd = ::open( path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600 );
    assert( fd >= 0 );
    assert( ::write( fd, bytes.data(), bytes.size() ) == static_cast<ssize_t>( bytes.size() ) );
    ::close( fd );
}

std::string read_whole( const std::string &path )
{
    std::string bytes;
    const int fd = ::open( path.c_str(), O_RDONLY | O_CLOEXEC );
    if( fd < 0 ) {
        return "<missing>";
    }
    std::array<char, 8192> buffer{};
    for( ssize_t n = 0; ( n = ::read( fd, buffer.data(), buffer.size() ) ) > 0; ) {
        bytes.append( buffer.data(), static_cast<std::size_t>( n ) );
    }
    ::close( fd );
    return bytes;
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

    std::string home = temporary( "sq_client_home" );
    assert( ::mkdtemp( home.data() ) != nullptr );
    const std::string files = home + "/files";
    const std::string pulled = home + "/pulled";
    ::mkdir( files.c_str(), 0700 );
    assert( sq::console::add_file_commands( console, home.c_str(), &bridge ) == 0 );

    const std::uint16_t port = free_port();
    std::string conf_path = temporary( "sq_bridge_conf" );
    const int conf_fd = ::mkstemp( conf_path.data() );
    const char *conf = conf_path.c_str();
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
    assert( bridge.open( { .port = port, .secret = secret, .push_directory = files },
                         loop.handlers() ) == BridgeStatus::listening );
    assert( await( loop, interactive, seen, "connected to com.example.test" ) );
    assert( await( loop, interactive, seen, "the app's first line" ) ); // the replay
    type_into( interactive.in, "say hello\n" );
    assert( await( loop, interactive, seen, "said hello" ) );

    // The app goes, and comes back: it notices both, and reconnects by itself.
    bridge.close();
    assert( await( loop, interactive, seen, "the app went away" ) );
    seen.clear();
    assert( bridge.open( { .port = port, .secret = secret, .push_directory = files },
                         loop.handlers() ) == BridgeStatus::listening );
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

    // pull and push through sqconsole: --run, then the files are compared.
    const std::string blob = random_bytes( 70001 );
    write_whole( files + "/blob.bin", blob );
    const std::string local = home + "/mine.bin";
    const std::string mine = random_bytes( 12345 );
    write_whole( local, mine );
    Child files_run =
        spawn( { SQCONSOLE_PATH, "--conf", conf, "--into", pulled, "--run", "pull files/blob.bin",
                 "--run", "push " + local + " theirs.bin", "--wait", "5" } );
    std::string moved;
    assert( await( loop, files_run, moved, "theirs.bin, 12345 bytes" ) );
    for( int i = 0; i < 300; ++i ) {
        loop.pump( 10 );
        int status = 0;
        if( ::waitpid( files_run.pid, &status, WNOHANG ) == files_run.pid ) {
            assert( WIFEXITED( status ) && WEXITSTATUS( status ) == 0 );
            files_run.pid = -1;
            break;
        }
    }
    assert( files_run.pid == -1 );
    assert( contains( moved, "saved " + pulled + "/blob.bin (70001 bytes)" ) );
    assert( read_whole( pulled + "/blob.bin" ) == blob );
    assert( read_whole( files + "/theirs.bin" ) == mine );
    std::printf( "bridge: pulled 70001 and pushed 12345 bytes through sqconsole, both exact\n" );

    // A wrong secret in the conf: refused, exit 1.
    {
        std::string bad_path = temporary( "sq_bridge_bad" );
        const int fd = ::mkstemp( bad_path.data() );
        const char *bad = bad_path.c_str();
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

    // --until-closed: the app closing the connection after `bye` is the
    // success asked for (make console-only quits a running app this way).
    {
        static bool bye = false;
        console.add( "bye", "bye",
                     []( sq::console::Console &c, const sq::console::Args &, void * ) {
                         bye = true;
                         c.print( "bye then" );
                     } );
        Child quitting = spawn(
            { SQCONSOLE_PATH, "--conf", conf, "--wait", "0", "--run", "bye", "--until-closed" } );
        std::string said;
        assert( await( loop, quitting, said, "bye then" ) );
        assert( bye );
        int status = 0;
        for( int i = 0; i < 30 && ::waitpid( quitting.pid, &status, WNOHANG ) == 0; ++i ) {
            loop.pump( 10 ); // the command's end mark arrives; it keeps waiting
        }
        assert( ::waitpid( quitting.pid, &status, WNOHANG ) == 0 );
        bridge.close(); // as the app does when it has finished
        for( int i = 0; i < 300 && ::waitpid( quitting.pid, &status, WNOHANG ) == 0; ++i ) {
            std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
        }
        assert( WIFEXITED( status ) && WEXITSTATUS( status ) == 0 );
        assert( await( loop, interactive, seen, "the app went away" ) );

        // No app, --wait 0: fails at once rather than waiting.
        const auto started = std::chrono::steady_clock::now();
        Child none = spawn(
            { SQCONSOLE_PATH, "--conf", conf, "--wait", "0", "--run", "bye", "--until-closed" } );
        status = 0;
        for( int i = 0; i < 300 && ::waitpid( none.pid, &status, WNOHANG ) == 0; ++i ) {
            std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
        }
        assert( WIFEXITED( status ) && WEXITSTATUS( status ) == 1 );
        assert( std::chrono::steady_clock::now() - started < std::chrono::seconds( 2 ) );
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
    for( const std::string &path :
         { files + "/blob.bin", files + "/theirs.bin", pulled + "/blob.bin", local } ) {
        ::unlink( path.c_str() );
    }
    ::rmdir( files.c_str() );
    ::rmdir( pulled.c_str() );
    ::rmdir( home.c_str() );
}

/// What came over the wire, frames understood: lines, and a pulled file.
struct Heard
{
        std::vector<std::string> lines; // "<type> <text>", "." and "ok" as they came
        std::string file_header;        // "<size> <name>" of the last "p"
        std::string file;               // its bytes
        int done = 0;
};

/// Pump and read until `dones` end marks have arrived (or 5 s), parsing frames.
Heard listen( Loop &loop, int fd, int dones, const std::function<void()> &between = {} )
{
    Heard heard;
    std::string held;
    std::uint64_t data_left = 0;
    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
    while( heard.done < dones && std::chrono::steady_clock::now() < until ) {
        loop.pump( 5 );
        if( between ) {
            between();
        }
        std::array<char, 8192> buffer{};
        const ssize_t n = ::recv( fd, buffer.data(), buffer.size(), MSG_DONTWAIT );
        if( n > 0 ) {
            held.append( buffer.data(), static_cast<std::size_t>( n ) );
        }
        for( ;; ) {
            if( data_left != 0 ) {
                const std::size_t take = std::min<std::size_t>( data_left, held.size() );
                heard.file.append( held, 0, take );
                held.erase( 0, take );
                data_left -= take;
                if( data_left != 0 ) {
                    break;
                }
                continue;
            }
            const auto end = held.find( '\n' );
            if( end == std::string::npos ) {
                break;
            }
            const std::string line = held.substr( 0, end );
            held.erase( 0, end + 1 );
            if( line.rfind( "p ", 0 ) == 0 ) {
                heard.file_header = line.substr( 2 );
                heard.file.clear();
            } else if( line.rfind( "d ", 0 ) == 0 ) {
                data_left = std::stoull( line.substr( 2 ) );
                assert( data_left > 0 && data_left <= 4096 );
            } else {
                if( line == "." ) {
                    ++heard.done;
                }
                heard.lines.push_back( line );
            }
        }
    }
    return heard;
}

bool heard_line( const Heard &heard, std::string_view part )
{
    for( const std::string &line : heard.lines ) {
        if( line.find( part ) != std::string::npos ) {
            return true;
        }
    }
    return false;
}

/// pull and push through raw sockets; ls, libs, run_program in the Console.
void transfer_tests( const std::string &home )
{
    static sq::console::Console console;
    Bridge bridge;
    Loop loop;
    loop.bridge = &bridge;
    loop.console = &console;
    assert( sq::console::add_file_commands( console, home.c_str(), &bridge ) == 0 );
    console.set_mirror(
        []( void *b, Level level, std::string_view text ) noexcept {
            static_cast<Bridge *>( b )->send( level, text );
        },
        &bridge );
    const std::string files = home + "/files";
    ::mkdir( files.c_str(), 0700 );
    assert(
        bridge.open( { .port = 0, .secret = secret, .announce = false, .push_directory = files },
                     loop.handlers() ) == BridgeStatus::listening );

    // Without a client, pull says so.
    console.run( "pull files/none" );
    assert( ( *console.lines().last( 1 ) ).text.find( "needs Termux connected" ) !=
            std::string_view::npos );

    const int fd = connect_client( bridge.port() );
    say( fd, hello( secret, true ) );
    hear( loop, fd );

    // A 100,003-byte file, in chunks, with Console lines between them; the
    // end mark only after its last byte.
    const std::string big = random_bytes( 100003 );
    write_whole( files + "/big.bin", big );
    say( fd, "pull files/big.bin\n" );
    int sent_between = 0;
    Heard heard = listen( loop, fd, 1, [&] {
        if( sent_between < 3 && bridge.transferring() ) {
            bridge.send( Level::warning, "between chunks" );
            ++sent_between;
        }
    } );
    assert( heard.done == 1 && heard.file_header == "100003 big.bin" );
    assert( heard.file == big );
    assert( sent_between > 0 && heard_line( heard, "w between chunks" ) );
    assert( heard.lines.back() == "." && !bridge.transferring() );

    // A /proc file says size 0: it is sent until it ends, size "?".
    say( fd, "pull /proc/self/status\n" );
    heard = listen( loop, fd, 1 );
    assert( heard.file_header == "? status" && heard.file.find( "Pid:" ) != std::string::npos );

    // Errors are lines, then the end mark; nothing is sent.
    say( fd, "pull files/none\npull files\npull\n" );
    heard = listen( loop, fd, 3 );
    assert( heard.done == 3 && heard.file_header.empty() );
    assert( heard_line( heard, "e pull: " ) && heard_line( heard, "not a regular file" ) );

    // push: the bytes land in the push directory, through a .part file.
    const std::string up = random_bytes( 9000 );
    say( fd, "push 9000 up.bin\n" + up );
    heard = listen( loop, fd, 1 );
    assert( read_whole( files + "/up.bin" ) == up );
    assert( ::access( ( files + "/up.bin.part" ).c_str(), F_OK ) != 0 );
    assert( heard_line( heard, "push: " + files + "/up.bin, 9000 bytes" ) );
    // A name that is a path is refused, its bytes skipped, the next command whole.
    say( fd, "push 5 ../x\nhello" + std::string( "ls files\n" ) );
    heard = listen( loop, fd, 2 );
    assert( heard_line( heard, "e push: not a plain file name" ) );
    assert( ::access( ( home + "/x" ).c_str(), F_OK ) != 0 );
    assert( heard_line( heard, "i ls: " + files ) && heard_line( heard, "big.bin" ) &&
            heard_line( heard, "100003" ) && heard_line( heard, "ls: 2 entries" ) );
    // An empty push, and a malformed one.
    say( fd, "push 0 empty.bin\npush x y\n" );
    heard = listen( loop, fd, 2 );
    assert( read_whole( files + "/empty.bin" ).empty() && heard_line( heard, "malformed" ) );
    // The client leaving mid-push: the part file is removed.
    say( fd, "push 100000 cut.bin\n" + std::string( 100, 'z' ) );
    hear( loop, fd, nullptr, 2 );
    ::close( fd );
    hear( loop, -1, nullptr, 3 );
    assert( ::access( ( files + "/cut.bin.part" ).c_str(), F_OK ) != 0 &&
            ::access( ( files + "/cut.bin" ).c_str(), F_OK ) != 0 );

    // libs: this test's own libraries, the C library among them. ls, libs
    // and a pull's start allocate nothing.
    console.lines().clear();
    static_cast<void>( sq::console::add_system_commands( console ) );
    sq::dev::frame_begin();
    console.run( "ls files" );
    console.run( "pull files/up.bin" ); // no client now: says so
    console.lines().clear();
    console.run( "libs" );
    assert( sq::dev::frame_end().count == 0 );
    std::vector<std::string> shown;
    for( const auto line : console.lines() ) {
        shown.emplace_back( line.text );
    }
    assert( shown.size() >= 3 && shown[1].rfind( "libs: ", 0 ) == 0 );
    bool libc = false;
    for( const std::string &line : shown ) {
        if( line.find( "libc" ) != std::string::npos ) {
            libc = true;
            assert( std::strtoull( line.c_str(), nullptr, 10 ) > 0 ); // the size was parsed
        }
    }
    assert( libc );

    // run_program: its lines, at most max_lines, then a count; a missing one fails.
    console.lines().clear();
    const char *const lines[] = { "/bin/sh", "-c", "printf 'one\\ntwo\\nthree\\n'; echo err >&2",
                                  nullptr };
    sq::dev::frame_begin();
    assert( sq::console::run_program( console, lines, 2 ) );
    assert( sq::dev::frame_end().count == 0 );
    shown.clear();
    for( const auto line : console.lines() ) {
        shown.emplace_back( line.text );
    }
    assert( shown.size() == 3 && shown[0] == "one" && shown[1] == "two" &&
            shown[2] == "... and 2 more lines" );
    const char *const missing[] = { "/nonexistent/program", nullptr };
    assert( !sq::console::run_program( console, missing, 5 ) );

    console.set_mirror( nullptr, nullptr );
    bridge.close();
    for( const char *name : { "/big.bin", "/up.bin", "/empty.bin" } ) {
        ::unlink( ( files + name ).c_str() );
    }
    ::rmdir( files.c_str() );
}

} // namespace

int main()
{
    protocol_tests();
    console_tests();
    log_file_tests();
    {
        std::string home = temporary( "sq_bridge_home" );
        assert( ::mkdtemp( home.data() ) != nullptr );
        transfer_tests( home );
        ::rmdir( home.c_str() );
    }
    client_tests();
    std::printf( "bridge: all assertions passed  sizeof(Bridge)=%zu\n", sizeof( Bridge ) );
    return 0;
}
