// The Console without drawing: words, the command table, the input line and
// its history, completion, keys, and running commands. Nothing it does may
// allocate, which the frame-allocation counter checks at the end.

#include <squared/console/console.hpp>
#include <squared/console/copy_command.hpp>
#include <squared/console/file_commands.hpp>
#include <squared/console/system_commands.hpp>
#include <squared/dev/frame_allocations.hpp>

#include <sys/syscall.h>
#include <unistd.h>

#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
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
    assert( add_system_commands( *console ) == 7 ); // the second time, every name is taken
    const auto starts = []( const std::string &text, const char *prefix ) {
        return text.rfind( prefix, 0 ) == 0;
    };
    console->run( "mem" );
    const std::string mem = newest( *console );
    assert( starts( mem, "memory: resident " ) && mem.find( " MB (peak " ) != std::string::npos );
    assert( mem.find( '?' ) == std::string::npos ); // every figure was found
    {
        // The thread's own descriptor directory, the fallback for kernels
        // before 3.17, counts the same table.
        std::array<char, 48> path{};
        std::snprintf( path.data(), path.size(), "/proc/self/task/%ld/fd",
                       ::syscall( SYS_gettid ) );
        assert( ::access( path.data(), R_OK ) == 0 );
    }
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
    // smaps: a total, then kinds largest first; this test binary's own code and
    // the heap are among them.
    console->run( "smaps" );
    {
        std::vector<std::string> rows;
        for( auto it = console->lines().last( 12 ); it != console->lines().end(); ++it ) {
            rows.emplace_back( ( *it ).text );
        }
        std::size_t at = 0;
        while( at < rows.size() && rows[at].rfind( "memory by kind: PSS ", 0 ) != 0 ) {
            ++at;
        }
        assert( at < rows.size() );
        bool code = false;
        bool heap = false;
        for( std::size_t i = at + 1; i < rows.size(); ++i ) {
            assert( starts( rows[i], "  " ) && rows[i].find( " pss " ) != std::string::npos );
            code = code || starts( rows[i], "  this program" );
            heap = heap || starts( rows[i], "  native heap" );
        }
        assert( code && heap );
        std::printf( "smaps:" );
        for( std::size_t i = at; i < rows.size(); ++i ) {
            std::printf( " %s |", rows[i].c_str() );
        }
        std::printf( "\n" );
    }
    console->run( "device" );
    const std::string kernel = newest( *console );
    assert( starts( kernel, "kernel: Linux " ) );
#if defined( __BIONIC__ )
    {
        auto it = console->lines().last( 2 );
        assert( starts( std::string( ( *it ).text ), "device: " ) );
    }
#endif
    // --- copy: all, errors, the last n; the echo left out; a missing clipboard ----------
    {
        struct Clip
        {
                std::string text;
                bool available = true;
                bool accept = true;
                int clips = 0;
        };
        static Clip clip;
        const ClipboardWriter writer{ .context = &clip,
                                      .begin =
                                          []( void *context ) noexcept {
                                              auto &c = *static_cast<Clip *>( context );
                                              c.text.clear();
                                              return c.available;
                                          },
                                      .append =
                                          []( void *context, std::string_view text ) noexcept {
                                              static_cast<Clip *>( context )->text.append( text );
                                          },
                                      .end =
                                          []( void *context ) noexcept {
                                              auto &c = *static_cast<Clip *>( context );
                                              ++c.clips;
                                              return c.accept;
                                          } };
        assert( add_copy_command( *console, writer ) == Commands::AddResult::added );
        console->run( "clear" );
        console->print( "one" );
        console->print( "two", Level::error );
        console->print( "three" );
        console->print( "four", Level::error );
        console->run( "copy" );
        assert( clip.text == "one\ntwo\nthree\nfour" && clip.clips == 1 );
        assert( newest( *console ) == "copied 4 lines, 18 bytes" );
        console->run( "copy errors" );
        // the previous echo and "copied" lines are info, so only the two errors
        assert( clip.text == "two\nfour" );
        assert( newest( *console ) == "copied 2 lines, 8 bytes" );
        console->run( "copy 2" ); // the two lines before this echo
        assert( clip.text == "> copy errors\ncopied 2 lines, 8 bytes" );
        console->run( "copy 1000" ); // more than there are: all of them
        assert( clip.text.rfind( "one\n", 0 ) == 0 );
        console->run( "copy zero" );
        assert( newest_level( *console ) == Level::error && clip.clips == 4 );
        console->run( "copy 1 2" );
        assert( newest_level( *console ) == Level::error );
        clip.available = false;
        console->run( "copy" );
        assert( newest( *console ) == "copy: the clipboard is not available" && clip.clips == 4 );
        clip.available = true;
        clip.accept = false;
        console->run( "copy" );
        assert( newest( *console ) == "copy: the clipboard refused the text" );
        clip.accept = true;
        console->run( "clear" );
        console->run( "copy" ); // nothing but its own echo
        assert( clip.text.empty() && newest( *console ) == "copied 0 lines, 0 bytes" );
    }

    std::printf( "system commands: %s | %s | %s\n", mem.c_str(), fds.c_str(), kernel.c_str() );

    // `apk`: the APK this library came from, extracted or loaded in place;
    // not merely the first .apk mapped (that was framework-res.apk).
    {
        std::array<char, 128> apk{};
        using sq::console::detail::apk_of_library;
        assert( apk_of_library( "/data/app/~~a==/com.example.demo-b==/lib/arm64/libdemo.so",
                                apk.data(), apk.size() ) );
        assert( std::strcmp( apk.data(), "/data/app/~~a==/com.example.demo-b==/base.apk" ) == 0 );
        assert( apk_of_library( "/data/app/x/base.apk!/lib/arm64-v8a/libdemo.so", apk.data(),
                                apk.size() ) );
        assert( std::strcmp( apk.data(), "/data/app/x/base.apk" ) == 0 );
        assert( !apk_of_library( "/usr/bin/console_test", apk.data(), apk.size() ) );
        assert( !apk_of_library( "/data/app/x/lib/arm64/libdemo.so", apk.data(), 8 ) );
    }

    std::printf( "console: all assertions passed  sizeof(Console)=%zu (scrollback %zu)  "
                 "sizeof(InputLine)=%zu  sizeof(Commands)=%zu\n",
                 sizeof( Console ), scrollback_bytes, sizeof( InputLine ), sizeof( Commands ) );
    return 0;
}
