// A program in a pty (12.2), read through a Terminal: what it writes, its
// exit code, a name found on PATH (this process's, or the given
// environment's), the environment and directory it was given, the size it sees
// and SIGWINCH on a resize, keys written to it, Ctrl-C, a program that
// cannot run, a full pty, hanging up a program that would run on, and no
// process left behind.
//
// Runs on the host: /bin/sh on Linux, /system/bin/sh on Android (Termux).

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

import sq.term;

import std;

using namespace sq::term;

namespace
{

int failures = 0;

#define CHECK( condition )                                                                         \
    do {                                                                                           \
        if( !( condition ) ) {                                                                     \
            std::fprintf( stderr, "pty_test.cpp:%d: %s\n", __LINE__, #condition );                 \
            ++failures;                                                                            \
        }                                                                                          \
    } while( false )

std::string_view shell()
{
    return ::access( "/bin/sh", X_OK ) == 0 ? "/bin/sh" : "/system/bin/sh";
}

void append_utf8( std::string &out, char32_t code )
{
    if( code < 0x80 ) {
        out += static_cast<char>( code );
    } else if( code < 0x800 ) {
        out += static_cast<char>( 0xC0 | ( code >> 6 ) );
        out += static_cast<char>( 0x80 | ( code & 0x3F ) );
    } else {
        out += static_cast<char>( 0xE0 | ( code >> 12 ) );
        out += static_cast<char>( 0x80 | ( ( code >> 6 ) & 0x3F ) );
        out += static_cast<char>( 0x80 | ( code & 0x3F ) );
    }
}

std::string screen_text( const Terminal &terminal )
{
    std::string out;
    for( std::uint16_t r = 0; r < terminal.rows(); ++r ) {
        std::string line;
        for( const Cell &cell : terminal.row( r ) ) {
            if( !cell.flags.wide_tail ) {
                append_utf8( line, cell.code );
            }
        }
        while( !line.empty() && line.back() == ' ' ) {
            line.pop_back();
        }
        if( !line.empty() ) {
            out += line;
            out += '\n';
        }
    }
    return out;
}

// Reads into the terminal until `done` holds for its text, the program has
// closed the pty, or 5 seconds pass. True when `done` held.
bool pump( Pty &pty, Terminal &terminal, const std::function<bool( const std::string & )> &done )
{
    std::array<std::byte, 4096> buffer{};
    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
    while( std::chrono::steady_clock::now() < until ) {
        if( done( screen_text( terminal ) ) ) {
            return true;
        }
        if( pty.closed() ) {
            break;
        }
        if( pty.wait_readable( 50 ) ) {
            const std::size_t got = pty.read( buffer );
            terminal.feed( std::span{ buffer.data(), got } );
            const auto replies = terminal.replies();
            if( !replies.empty() ) {
                pty.write( replies );
                terminal.clear_replies();
            }
        }
    }
    return done( screen_text( terminal ) );
}

bool contains( const std::string &text, std::string_view part )
{
    return text.find( part ) != std::string::npos;
}

std::optional<ProcessExit> wait_exit( Pty &pty )
{
    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
    while( std::chrono::steady_clock::now() < until ) {
        if( const auto exit = pty.poll_exit() ) {
            return exit;
        }
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
    }
    return std::nullopt;
}

Terminal terminal_of( std::uint16_t columns = 40, std::uint16_t rows = 10 )
{
    return Terminal{ { .columns = columns, .rows = rows, .scrollback_bytes = 64 * 1024 } };
}

void output_and_exit()
{
    Pty pty;
    const std::array<std::string_view, 2> arguments{
        "-c", "printf 'hello\\n\\303\\251t\\303\\251\\n'; exit 3"
    };
    const PtyError error = pty.spawn( { .program = shell(), .arguments = arguments } );
    CHECK( !error );
    CHECK( pty.pid() > 0 && pty.fd() >= 0 );
    Terminal terminal = terminal_of();
    CHECK( pump( pty, terminal, []( const std::string &text ) {
        return contains( text, "\xC3\xA9t\xC3\xA9" );
    } ) );
    CHECK( screen_text( terminal ) == "hello\n\xC3\xA9t\xC3\xA9\n" );
    const auto exit = wait_exit( pty );
    CHECK( exit && exit->code == 3 && exit->signal == 0 );
    CHECK( pty.poll_exit() == exit ); // asked again: the same
}

void found_on_path()
{
    // A name, found on this process's PATH; argv[0] stays the name.
    Pty pty;
    const std::array<std::string_view, 2> arguments{ "-c", "printf 'as %s\\n' \"$0\"" };
    CHECK( !pty.spawn( { .program = "sh", .arguments = arguments } ) );
    Terminal terminal = terminal_of();
    CHECK(
        pump( pty, terminal, []( const std::string &text ) { return contains( text, "as " ); } ) );
    CHECK( contains( screen_text( terminal ), "as sh" ) );
    CHECK( wait_exit( pty ) == ( ProcessExit{ 0, 0 } ) );

    // On the given environment's PATH, not this process's.
    const std::string directory{ shell().substr( 0, shell().rfind( '/' ) ) };
    const std::string path = "PATH=/sq-nowhere:" + directory;
    const std::array<std::string_view, 1> environment{ path };
    const std::array<std::string_view, 2> echo{ "-c", "echo found" };
    Pty given;
    CHECK( !given.spawn( { .program = "sh", .arguments = echo, .environment = environment } ) );
    Terminal other = terminal_of();
    CHECK(
        pump( given, other, []( const std::string &text ) { return contains( text, "found" ); } ) );
    CHECK( wait_exit( given ) == ( ProcessExit{ 0, 0 } ) );

    // A PATH without it: not found.
    const std::array<std::string_view, 1> empty_path{ "PATH=/sq-nowhere" };
    Pty lost;
    CHECK( !lost.spawn( { .program = "sh", .arguments = echo, .environment = empty_path } ) );
    Terminal third = terminal_of( 60, 5 );
    CHECK( pump( lost, third,
                 []( const std::string &text ) { return contains( text, "not found" ); } ) );
    CHECK( wait_exit( lost ) == ( ProcessExit{ 127, 0 } ) );
}

void environment_and_directory()
{
    Pty pty;
    const std::array<std::string_view, 2> arguments{
        "-c", "printf '%s|%s|' \"$SQ_TEST\" \"$TERM\"; pwd"
    };
    const std::array<std::string_view, 2> environment{ "SQ_TEST=forty two", "TERM=sqterm" };
    CHECK( !pty.spawn( { .program = shell(),
                         .arguments = arguments,
                         .environment = environment,
                         .directory = "/" } ) );
    Terminal terminal = terminal_of();
    CHECK(
        pump( pty, terminal, []( const std::string &text ) { return contains( text, "|/" ); } ) );
    CHECK( contains( screen_text( terminal ), "forty two|sqterm|/" ) );
    CHECK( wait_exit( pty ) == ( ProcessExit{ 0, 0 } ) );
}

void size_and_resize()
{
    Pty pty;
    // The size at the start, then again on SIGWINCH.
    const std::array<std::string_view, 2> arguments{
        "-c", "stty size; trap 'stty size; exit 0' WINCH; echo ready; while :; do sleep 1; done"
    };
    CHECK(
        !pty.spawn( { .program = shell(), .arguments = arguments, .columns = 33, .rows = 11 } ) );
    Terminal terminal = terminal_of();
    CHECK( pump( pty, terminal,
                 []( const std::string &text ) { return contains( text, "ready" ); } ) );
    CHECK( contains( screen_text( terminal ), "11 33" ) );
    pty.resize( 50, 7 );
    CHECK(
        pump( pty, terminal, []( const std::string &text ) { return contains( text, "7 50" ); } ) );
    const auto exit = wait_exit( pty );
    CHECK( exit && exit->code == 0 );
}

void keys_and_ctrl_c()
{
    // A line typed, with the terminal's own echo.
    Pty pty;
    const std::array<std::string_view, 2> arguments{ "-c",
                                                     "read line; printf 'got:%s\\n' \"$line\"" };
    CHECK( !pty.spawn( { .program = shell(), .arguments = arguments } ) );
    CHECK( pty.write( "abc\r" ) == 4 );
    Terminal terminal = terminal_of();
    CHECK( pump( pty, terminal,
                 []( const std::string &text ) { return contains( text, "got:abc" ); } ) );
    CHECK( contains( screen_text( terminal ), "abc\ngot:abc" ) );
    CHECK( wait_exit( pty ) == ( ProcessExit{ 0, 0 } ) );

    // Ctrl-C ends a program waiting in the foreground.
    Pty sleeper;
    const std::array<std::string_view, 2> sleep{ "-c", "echo ready; exec sleep 30" };
    CHECK( !sleeper.spawn( { .program = shell(), .arguments = sleep } ) );
    Terminal other = terminal_of();
    CHECK( pump( sleeper, other,
                 []( const std::string &text ) { return contains( text, "ready" ); } ) );
    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) ); // sleep has started
    CHECK( sleeper.write( "\x03" ) == 1 );
    const auto exit = wait_exit( sleeper );
    CHECK( exit && exit->signal == SIGINT );
}

void cannot_run()
{
    Pty pty;
    CHECK( !pty.spawn( { .program = "/no/such/program" } ) );
    Terminal terminal = terminal_of( 60, 5 );
    CHECK( pump( pty, terminal,
                 []( const std::string &text ) { return contains( text, "not found" ); } ) );
    CHECK( contains( screen_text( terminal ), "pty: cannot run /no/such/program: not found" ) );
    const auto exit = wait_exit( pty );
    CHECK( exit && exit->code == 127 );

    // A name not on PATH: the same, from the child.
    Pty missing;
    CHECK( !missing.spawn( { .program = "sq-no-such-program" } ) );
    Terminal said = terminal_of( 60, 5 );
    CHECK( pump( missing, said,
                 []( const std::string &text ) { return contains( text, "not found" ); } ) );
    CHECK( contains( screen_text( said ), "pty: cannot run sq-no-such-program: not found" ) );
    CHECK( wait_exit( missing ) == ( ProcessExit{ 127, 0 } ) );

    // Refused before anything is made.
    Pty none;
    CHECK( none.spawn( {} ).code == PtyErrorCode::no_program );
    CHECK( none.read( std::span<std::byte>{} ) == 0 );
    CHECK( !none.poll_exit() );
    CHECK( none.write( "x" ) == 0 );
    CHECK( ( PtyError{ PtyErrorCode::fork, 0 }.what() == "could not make a process" ) );

    // One program at a time.
    Pty busy;
    const std::array<std::string_view, 2> wait{ "-c", "sleep 5" };
    CHECK( !busy.spawn( { .program = shell(), .arguments = wait } ) );
    CHECK( busy.spawn( { .program = shell(), .arguments = wait } ).code ==
           PtyErrorCode::already_running );
}

void full_pty()
{
    // A program that reads nothing, its terminal not in line mode (in line
    // mode the line discipline drops what does not fit, so a write never
    // waits): a big paste stops at the timeout.
    Pty pty;
    const std::array<std::string_view, 2> arguments{
        "-c", "stty -icanon -echo; echo ready; exec sleep 30"
    };
    CHECK( !pty.spawn( { .program = shell(), .arguments = arguments } ) );
    Terminal terminal = terminal_of();
    CHECK( pump( pty, terminal,
                 []( const std::string &text ) { return contains( text, "ready" ); } ) );
    const std::string paste( 1 << 20, 'x' );
    const auto start = std::chrono::steady_clock::now();
    const std::size_t written = pty.write( paste, 200 );
    const auto took = std::chrono::steady_clock::now() - start;
    CHECK( written < paste.size() );
    CHECK( took >= std::chrono::milliseconds( 150 ) );
    CHECK( took < std::chrono::seconds( 2 ) );
}

void hang_up()
{
    // A program that ignores SIGHUP is killed; nothing is left behind.
    Pty pty;
    const std::array<std::string_view, 2> arguments{
        "-c", "trap '' HUP; echo ready; while :; do sleep 1; done"
    };
    CHECK( !pty.spawn( { .program = shell(), .arguments = arguments } ) );
    Terminal terminal = terminal_of();
    CHECK( pump( pty, terminal,
                 []( const std::string &text ) { return contains( text, "ready" ); } ) );
    const int pid = pty.pid();
    const auto start = std::chrono::steady_clock::now();
    pty.hang_up();
    CHECK( std::chrono::steady_clock::now() - start < std::chrono::seconds( 2 ) );
    CHECK( pty.fd() == -1 && pty.closed() );
    CHECK( ::kill( pid, 0 ) != 0 && errno == ESRCH );
    const auto exit = pty.poll_exit();
    CHECK( exit && exit->signal == SIGKILL );

    // One that ends on SIGHUP, by destruction; and a moved-from one is empty.
    int quiet = 0;
    {
        Pty first;
        const std::array<std::string_view, 2> wait{ "-c", "sleep 30" };
        CHECK( !first.spawn( { .program = shell(), .arguments = wait } ) );
        Pty second = std::move( first );
        CHECK( first.pid() == 0 && first.fd() == -1 ); // NOLINT(bugprone-use-after-move)
        quiet = second.pid();
    }
    CHECK( ::kill( quiet, 0 ) != 0 && errno == ESRCH );
}

} // namespace

int main()
{
    output_and_exit();
    found_on_path();
    environment_and_directory();
    size_and_resize();
    keys_and_ctrl_c();
    cannot_run();
    full_pty();
    hang_up();
    if( failures > 0 ) {
        std::fprintf( stderr, "pty_test: %d failed\n", failures );
        return 1;
    }
    std::printf( "pty_test: ok\n" );
    return 0;
}
