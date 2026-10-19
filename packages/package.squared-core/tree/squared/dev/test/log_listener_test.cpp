// The dev log's listener: what the in-app Console reads the dev log through.
// Lines reach it with their level, with or without a sink open, only on the
// thread that set it, never once cleared; report_allocations() arrives through
// it too. No socket is needed.

#include <squared/dev/dev_log.hpp>
#include <squared/dev/frame_allocations.hpp>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace
{

struct Heard
{
        std::vector<std::string> lines;
        std::vector<sq::dev::LogLevel> levels;
};

void hear( void *context, sq::dev::LogLevel level, std::string_view line ) noexcept
{
    auto *heard = static_cast<Heard *>( context );
    heard->lines.emplace_back( line );
    heard->levels.push_back( level );
}

// Keeps the counted allocation observable: clang elides a new/delete pair
// nothing reads, even at -O1 (it did on the phone, 2026-10-17; g++ does not).
void *volatile g_sink = nullptr;

bool starts_with( const std::string &text, const char *prefix )
{
    return text.rfind( prefix, 0 ) == 0;
}

} // namespace

int main()
{
    Heard heard;
    sq::dev::log_line( "before" ); // no listener yet: nowhere to go
    sq::dev::set_log_listener( hear, &heard );
    sq::dev::log_line( "plain" );
    sq::dev::log_line( "careful", sq::dev::LogLevel::Warning );
    const std::string long_line( 3000, 'x' );
    sq::dev::log_line( long_line, sq::dev::LogLevel::Error );
    assert( heard.lines.size() == 3 );
    assert( heard.lines[0] == "plain" && heard.levels[0] == sq::dev::LogLevel::Info );
    assert( heard.lines[1] == "careful" && heard.levels[1] == sq::dev::LogLevel::Warning );
    assert( heard.lines[2].size() == 3000 && heard.levels[2] == sq::dev::LogLevel::Error );

    // Another thread's lines go to the sink only.
    std::thread other( [] { sq::dev::log_line( "from another thread" ); } );
    other.join();
    assert( heard.lines.size() == 3 );

    // report_allocations(): the summary, then a line per site.
    sq::dev::frame_begin();
    static_cast<void>( sq::dev::frame_end() ); // a frame with nothing
    sq::dev::frame_begin();
    auto kept = std::make_unique<std::uint64_t>( 7 ); // one allocation, one site
    g_sink = kept.get();
    static_cast<void>( sq::dev::frame_end() );
    const std::size_t before = heard.lines.size(); // frame_end() reported the new site
    assert( before >= 4 && starts_with( heard.lines[3], "alloc: site 1 (frame 2):" ) );
    sq::dev::report_allocations();
    assert( heard.lines.size() == before + 3 );
    assert( heard.lines[before] ==
            "alloc: 2 frames counted, 1 allocated: 1 allocations, 8 bytes; most in one frame: 1" );
    assert( starts_with( heard.lines[before + 1], "alloc: operator new holds " ) );
    assert( heard.lines[before + 2] == heard.lines[3] );

    // What operator new holds rises by a megabyte with a megabyte held, and
    // falls back when it is freed.
    const auto held_kb = []( const std::string &line ) {
        return std::stoull( line.substr( std::string( "alloc: operator new holds " ).size() ) );
    };
    const unsigned long long base = held_kb( heard.lines[before + 1] );
    {
        auto megabyte = std::make_unique<char[]>( std::size_t{ 1024 } * 1024 );
        g_sink = megabyte.get();
        sq::dev::report_allocations();
        assert( held_kb( heard.lines[heard.lines.size() - 2] ) >= base + 1024 );
    }
    sq::dev::report_allocations();
    assert( held_kb( heard.lines[heard.lines.size() - 2] ) < base + 1024 );
    const std::size_t reports = heard.lines.size();

    // Cleared: nothing more arrives.
    sq::dev::set_log_listener( nullptr, nullptr );
    sq::dev::log_line( "after" );
    sq::dev::report_allocations();
    assert( heard.lines.size() == reports );

    std::printf( "log listener: all assertions passed\n" );
    return 0;
}
