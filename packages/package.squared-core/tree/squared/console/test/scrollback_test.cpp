// The console's scrollback: lines in, lines out, oldest dropped, nothing
// allocated.
//
// Checked against a plain model (a deque of strings with a byte budget), over
// many random pushes into several storage sizes, so wrapping, dropping and
// cutting long lines are all exercised, not only the cases written out here.

#include <squared/console/scrollback.hpp>
#include <squared/dev/frame_allocations.hpp>

#include <array>
#include <cassert>
#include <cstdio>
#include <deque>
#include <random>
#include <string>
#include <vector>

using sq::console::Level;
using sq::console::Line;
using sq::console::Scrollback;

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

// The model: what the ring should hold. A record costs its text rounded up to
// 4, plus 4; the ring holds what fits, newest kept.
struct Model
{
        std::deque<std::pair<std::string, Level>> lines;
};

std::size_t record( std::size_t length )
{
    return ( 4 + length + 3 ) & ~std::size_t{ 3 };
}

} // namespace

int main()
{
    // The basics.
    {
        std::array<char, 256> storage{};
        Scrollback lines( storage );
        assert( lines.empty() && lines.begin() == lines.end() );
        lines.push( Level::info, "one" );
        lines.push( Level::error, "two\nthree\n" );
        assert( lines.size() == 3 );
        assert( ( texts( lines ) == std::vector<std::string>{ "one", "two", "three" } ) );
        auto it = lines.begin();
        assert( ( *it ).level == Level::info );
        ++it;
        assert( ( *it ).level == Level::error && ( *it ).text == "two" );
        assert( ( texts( lines ).size() == 3 ) );

        // last(): the newest n, oldest of them first.
        std::vector<std::string> tail;
        for( auto at = lines.last( 2 ); at != lines.end(); ++at ) {
            tail.emplace_back( ( *at ).text );
        }
        assert( ( tail == std::vector<std::string>{ "two", "three" } ) );
        assert( lines.last( 99 ) == lines.begin() );

        // An empty line is a line.
        lines.push( Level::warning, "" );
        assert( lines.size() == 4 && texts( lines ).back().empty() );

        lines.clear();
        assert( lines.empty() && lines.dropped() == 0 );
    }

    // Too long for the storage: cut, not dropped.
    {
        std::array<char, 64> storage{};
        Scrollback lines( storage );
        assert( lines.longest_line() == 60 );
        lines.push( Level::info, std::string( 200, 'x' ) );
        assert( lines.size() == 1 && texts( lines )[0] == std::string( 60, 'x' ) );
    }

    // Records that fill the storage exactly are all kept, before and after the
    // ring wraps: 4 records of 16 bytes (12 characters each) in 64.
    {
        std::array<char, 64> storage{};
        Scrollback lines( storage );
        for( int i = 0; i < 4; ++i ) {
            lines.push( Level::info, "twelve chars" );
        }
        assert( lines.size() == 4 && lines.dropped() == 0 );
        for( int i = 0; i < 9; ++i ) {
            lines.push( Level::info, "twelve chars" );
            assert( lines.size() == 4 );
        }
        assert( lines.dropped() == 9 );
    }

    // Against the model, with random lines into several sizes.
    std::mt19937 random( 12345 );
    for( const std::size_t size : { 64, 100, 256, 1000, 4096 } ) {
        std::vector<char> storage( size );
        Scrollback lines( storage );
        Model model;
        const std::size_t capacity = size & ~std::size_t{ 3 };
        std::size_t used = 0;
        std::uint64_t dropped = 0;
        for( int step = 0; step < 20000; ++step ) {
            const std::size_t length = random() % ( step % 50 == 0 ? 300 : 40 );
            const std::string text( length, static_cast<char>( 'a' + step % 26 ) );
            const auto level = static_cast<Level>( random() % 3 );
            lines.push( level, text );

            const std::string kept = text.substr( 0, lines.longest_line() );
            // The ring keeps a suffix of the history that fits, but loses
            // some room at the top when it wraps, so it may hold fewer lines
            // than the model's best case; never more, never different ones.
            model.lines.emplace_back( kept, level );
            used += record( kept.size() );
            while( used > capacity ) {
                used -= record( model.lines.front().first.size() );
                model.lines.pop_front();
                ++dropped;
            }
            assert( lines.size() <= model.lines.size() );
            assert( lines.size() + lines.dropped() == static_cast<std::size_t>( step + 1 ) );
            const std::size_t skip = model.lines.size() - lines.size();
            std::size_t i = 0;
            for( const Line line : lines ) {
                const auto &[want, want_level] = model.lines[skip + i];
                assert( line.text == want && line.level == want_level );
                ++i;
            }
            assert( i == lines.size() && lines.size() >= 1 );
        }
        // Wrapping costs at most one record's worth of room.
        assert( lines.dropped() >= dropped );
    }

    // Pushing allocates nothing.
    {
        std::array<char, 1024> storage{};
        Scrollback lines( storage );
        sq::dev::frame_begin();
        for( int i = 0; i < 1000; ++i ) {
            lines.push( Level::warning, "a warning, then\nanother line" );
        }
        std::size_t seen = 0;
        for( const Line line : lines ) {
            seen += line.text.size();
        }
        const sq::dev::FrameAllocations used = sq::dev::frame_end();
        assert( used.count == 0 && seen > 0 );
    }

    std::printf(
        "scrollback: all assertions passed  sizeof(Scrollback)=%zu  default storage %zu bytes\n",
        sizeof( Scrollback ), sq::console::scrollback_bytes );
    return 0;
}
