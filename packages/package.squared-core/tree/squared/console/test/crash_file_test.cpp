// The crash file, with real crashes: a child process installs it and dies of
// SIGSEGV, or of abort(); the parent reads what it left. Also: the signal goes
// on to the handler installed before (here a marker; on Android the dev log's,
// then the system's), the report is shown once and kept for `crash`, and the
// handlers come back off.

#include <squared/console/crash_file.hpp>

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace sq::console;

namespace
{

std::string g_marker;

void previous_handler( int, siginfo_t *, void * )
{
    const int fd = ::open( g_marker.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600 );
    if( fd >= 0 ) {
        static_cast<void>( ::write( fd, "previous", 8 ) );
        ::close( fd );
    }
    // Back to the default: returning re-runs the faulting instruction, which
    // now kills the process the ordinary way.
    ::signal( SIGSEGV, SIG_DFL );
}

std::vector<std::string> lines_of( const std::string &path )
{
    std::ifstream in( path );
    std::vector<std::string> lines;
    for( std::string line; std::getline( in, line ); ) {
        lines.push_back( line );
    }
    return lines;
}

bool exists( const std::string &path )
{
    return ::access( path.c_str(), F_OK ) == 0;
}

std::vector<std::string> texts( const Console &console )
{
    std::vector<std::string> out;
    for( const Line line : console.lines() ) {
        out.emplace_back( line.text );
    }
    return out;
}

int run_child( void ( *body )() )
{
    const pid_t child = ::fork();
    assert( child >= 0 );
    if( child == 0 ) {
        body();
        ::_exit( 0 ); // not reached
    }
    int status = 0;
    assert( ::waitpid( child, &status, 0 ) == child );
    assert( WIFSIGNALED( status ) );
    return WTERMSIG( status );
}

std::string g_report;

void segfault_child()
{
    struct sigaction before = {};
    before.sa_sigaction = previous_handler;
    before.sa_flags = SA_SIGINFO;
    sigemptyset( &before.sa_mask );
    ::sigaction( SIGSEGV, &before, nullptr );

    static auto console = std::make_unique<Console>(); // the child dies with it
    console->print( "line one" );
    console->print( "line two", Level::warning );
    assert( install_crash_file( g_report.c_str(), console.get(), 5 ) );
    volatile int *nowhere = nullptr;
    *nowhere = 1; // NOLINT(clang-analyzer-core.NullDereference): the crash under test
}

void abort_child()
{
    static auto console = std::make_unique<Console>();
    console->print( "about to abort" );
    assert( install_crash_file( g_report.c_str(), console.get() ) );
    assert( add_crash_command( *console, "unused" ) == Commands::AddResult::added );
    console->run( "crash abort" ); // the command's own way to crash
}

} // namespace

int main()
{
    const char *tmp = std::getenv( "TMPDIR" );
    std::string dir = std::string( tmp != nullptr ? tmp : "/tmp" ) + "/sq_crash_XXXXXX";
    assert( ::mkdtemp( dir.data() ) != nullptr );
    g_report = dir + "/crash.txt";
    g_marker = dir + "/marker.txt";
    const std::string previous = dir + "/crash-previous.txt";

    // --- SIGSEGV: the report, the log lines, and the handler before ours ---------
    assert( run_child( segfault_child ) == SIGSEGV );
    {
        const std::vector<std::string> report = lines_of( g_report );
        assert( report.size() >= 6 );
        assert( report[0].rfind( "CRASH SIGSEGV (11) code 1 addr 0x0", 0 ) == 0 );
        assert( report[1].rfind( "  #0 pc ", 0 ) == 0 );
        assert( report[2].rfind( "  #1 pc ", 0 ) == 0 ); // the unwinder got past the handler
        std::size_t end = 0;
        while( end < report.size() && report[end] != "END" ) {
            ++end;
        }
        assert( end < report.size() );
        assert( report[end + 1] == "log: the last 5 lines" );
        assert( report[end + 2] == "  line one" && report[end + 3] == "  line two" );
        assert( lines_of( g_marker ) == std::vector<std::string>{ "previous" } );
        std::printf( "crash file: %zu lines; %s | %s | %s\n", report.size(), report[0].c_str(),
                     report[1].c_str(), report[end + 2].c_str() );
    }

    // --- abort(): sent, not faulted; sent again after the report -----------------------
    assert( run_child( abort_child ) == SIGABRT );
    assert( lines_of( g_report ).front().rfind( "CRASH SIGABRT (6)", 0 ) == 0 );

    // --- shown once, then kept for `crash` ------------------------------------------------
    auto console = std::make_unique<Console>();
    assert( show_crash_file( *console, g_report.c_str(), previous.c_str() ) );
    {
        const std::vector<std::string> shown = texts( *console );
        assert( shown[0].rfind( "the last run crashed", 0 ) == 0 );
        assert( shown[1].rfind( "CRASH SIGABRT (6)", 0 ) == 0 );
        assert( shown.back() == "  crash: abort()" );
        for( const Line line : console->lines() ) {
            assert( line.level == Level::error );
        }
    }
    assert( !exists( g_report ) && exists( previous ) );
    assert( !show_crash_file( *console, g_report.c_str(), previous.c_str() ) );

    assert( add_crash_command( *console, previous.c_str() ) == Commands::AddResult::added );
    console->run( "clear" );
    console->run( "crash" );
    assert( texts( *console )[1].rfind( "CRASH SIGABRT (6)", 0 ) == 0 );
    ::unlink( previous.c_str() );
    console->run( "crash" );
    assert( texts( *console ).back().rfind( "crash: no report kept", 0 ) == 0 );

    // --- refusals, and the handlers come back off --------------------------------------
    assert( !install_crash_file( "", console.get() ) );
    const std::string long_path( 600, 'x' );
    assert( !install_crash_file( long_path.c_str(), console.get() ) );
    struct sigaction before = {};
    ::sigaction( SIGSEGV, nullptr, &before );
    assert( install_crash_file( g_report.c_str(), console.get() ) );
    struct sigaction during = {};
    ::sigaction( SIGSEGV, nullptr, &during );
    assert( during.sa_sigaction != before.sa_sigaction );
    remove_crash_file();
    struct sigaction after = {};
    ::sigaction( SIGSEGV, nullptr, &after );
    assert( after.sa_handler == before.sa_handler );

    ::unlink( g_marker.c_str() );
    ::rmdir( dir.c_str() );
    std::printf( "crash file: all assertions passed\n" );
    return 0;
}
