// Snapshots (12.3c): the triple buffer alone on two real threads, every
// snapshot read whole and never older than one read before it, the
// newest always reached; then through the Telegraph by name: nothing
// before the first publish, the newest of several, the bytes kept until
// the next read, room growing, a key never given room, a full set of
// names, the UI woken.

#include <poll.h>
#include <stdio.h>
#include <stdlib.h>

import sq.messaging;

import std;

using namespace sq::messaging;

namespace
{

int failures = 0;

#define CHECK( condition )                                                                         \
    do {                                                                                           \
        if( !( condition ) ) {                                                                     \
            std::fprintf( stderr, "snapshot_test.cpp:%d: %s\n", __LINE__, #condition );            \
            ++failures;                                                                            \
        }                                                                                          \
    } while( false )

// A snapshot of `size` bytes, each the publish's number (mod 251), so a
// torn read (two publishes mixed) shows.
void fill( std::span<std::byte> bytes, std::uint64_t number )
{
    std::fill( bytes.begin(), bytes.end(), static_cast<std::byte>( number % 251 ) );
}

bool whole( std::span<const std::byte> bytes, std::uint64_t number )
{
    return std::ranges::all_of(
        bytes, [number]( std::byte b ) { return b == static_cast<std::byte>( number % 251 ); } );
}

void channel_on_two_threads()
{
    detail::SnapshotChannel channel;
    CHECK( !channel.latest() );
    constexpr std::uint64_t publishes = 200000;
    std::atomic<bool> done{ false };
    std::thread writer{ [&] {
        for( std::uint64_t n = 1; n <= publishes; ++n ) {
            // Sizes vary, so buffers grow while the reader reads.
            const std::size_t size = 64 + ( n * 37 ) % 4000;
            const std::span<std::byte> back = channel.back( size );
            fill( back, n );
            channel.publish( size );
        }
        done.store( true, std::memory_order_release );
    } };
    std::uint64_t last = 0;
    std::uint64_t reads = 0;
    std::uint64_t torn = 0;
    std::uint64_t backwards = 0;
    while( true ) {
        const bool finished = done.load( std::memory_order_acquire );
        if( const auto view = channel.latest() ) {
            ++reads;
            if( view->sequence < last ) {
                ++backwards;
            }
            const std::size_t expected = 64 + ( view->sequence * 37 ) % 4000;
            if( view->bytes.size() != expected || !whole( view->bytes, view->sequence ) ) {
                ++torn;
            }
            last = view->sequence;
        }
        if( finished ) {
            break;
        }
    }
    writer.join();
    CHECK( torn == 0 );
    CHECK( backwards == 0 );
    CHECK( reads > 0 );
    // After the writer is done, the reader reaches the last publish.
    const auto final_view = channel.latest();
    CHECK( final_view && final_view->sequence == publishes );
    std::printf( "snapshot_test: %llu publishes, %llu reads\n",
                 static_cast<unsigned long long>( publishes ),
                 static_cast<unsigned long long>( reads ) );
}

constexpr SnapshotKey screen{ "term.screen" };
constexpr SnapshotKey other{ "doc.view" };

void through_the_telegraph()
{
    auto line = std::make_unique<TelegraphLine>(); // 90 KB: not on the stack
    Telegraph &model = line->model();
    TelegraphUi &ui = line->ui();

    CHECK( !ui.snapshot( screen ) );
    CHECK( !model.publish( screen, 4 ) ); // no room asked for yet

    std::span<std::byte> room = model.snapshot( screen, 16 );
    CHECK( room.size() == 16 );
    CHECK( !ui.snapshot( screen ) ); // named, not yet published
    fill( room, 1 );
    CHECK( model.publish( screen, 16 ) );
    auto view = ui.snapshot( screen );
    CHECK( view && view->sequence == 1 && view->bytes.size() == 16 && whole( view->bytes, 1 ) );

    // Two publishes before the UI looks: it gets the newer, smaller one.
    fill( model.snapshot( screen, 16 ), 2 );
    CHECK( model.publish( screen, 16 ) );
    fill( model.snapshot( screen, 8 ), 3 );
    CHECK( model.publish( screen, 8 ) );
    view = ui.snapshot( screen );
    CHECK( view && view->sequence == 3 && view->bytes.size() == 8 && whole( view->bytes, 3 ) );
    // Asked again with nothing new: the same bytes, still whole.
    const auto again = ui.snapshot( screen );
    CHECK( again && again->sequence == 3 && again->bytes.data() == view->bytes.data() );

    // The UI holds sequence 3 while the model writes on: untouched.
    for( std::uint64_t n = 4; n < 10; ++n ) {
        fill( model.snapshot( screen, 8 ), n );
        CHECK( model.publish( screen, 8 ) );
        CHECK( whole( view->bytes, 3 ) );
    }
    view = ui.snapshot( screen );
    CHECK( view && view->sequence == 9 && whole( view->bytes, 9 ) );

    // Growing: a far larger snapshot.
    room = model.snapshot( screen, 100000 );
    CHECK( room.size() == 100000 );
    fill( room, 10 );
    CHECK( model.publish( screen, 100000 ) );
    view = ui.snapshot( screen );
    CHECK( view && view->bytes.size() == 100000 && whole( view->bytes, 10 ) );

    // A second key is separate.
    fill( model.snapshot( other, 4 ), 77 );
    CHECK( model.publish( other, 4 ) );
    const auto doc = ui.snapshot( other );
    CHECK( doc && doc->sequence == 1 && whole( doc->bytes, 77 ) );
    CHECK( ui.snapshot( screen )->sequence == 10 );

    // A full set of names: room is refused for one more.
    auto full_line = std::make_unique<TelegraphLine>();
    Telegraph &full = full_line->model();
    constexpr std::array<SnapshotKey, 5> keys{ "a.a", "a.b", "a.c", "a.d", "a.e" };
    for( std::size_t i = 0; i < telegraph_size::snapshots && i < keys.size(); ++i ) {
        CHECK( full.snapshot( keys[i], 4 ).size() == 4 );
    }
    if( telegraph_size::snapshots < keys.size() ) {
        CHECK( full.snapshot( keys[telegraph_size::snapshots], 4 ).empty() );
        CHECK( !full.publish( keys[telegraph_size::snapshots], 4 ) );
    }
}

void wakes_the_ui()
{
    auto line = std::make_unique<TelegraphLine>();
    TelegraphUi &ui = line->ui();
    CHECK( ui.wake_fd() >= 0 );
    CHECK( ui.ready_to_sleep() );
    std::ignore = line->model().snapshot( screen, 4 );
    CHECK( line->model().publish( screen, 4 ) );
    // The wake descriptor is readable now.
    pollfd waiting{ ui.wake_fd(), POLLIN, 0 };
    CHECK( ::poll( &waiting, 1, 0 ) == 1 );
    ui.woke();
}

} // namespace

int main()
{
    channel_on_two_threads();
    through_the_telegraph();
    wakes_the_ui();
    if( failures > 0 ) {
        std::fprintf( stderr, "snapshot_test: %d failed\n", failures );
        return 1;
    }
    std::printf( "snapshot_test: ok\n" );
    return 0;
}
