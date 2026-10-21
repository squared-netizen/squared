// Virtualized lists (6.5), both ends without a Telegraph: the model's list
// (what it sends for a view, after a scroll, a sort, a change, a shrink, no
// allocation), the widget's window (when it asks), and the two together
// through a stand-in, a fling and a sort with every row on screen checked.

#include <squared/messaging/list_window.hpp>
#include <squared/messaging/model_list.hpp>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <numeric>
#include <random>
#include <vector>

namespace
{
std::size_t allocations = 0;
} // namespace

void *operator new( std::size_t size )
{
    ++allocations;
    void *p = std::malloc( size == 0 ? 1 : size ); // NOLINT(cppcoreguidelines-no-malloc)
    if( p == nullptr ) {
        std::abort();
    }
    return p;
}
void operator delete( void *p ) noexcept
{
    std::free( p ); // NOLINT(cppcoreguidelines-no-malloc)
}
void operator delete( void *p, std::size_t /*size*/ ) noexcept
{
    std::free( p ); // NOLINT(cppcoreguidelines-no-malloc)
}

using namespace sq::messaging;

namespace
{

// A test fails loudly, with its line, in any build.
#define CHECK( condition )                                                                         \
    do {                                                                                           \
        if( !( condition ) ) {                                                                     \
            std::fprintf( stderr, "list_test.cpp:%d: %s\n", __LINE__, #condition );                \
            std::abort();                                                                          \
        }                                                                                          \
    } while( false )

constexpr Name inventory{ "inventory" };
constexpr Name inventory_count{ "inventory.count" };
constexpr Name item_name{ "item.name" };

// The test's Telegraph: what a list sends, and the fields its source publishes.
struct Wire
{
        std::vector<Telegram> sent;
        std::size_t rows = 0;      // `state inventory[row] uid`
        std::size_t counts = 0;    // `state inventory.count n`
        std::size_t published = 0; // fields
        bool recording = true;     // false: count only, allocate nothing

        void state( Address to, std::int64_t value ) noexcept
        {
            ++published;
            if( recording ) {
                sent.push_back( Telegram::integer( Verb::state, to, value ) );
            }
        }
        void clear() noexcept
        {
            sent.clear();
            rows = counts = published = 0;
        }
};

ListOutput output( Wire &wire ) noexcept
{
    return { &wire, []( void *context, const Telegram &telegram ) noexcept {
                auto &w = *static_cast<Wire *>( context );
                if( telegram.address().name == inventory_count ) {
                    ++w.counts;
                } else {
                    ++w.rows;
                }
                if( w.recording ) {
                    w.sent.push_back( telegram );
                }
            } };
}

// A model's list: items by uid, in an order.
struct Inventory
{
        std::vector<std::uint32_t> order; // the uid at each row

        [[nodiscard]] std::uint32_t count() const
        {
            return static_cast<std::uint32_t>( order.size() );
        }
        [[nodiscard]] std::uint32_t uid_at( std::uint32_t row ) const
        {
            CHECK( row < order.size() );
            return order[row];
        }
        // An item's one field: its name, here the uid times 10.
        void publish( Wire &wire, std::uint32_t uid ) const
        {
            wire.state( { item_name, uid }, std::int64_t{ uid } * 10 );
        }
};

static_assert( ListRows<Inventory, Wire> );
struct NoUid
{
        [[nodiscard]] std::uint32_t count() const { return 0; }
        void publish( Wire &, std::uint32_t ) {}
};
static_assert( !ListRows<NoUid, Wire> ); // a missing member does not compile

Inventory items( std::uint32_t n )
{
    Inventory inv;
    inv.order.resize( n );
    std::iota( inv.order.begin(), inv.order.end(), 1U );
    return inv;
}

// ---- the model's end ------------------------------------------------------------------------
void model_list()
{
    Inventory inv = items( 300 );
    std::array<std::uint32_t, 128> window{};
    std::array<std::uint32_t, 128> scratch{};
    ModelList list( inventory, list_source<Wire>( inv ), window, scratch );
    Wire wire;
    const ListOutput out = output( wire );

    list.start( out );
    CHECK( wire.counts == 1 && wire.sent.size() == 1 );
    CHECK( wire.sent[0].address() == ( Address{ inventory_count, 0 } ) );
    CHECK( wire.sent[0].as_integer() == 300 );

    // Rows 0 to 29: each row's uid, each item's field.
    wire.clear();
    list.view( 0, 29, out );
    CHECK( wire.rows == 30 && wire.published == 30 );
    CHECK( wire.sent[0].address() == ( Address{ inventory, 0 } ) );
    CHECK( wire.sent[0].as_integer() == 1 );
    CHECK( list.first() == 0 && list.rows() == 30 );

    // The same view again: nothing.
    wire.clear();
    list.view( 0, 29, out );
    CHECK( wire.sent.empty() );

    // Scrolled ten rows: only rows 30 to 39, and their items.
    list.view( 10, 39, out );
    CHECK( wire.rows == 10 && wire.published == 10 );
    for( const Telegram &t : wire.sent ) {
        CHECK( t.address().name == item_name || t.address().uid >= 30 );
    }

    // Changed, but nothing did: nothing.
    wire.clear();
    list.changed( out );
    CHECK( wire.sent.empty() );

    // The view's items in reverse: every row again, no fields (all known).
    std::reverse( inv.order.begin() + 10, inv.order.begin() + 40 );
    list.changed( out );
    CHECK( wire.rows == 30 && wire.published == 0 );

    // A sort of everything: rows whose item changed; fields of items new to the view.
    std::mt19937 random( 6 ); // NOLINT(cert-msc32-c,cert-msc51-cpp): repeatable on purpose
    const std::vector<std::uint32_t> before( inv.order.begin() + 10, inv.order.begin() + 40 );
    std::shuffle( inv.order.begin(), inv.order.end(), random );
    std::size_t moved = 0;
    std::size_t fresh = 0;
    for( std::uint32_t row = 10; row < 40; ++row ) {
        moved += inv.order[row] != before[row - 10] ? 1 : 0;
        fresh += std::find( before.begin(), before.end(), inv.order[row] ) == before.end() ? 1 : 0;
    }
    wire.clear();
    list.changed( out );
    CHECK( wire.rows == moved && wire.published == fresh && wire.counts == 0 );

    // One item: published again only when it is on screen.
    wire.clear();
    list.changed( inv.order[20], out );
    CHECK( wire.published == 1 );
    list.changed( inv.order[200], out );
    CHECK( wire.published == 1 );

    // Shrunk to 15 rows: the count, and the view cut to rows 10 to 14.
    inv.order.resize( 15 );
    wire.clear();
    list.changed( out );
    CHECK( wire.counts == 1 && list.first() == 10 && list.rows() == 5 );
    // A view past the end is cut; a view of nothing sends nothing.
    list.view( 12, 100, out );
    CHECK( list.rows() == 3 );
    wire.clear();
    list.view( 40, 50, out );
    CHECK( list.rows() == 0 && wire.sent.empty() );

    // At most the window's size: 128 rows.
    Inventory big = items( 10000 );
    ModelList long_list( inventory, list_source<Wire>( big ), window, scratch );
    long_list.start( out );
    wire.clear();
    long_list.view( 0, 999, out );
    CHECK( long_list.rows() == 128 && wire.rows == 128 && wire.published == 128 );
    std::puts( "model list: a view, a scroll, a reverse, a sort, one item, a shrink: only what "
               "changed on screen is sent" );
}

void no_allocation()
{
    Inventory inv = items( 10000 );
    std::array<std::uint32_t, 128> window{};
    std::array<std::uint32_t, 128> scratch{};
    ModelList list( inventory, list_source<Wire>( inv ), window, scratch );
    ListWindow widget( 128 );
    Wire wire;
    wire.recording = false;
    const ListOutput out = output( wire );
    list.start( out );
    widget.set_count( list.count() );
    const std::size_t before = allocations;
    for( std::uint32_t frame = 0; frame < 2000; ++frame ) {
        const std::uint32_t first = ( frame * 37 ) % 9900;
        if( const auto ask = widget.visible( first, 30 ) ) {
            list.view( ask->first, ask->last(), out );
        }
        if( frame % 50 == 0 ) {
            std::swap( inv.order[first], inv.order[first + 5] );
            list.changed( out );
            list.changed( inv.order[first], out );
        }
    }
    CHECK( allocations == before );
    std::puts( "no allocation: 2,000 frames of asking, answering and changing" );
}

// ---- the widget's end -----------------------------------------------------------------------
void list_window()
{
    ListWindow window( 128 );
    CHECK( !window.visible( 0, 30 ) ); // no count yet: nothing to ask

    window.set_count( 10000 );
    auto ask = window.visible( 0, 30 );
    CHECK( ask && *ask == ( RowSpan{ 0, 60 } ) ); // the screen and one more below
    CHECK( !window.visible( 0, 30 ) );            // the same: nothing

    // A slow scroll, a row a frame: always covered, asked about every half screen.
    int asks = 0;
    for( std::uint32_t first = 1; first <= 1000; ++first ) {
        if( window.visible( first, 30 ) ) {
            ++asks;
        }
        const RowSpan w = window.window();
        CHECK( w.first <= first && first + 30 <= w.first + w.rows );
    }
    CHECK( asks >= 60 && asks <= 70 ); // 1,000 rows, every 15
    // Back up a little: still inside, nothing asked.
    CHECK( !window.visible( 995, 30 ) );

    // A fling, 100 rows a frame: asked every frame, once, covered.
    for( std::uint32_t first = 1100; first < 3000; first += 100 ) {
        CHECK( window.visible( first, 30 ) );
        const RowSpan w = window.window();
        CHECK( w.first <= first && first + 30 <= w.first + w.rows && w.rows <= 128 );
    }

    // Rotated: fewer rows on screen, a new window.
    CHECK( window.visible( 2900, 15 ) && window.window() == ( RowSpan{ 2885, 45 } ) );

    // At the end of the list: the window ends there, and scrolling there asks nothing more.
    ask = window.visible( 9970, 30 );
    CHECK( ask && ask->first + ask->rows == 10000 );
    for( std::uint32_t first = 9960; first <= 9970; ++first ) {
        CHECK( !window.visible( first, 30 ) );
    }

    // The count changes: asked again.
    window.set_count( 9000 );
    ask = window.visible( 8970, 30 );
    CHECK( ask && ask->first + ask->rows == 9000 );

    // More on screen than a window holds: the window's size, no margin.
    ListWindow small( 16 );
    small.set_count( 100 );
    CHECK( small.visible( 0, 40 ) && small.window() == ( RowSpan{ 0, 16 } ) );
    std::puts( "list window: a slow scroll asks every half screen, a fling once a frame, the end "
               "and the count respected" );
}

// ---- the two together ---------------------------------------------------------------------------
// The UI's view of the list: rows by position and fields by uid, filled only
// from what the model sent (the latest-value cache stands in as two maps).
struct Screen
{
        std::map<std::uint32_t, std::uint32_t> row_uid;
        std::map<std::uint32_t, std::int64_t> name;
        std::uint32_t count = 0;

        void take( const Wire &wire )
        {
            for( const Telegram &t : wire.sent ) {
                if( t.address().name == inventory_count ) {
                    count = static_cast<std::uint32_t>( t.as_integer() );
                } else if( t.address().name == inventory ) {
                    row_uid[t.address().uid] = static_cast<std::uint32_t>( t.as_integer() );
                } else {
                    name[t.address().uid] = t.as_integer();
                }
            }
        }
};

void together()
{
    Inventory inv = items( 10000 );
    std::array<std::uint32_t, 128> window{};
    std::array<std::uint32_t, 128> scratch{};
    ModelList list( inventory, list_source<Wire>( inv ), window, scratch );
    ListWindow widget( 128 );
    Wire wire;
    const ListOutput out = output( wire );
    Screen screen;
    std::size_t telegrams = 0;

    const auto frame = [&]( std::uint32_t first ) {
        wire.clear();
        widget.set_count( screen.count );
        if( const auto ask = widget.visible( first, 30 ) ) {
            list.view( ask->first, ask->last(), out ); // the round trip, at once
        }
        screen.take( wire );
        telegrams += wire.sent.size();
        // Every row on screen has its item, and the item its field.
        for( std::uint32_t row = first; row < std::min( first + 30, inv.count() ); ++row ) {
            CHECK( screen.row_uid.count( row ) == 1 && screen.row_uid[row] == inv.order[row] );
            CHECK( screen.name.count( inv.order[row] ) == 1 );
            CHECK( screen.name[inv.order[row]] == std::int64_t{ inv.order[row] } * 10 );
        }
    };

    wire.clear();
    list.start( out );
    screen.take( wire );
    std::uint32_t frames = 0;
    for( std::uint32_t first = 0; first <= 9000; first += 150, ++frames ) { // a fling down
        frame( first );
    }
    const std::size_t fling = telegrams;
    const std::uint32_t fling_frames = frames;
    // A sort while on screen: the rows that changed, the items new to the view.
    std::mt19937 random( 65 ); // NOLINT(cert-msc32-c,cert-msc51-cpp): repeatable on purpose
    std::shuffle( inv.order.begin(), inv.order.end(), random );
    wire.clear();
    list.changed( out );
    screen.take( wire );
    frame( 9000 );
    for( std::uint32_t first = 9000; first >= 1000; first -= 40, ++frames ) { // and back up
        frame( first );
    }
    std::printf( "together: a fling to row 9,000 (%u frames, %zu telegrams), a sort, and back "
                 "(%u frames in all); every row on screen filled each frame\n",
                 fling_frames, fling, frames );
}

} // namespace

int main()
{
    static_assert( sizeof( ModelList ) == 96 && sizeof( ListWindow ) == 24 );
    model_list();
    no_allocation();
    list_window();
    together();
    std::puts( "list_test: ok" );
    return 0;
}
