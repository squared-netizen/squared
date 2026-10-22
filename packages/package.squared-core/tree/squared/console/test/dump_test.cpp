// Groups of command lines and `dump` (6.7a): what a group may hold, the
// listing, queueing one line a frame through update(), combinations run
// once each, `all`, `stop`, unknown groups, the system groups, the table's
// limits, and no allocation.

#include <squared/console/console.hpp>
#include <squared/console/system_commands.hpp>
#include <squared/dev/frame_allocations.hpp>

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

using namespace sq::console;

namespace
{

// A test fails loudly, with its line, in any build.
#define CHECK( condition )                                                                         \
    do {                                                                                           \
        if( !( condition ) ) {                                                                     \
            std::fprintf( stderr, "dump_test.cpp:%d: %s\n", __LINE__, #condition );                \
            std::abort();                                                                          \
        }                                                                                          \
    } while( false )

using Added = Groups::AddResult;

struct Calls
{
        int a = 0;
        int b = 0;
        long long c = 0; // the argument c was given
};

std::vector<std::string> lines_since( const Console &console, std::size_t from )
{
    std::vector<std::string> out;
    std::size_t at = 0;
    for( const Line line : console.lines() ) {
        if( at++ >= from ) {
            out.emplace_back( line.text );
        }
    }
    return out;
}

std::size_t count( const Console &console )
{
    std::size_t lines = 0;
    for( const Line line : console.lines() ) {
        static_cast<void>( line );
        ++lines;
    }
    return lines;
}

void add_commands( Console &console, Calls &calls )
{
    const auto a = []( Console &, const Args &, void *context ) {
        ++static_cast<Calls *>( context )->a;
    };
    const auto b = []( Console &, const Args &, void *context ) {
        ++static_cast<Calls *>( context )->b;
    };
    const auto c = []( Console &, const Args &args, void *context ) {
        static_cast<Calls *>( context )->c = args.integer( 1, -1 );
    };
    CHECK( console.add( "a", "a", a, &calls ) == Commands::AddResult::added );
    CHECK( console.add( "b", "b", b, &calls ) == Commands::AddResult::added );
    CHECK( console.add( "c", "c <n>", c, &calls ) == Commands::AddResult::added );
}

void groups_refuse()
{
    Groups groups;
    CHECK( groups.add( "one", "a" ) == Added::added );
    CHECK( groups.add( "one", "a" ) == Added::added ); // already there: once
    CHECK( groups.lines().size() == 1 );
    CHECK( groups.add( "all", "a" ) == Added::invalid_name );  // dump's word
    CHECK( groups.add( "stop", "a" ) == Added::invalid_name ); // dump's word
    CHECK( groups.add( "two words", "a" ) == Added::invalid_name );
    CHECK( groups.add( "", "a" ) == Added::invalid_name );
    CHECK( groups.add( "one", "" ) == Added::invalid_name );
    CHECK( groups.add( "one", "dump sys" ) == Added::invalid_name ); // would queue itself
    CHECK( groups.add( "one", "dump" ) == Added::invalid_name );
    CHECK( groups.add( "one", "dumpling" ) == Added::added ); // only the word dump
    CHECK( groups.find( "one" ) == 0U && !groups.find( "two" ) );

    // The limits: group_slots names, group_line_slots lines.
    Groups full;
    static const std::vector<std::string> names = [] {
        std::vector<std::string> made;
        for( std::size_t i = 0; i <= group_slots; ++i ) {
            made.push_back( "g" + std::to_string( i ) );
        }
        return made;
    }();
    for( std::size_t i = 0; i < group_slots; ++i ) {
        CHECK( full.add( names[i].c_str(), "a" ) == Added::added );
    }
    CHECK( full.add( names[group_slots].c_str(), "a" ) == Added::groups_full );
    static const std::vector<std::string> lines = [] {
        std::vector<std::string> made;
        for( std::size_t i = 0; i <= group_line_slots; ++i ) {
            made.push_back( "line " + std::to_string( i ) );
        }
        return made;
    }();
    std::size_t added = full.lines().size();
    for( std::size_t i = 0; added < group_line_slots; ++i, ++added ) {
        CHECK( full.add( "g0", lines[i].c_str() ) == Added::added );
    }
    CHECK( full.add( "g0", lines[group_line_slots].c_str() ) == Added::lines_full );
}

void dump_runs_one_a_frame()
{
    auto console = std::make_unique<Console>();
    Calls calls;
    add_commands( *console, calls );
    CHECK( console->add_group( "one", { "a", "b" } ) == Added::added );
    CHECK( console->add_to_group( "two", "b" ) == Added::added );
    CHECK( console->add_to_group( "two", "c 5" ) == Added::added );
    CHECK( console->add_group( "bad", { "a", "dump one" } ) == Added::invalid_name );

    // The listing.
    std::size_t from = count( *console );
    console->run( "dump" );
    std::vector<std::string> printed = lines_since( *console, from );
    CHECK( printed.size() == 5 ); // the echo, the usage, three groups
    CHECK( printed[2] == "  one       a, b" );
    CHECK( printed[3] == "  two       b, c 5" );
    CHECK( printed[4] == "  bad       a" ); // made, with what fitted before the refusal

    // Two groups: b, in both, runs once; nothing runs until update().
    from = count( *console );
    console->run( "dump one two" );
    CHECK( lines_since( *console, from ).back() == "dump: 3 queued, one a frame" );
    CHECK( console->dumping() && calls.a == 0 && calls.b == 0 );
    console->update();
    CHECK( calls.a == 1 && calls.b == 0 );
    console->update();
    CHECK( calls.b == 1 && calls.c == 0 );
    console->update();
    CHECK( calls.c == 5 && !console->dumping() ); // its argument reached it
    printed = lines_since( *console, from );
    CHECK( printed.back() == "dump: done, 3 run" );
    CHECK( printed[2] == "> a" && printed[3] == "> b" && printed[4] == "> c 5" );
    console->update(); // nothing queued: nothing runs
    CHECK( calls.a == 1 && calls.b == 1 && lines_since( *console, from ) == printed );

    // all, a group already queued, stop.
    console->run( "dump all" );
    from = count( *console );
    console->run( "dump one" );
    CHECK( lines_since( *console, from ).size() == 1 ); // only the echo: nothing new to queue
    console->update();
    CHECK( calls.a == 2 );
    from = count( *console );
    console->run( "dump stop" );
    CHECK( lines_since( *console, from ).back() == "dump: stopped; 2 not run" );
    CHECK( !console->dumping() );
    console->update();
    CHECK( calls.b == 1 );

    // An unknown group is an error; the others still run.
    from = count( *console );
    console->run( "dump nothing one" );
    printed = lines_since( *console, from );
    CHECK( printed[1] == "dump: no group named nothing (dump lists them)" );
    CHECK( printed.back() == "dump: 2 queued, one a frame" );
    console->update();
    console->update();
    CHECK( calls.a == 3 && calls.b == 2 && !console->dumping() );

    // A command typed while a dump runs runs at once; the dump goes on.
    console->run( "dump two" );
    console->run( "a" );
    CHECK( calls.a == 4 && console->dumping() );
    console->update();
    console->update();
    CHECK( calls.b == 3 && calls.c == 5 && !console->dumping() );

    // Nothing allocates: queueing, running, listing.
    sq::dev::frame_begin();
    for( int round = 0; round < 100; ++round ) {
        console->run( "dump all" );
        while( console->dumping() ) {
            console->update();
        }
        console->run( "dump" );
    }
    const sq::dev::FrameAllocations used = sq::dev::frame_end();
    CHECK( used.count == 0 );
    CHECK( calls.a == 104 && calls.b == 103 );
}

void system_groups()
{
    auto console = std::make_unique<Console>();
    CHECK( add_system_commands( *console ) == 0 );
    const Groups &groups = console->groups();
    const auto sys = groups.find( "sys" );
    const auto memory = groups.find( "memory" );
    CHECK( sys && memory );
    std::string in_sys;
    std::string in_memory;
    for( const GroupLine &line : groups.lines() ) {
        std::string &joined = line.group == *sys ? in_sys : in_memory;
        joined += joined.empty() ? "" : ",";
        joined += line.line;
    }
    CHECK( in_sys == "mem,heap,threads,fds,device,smaps,libs" );
    CHECK( in_memory == "mem,heap,smaps" );
    console->run( "dump sys memory" ); // memory's lines are all in sys: 7, not 10
    int ran = 0;
    while( console->dumping() ) {
        console->update();
        ++ran;
    }
    CHECK( ran == 7 );
}

} // namespace

int main()
{
    groups_refuse();
    dump_runs_one_a_frame();
    system_groups();
    std::printf( "dump_test: ok (sizeof(Groups) %zu, sizeof(Console) %zu)\n", sizeof( Groups ),
                 sizeof( Console ) );
    return 0;
}
