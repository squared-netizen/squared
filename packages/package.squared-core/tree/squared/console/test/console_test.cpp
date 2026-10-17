// The Console without drawing: words, the command table, the input line and
// its history, completion, keys, and running commands. Nothing it does may
// allocate, which the frame-allocation counter checks at the end.

#include <squared/console/console.hpp>
#include <squared/console/system_commands.hpp>
#include <squared/dev/frame_allocations.hpp>

#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace sq::console;
using sq::app::Key;

namespace
{

std::vector<std::string> texts( const Scrollback &lines )
{
    std::vector<std::string> out;
    for( const Line line : lines ) {
        out.emplace_back( line.text );
    }
    return out;
}

std::string newest( const Console &console )
{
    return std::string( ( *console.lines().last( 1 ) ).text );
}

Level newest_level( const Console &console )
{
    return ( *console.lines().last( 1 ) ).level;
}

struct Spawned
{
        std::string monster;
        long long count = 0;
        int calls = 0;
};

void spawn( Console &console, const Args &args, void *context )
{
    auto *spawned = static_cast<Spawned *>( context );
    spawned->monster = std::string( args.text( 1 ) );
    spawned->count = args.integer( 2, 1 );
    ++spawned->calls;
    console.printf( Level::info, "spawned %lld %s", spawned->count, spawned->monster.c_str() );
}

void quiet( Console &, const Args &, void *context )
{
    ++*static_cast<int *>( context );
}

} // namespace

int main()
{
    // --- words ---------------------------------------------------------------
    {
        Args args;
        assert( args.parse( "  spawn  goblin 3 " ) && args.size() == 3 );
        assert( args.text( 0 ) == "spawn" && args.text( 1 ) == "goblin" && args.text( 2 ) == "3" );
        assert( args.text( 3 ).empty() );
        assert( args.integer( 2 ) == 3 && args.integer( 1, -1 ) == -1 &&
                args.integer( 9, 7 ) == 7 );
        assert( args.parse( "say \"two words\" -12 0x1F 2.5" ) && args.size() == 5 );
        assert( args.text( 1 ) == "two words" );
        assert( args.integer( 2 ) == -12 && args.integer( 3 ) == 31 );
        assert( args.real( 4 ) == 2.5 && args.real( 1, 9.0 ) == 9.0 );
        assert( args.parse( "" ) && args.size() == 0 );
        assert( args.parse( "say \"unclosed" ) && args.text( 1 ) == "unclosed" );
        std::string many = "x";
        for( std::size_t i = 0; i < max_words; ++i ) {
            many += " w";
        }
        assert( !args.parse( many ) && args.size() == 0 );
    }

    // --- the table --------------------------------------------------------------
    {
        Commands commands;
        int calls = 0;
        assert( commands.add( "a", "", quiet, &calls ) == Commands::AddResult::added );
        assert( commands.add( "a", "", quiet ) == Commands::AddResult::duplicate_name );
        assert( commands.add( "", "", quiet ) == Commands::AddResult::invalid_name );
        assert( commands.add( "two words", "", quiet ) == Commands::AddResult::invalid_name );
        assert( commands.add( "b", "", nullptr ) == Commands::AddResult::invalid_name );
        static char names[command_slots][8];
        std::size_t added = 1;
        for( std::size_t i = 0; i < command_slots; ++i ) {
            std::snprintf( names[i], sizeof( names[i] ), "c%zu", i );
            if( commands.add( names[i], "", quiet ) == Commands::AddResult::added ) {
                ++added;
            }
        }
        assert( added == command_slots && commands.all().size() == command_slots );
        assert( commands.add( "late", "", quiet ) == Commands::AddResult::table_full );
        assert( commands.find( "a" ) != nullptr && commands.find( "nope" ) == nullptr );
    }

    // --- the input line -----------------------------------------------------------
    {
        InputLine input;
        input.insert( "héllo" ); // é is two bytes
        assert( input.text() == "héllo" && input.cursor() == 6 );
        input.left();
        input.left();
        input.left();
        input.left(); // before é
        assert( input.cursor() == 1 );
        input.erase_after(); // removes é whole
        assert( input.text() == "hllo" );
        input.insert( "e" );
        assert( input.text() == "hello" && input.cursor() == 2 );
        input.end();
        input.erase_before();
        assert( input.text() == "hell" );
        input.home();
        input.erase_before(); // nothing before
        input.insert( "a\tb\x01"
                      "c" );
        assert( input.text() == "abchell" ); // control characters dropped

        // Cut at a character boundary when full.
        input.clear();
        std::string full( input_bytes - 1, 'x' );
        input.insert( full );
        input.insert( "é" ); // two bytes; one fits; neither is taken
        assert( input.text().size() == input_bytes - 1 );
        input.insert( "y" );
        assert( input.text().size() == input_bytes && input.text().back() == 'y' );

        // History: Up and Down, the draft kept.
        input.clear();
        input.set( "first" );
        input.remember();
        input.set( "second" );
        input.remember();
        input.set( "second" );
        input.remember(); // same as the newest: not added twice
        assert( input.history().size() == 2 );
        input.set( "draft" );
        input.previous();
        assert( input.text() == "second" );
        input.previous();
        assert( input.text() == "first" );
        input.previous(); // no further
        assert( input.text() == "first" );
        input.next();
        assert( input.text() == "second" );
        input.next();
        assert( input.text() == "draft" );
        input.next();
        assert( input.text() == "draft" );
    }

    // --- the Console -----------------------------------------------------------------
    auto console = std::make_unique<Console>(); // about 68 KB: never on the stack
    Spawned spawned;
    assert( console->add( "spawn", "spawn <monster> [count]", spawn, &spawned ) ==
            Commands::AddResult::added );
    int quiet_calls = 0;
    assert( console->add( "spam", "", quiet, &quiet_calls ) == Commands::AddResult::added );

    console->run( "spawn goblin 3" );
    assert( spawned.calls == 1 && spawned.monster == "goblin" && spawned.count == 3 );
    assert( newest( *console ) == "spawned 3 goblin" );
    assert( ( texts( console->lines() ).front() == "> spawn goblin 3" ) );

    console->run( "dance" );
    assert( newest( *console ) == "unknown command: dance (help lists them)" );
    assert( newest_level( *console ) == Level::error );

    console->run( "help" );
    assert( newest( *console ).rfind( "spam", 0 ) == 0 );
    console->run( "help spawn" );
    assert( newest( *console ) == "spawn  spawn <monster> [count]" );
    console->run( "help nothing" );
    assert( newest_level( *console ) == Level::error );

    console->run( "clear" );
    assert( console->lines().empty() );

    // Typing, completing and submitting through keys.
    console->text( "spaw" );
    assert( console->complete() == 1 && console->input().text() == "spawn " );
    console->text( "orc" );
    assert( console->key( Key::enter, {} ) );
    assert( spawned.calls == 2 && spawned.monster == "orc" && spawned.count == 1 );
    assert( console->input().text().empty() && console->input().history().size() == 1 );

    console->text( "sp" );
    assert( console->complete() == 2 && console->input().text() == "spa" ); // the common part
    assert( newest( *console ) == "spawn spam" );
    console->key( Key::escape, {} );
    assert( console->input().text().empty() );

    assert( console->key( Key::up, {} ) && console->input().text() == "spawn orc" );
    console->key( Key::backspace, {} );
    console->key( Key::backspace, {} );
    console->key( Key::backspace, {} );
    console->text( "imp" );
    console->key( Key::enter, {} );
    assert( spawned.monster == "imp" );
    assert( !console->key( Key::space, {} ) && !console->key( Key::unknown, {} ) );

    // printf cuts at the buffer, never overruns it.
    const std::string huge( 2000, 'z' );
    console->printf( Level::warning, "%s", huge.c_str() );
    assert( newest( *console ).size() == format_bytes - 1 &&
            newest_level( *console ) == Level::warning );

    // Nothing allocates: running commands, printing, editing, completing.
    sq::dev::frame_begin();
    for( int i = 0; i < 200; ++i ) {
        console->run( "spam once more" );
        console->printf( Level::info, "line %d", i );
        console->text( "sp" );
        static_cast<void>( console->complete() );
        console->key( Key::escape, {} );
        console->text( "spam" );
        console->key( Key::enter, {} );
    }
    const sq::dev::FrameAllocations used = sq::dev::frame_end();
    assert( used.count == 0 && quiet_calls == 400 );

    // --- shown, and the close built-in ------------------------------------------------
    assert( !console->shown() );
    console->show();
    assert( console->shown() );
    console->run( "close" );
    assert( !console->shown() );
    console->show();
    console->hide();
    assert( !console->shown() );

    // --- the system commands: real /proc, real libc ----------------------------------------
    assert( add_system_commands( *console ) == 0 );
    assert( add_system_commands( *console ) == 5 ); // the second time, every name is taken
    const auto starts = []( const std::string &text, const char *prefix ) {
        return text.rfind( prefix, 0 ) == 0;
    };
    console->run( "mem" );
    const std::string mem = newest( *console );
    assert( starts( mem, "memory: resident " ) && mem.find( " MB (peak " ) != std::string::npos );
    assert( mem.find( '?' ) == std::string::npos ); // every figure was found
    console->run( "heap" );
    assert( starts( newest( *console ), "heap: " ) &&
            newest( *console ).find( " MB mapped" ) != std::string::npos );
    console->run( "threads" );
    assert( newest( *console ) == "threads: 1" );
    console->run( "fds" );
    const std::string fds = newest( *console );
    assert( starts( fds, "open files: " ) && std::stoi( fds.substr( 12 ) ) >= 3 );
    const int open_before = std::stoi( fds.substr( 12 ) );
    {
        const int extra = ::dup( 0 );
        console->run( "fds" );
        assert( std::stoi( newest( *console ).substr( 12 ) ) == open_before + 1 );
        ::close( extra );
    }
    console->run( "device" );
    assert( starts( newest( *console ), "kernel: Linux " ) );
#if defined( __BIONIC__ )
    {
        auto it = console->lines().last( 2 );
        assert( starts( std::string( ( *it ).text ), "device: " ) );
    }
#endif
    std::printf( "system commands: %s | %s | %s\n", mem.c_str(), fds.c_str(),
                 newest( *console ).c_str() );

    std::printf( "console: all assertions passed  sizeof(Console)=%zu (scrollback %zu)  "
                 "sizeof(InputLine)=%zu  sizeof(Commands)=%zu\n",
                 sizeof( Console ), scrollback_bytes, sizeof( InputLine ), sizeof( Commands ) );
    return 0;
}
