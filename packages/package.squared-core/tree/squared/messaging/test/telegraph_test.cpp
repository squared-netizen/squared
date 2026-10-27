// The Telegraph, single-threaded (6.6a): both ends through the line, every
// verb and value type, what does not compile, states held back and
// coalesced, the order kept, lists wired in, the lifecycle in order, and no
// allocation. The model runs inline: `deliver` called where its thread will.

#include <squared/messaging/telegraph.hpp>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <string_view>
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
// The line is 64-byte aligned (the rings' counters), so make_unique uses this
// one. posix_memalign, not aligned_alloc: Android's C library has
// aligned_alloc only from API 28, and Termux's clang builds for API 24
// (found on the phone, 2026-10-20).
void *operator new( std::size_t size, std::align_val_t align )
{
    ++allocations;
    void *p = nullptr;
    if( ::posix_memalign( &p, static_cast<std::size_t>( align ), size == 0 ? 1 : size ) != 0 ) {
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
void operator delete( void *p, std::align_val_t /*align*/ ) noexcept
{
    std::free( p ); // NOLINT(cppcoreguidelines-no-malloc)
}
void operator delete( void *p, std::size_t /*size*/, std::align_val_t /*align*/ ) noexcept
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
            std::fprintf( stderr, "telegraph_test.cpp:%d: %s\n", __LINE__, #condition );           \
            std::abort();                                                                          \
        }                                                                                          \
    } while( false )

namespace names
{
inline constexpr Key<bool, state | set> sound{ "settings.sound" };
inline constexpr Key<double, state | set> volume{ "settings.volume" };
inline constexpr Key<std::int64_t, state> ticks{ "counter.ticks" };
inline constexpr Key<std::array<float, 2>, state> pos{ "player.pos" };
inline constexpr Key<std::string_view, do_> open{ "file.open" };
inline constexpr Key<std::string_view, event> opened{ "file.opened" };
inline constexpr Key<void, do_> save{ "doc.save" };
inline constexpr Key<void, event> saved{ "doc.saved" };
inline constexpr Key<std::string_view, state, with_uid> item_name{ "item.name" };
inline constexpr Key<std::int64_t, state, with_uid> inventory{ "inventory" };
inline constexpr Key<void, do_, with_uid> item_use{ "item.use" };
} // namespace names

// What does not compile: a verb the key does not carry, the wrong type, a
// uid where none is taken or none where one is. Asked through templates, so
// a refusal is `false`, not an error.
template<typename K, typename V>
constexpr bool can_state = requires( Telegraph &t, const K &k, const V &v ) { t.state( k, v ); };
template<typename K, typename V>
constexpr bool can_state_uid =
    requires( Telegraph &t, const K &k, std::uint32_t u, const V &v ) { t.state( k, u, v ); };
template<typename K, typename V>
constexpr bool can_event = requires( Telegraph &t, const K &k, const V &v ) { t.event( k, v ); };
template<typename K, typename V>
constexpr bool can_set = requires( TelegraphUi &ui, const K &k, const V &v ) { ui.set( k, v ); };
template<typename K>
constexpr bool can_do = requires( TelegraphUi &ui, const K &k ) { ui.do_( k ); };
template<typename K>
constexpr bool can_value = requires( TelegraphUi &ui, const K &k ) { ui.value( k ); };
template<typename K>
constexpr bool can_value_uid =
    requires( TelegraphUi &ui, const K &k, std::uint32_t u ) { ui.value( k, u ); };

using Sound = decltype( names::sound );
using Ticks = decltype( names::ticks );
using Open = decltype( names::open );
using Saved = decltype( names::saved );
using ItemName = decltype( names::item_name );
using Text = const char *;

static_assert( can_state<Sound, bool> && can_state<Ticks, int> && can_state_uid<ItemName, Text> );
static_assert( !can_state<Open, Text> );                       // open carries do, not state
static_assert( !can_state<Ticks, Text> );                      // an integer, not text
static_assert( !can_state<Ticks, bool> );                      // a bool is not an integer
static_assert( !can_state<ItemName, Text> );                   // item.name needs a uid
static_assert( !can_state_uid<Ticks, int> );                   // counter.ticks takes none
static_assert( !can_event<Saved, int> );                       // doc.saved has no value
static_assert( can_set<Sound, bool> && !can_set<Ticks, int> ); // ticks is the model's
static_assert( !can_do<Open> );                                // file.open needs its text
static_assert( !can_value<ItemName> && can_value_uid<ItemName> );

std::span<const std::byte> bytes_of( std::string_view text ) noexcept
{
    return std::as_bytes( std::span<const char>( text.data(), text.size() ) );
}

std::string text_of( std::span<const std::byte> data )
{
    return { reinterpret_cast<const char *>( data.data() ), data.size() };
}

// A list for the model: items by uid, in an order.
struct Inventory
{
        std::vector<std::uint32_t> order;

        [[nodiscard]] std::uint32_t count() const
        {
            return static_cast<std::uint32_t>( order.size() );
        }
        [[nodiscard]] std::uint32_t uid_at( std::uint32_t row ) const { return order[row]; }
        void publish( Telegraph &t, std::uint32_t uid ) const
        {
            static_cast<void>( t.state( names::item_name, uid, "item" ) );
        }
};

// The model: echoes sound, answers file.open, saves on doc.save, logs its lifecycle.
struct Model
{
        bool sound = true;
        std::vector<std::string> log;
        std::uint32_t used = 0;

        void start( Telegraph &t ) { CHECK( t.state( names::sound, sound ) == SendResult::sent ); }
        void receive( Telegraph &t, const Received &m )
        {
            const Name name = m.telegram.address().name;
            if( name == names::sound.name() ) {
                sound = m.telegram.as_boolean();
                CHECK( t.state( names::sound, sound ) == SendResult::sent );
            } else if( name == names::open.name() ) {
                CHECK( t.answer( m, names::opened, std::string_view( text_of( m.data ) ) ) ==
                       SendResult::sent );
            } else if( name == names::save.name() ) {
                CHECK( t.answer( m, names::saved ) == SendResult::sent );
            } else if( name == names::item_use.name() ) {
                used = m.telegram.address().uid;
            }
        }
        void background( Telegraph & ) { log.emplace_back( "background" ); }
        void foreground( Telegraph &, std::uint64_t elapsed )
        {
            log.push_back( "foreground " + std::to_string( elapsed ) );
        }
        void low_memory( Telegraph & ) { log.emplace_back( "low memory" ); }
};

// A model with nothing but receive: the hooks are optional.
struct Bare
{
        void receive( Telegraph &, const Received & ) {}
};

struct Ui
{
        std::vector<Telegram> events;
        std::vector<std::string> texts;
        void event( const Received &r )
        {
            events.push_back( r.telegram );
            texts.push_back( text_of( r.data ) );
        }
};

// A handler told of each state too, after it is in the cache (what a screen's
// bound widgets are updated from).
struct Showing
{
        std::vector<Telegram> states;
        bool cached = true;
        TelegraphUi *ui = nullptr;
        void state( const Received &r )
        {
            states.push_back( r.telegram );
            cached = cached && ui->find( r.telegram.address() ).has_value();
        }
};

// ---- round trips --------------------------------------------------------------------------
void round_trips()
{
    auto line = std::make_unique<TelegraphLine>();
    Telegraph &t = line->model();
    TelegraphUi &ui = line->ui();
    Model model;
    Ui events;

    t.start( model );
    CHECK( !ui.value( names::sound ) );
    Showing showing;
    showing.ui = &ui;
    CHECK( ui.drain( showing ) == 1 && ui.value( names::sound ) == true );
    CHECK( showing.states.size() == 1 && showing.cached );
    CHECK( showing.states[0].address().name == names::sound.name() );

    // set, through the model and back.
    CHECK( ui.set( names::sound, false ) == SendResult::sent );
    CHECK( ui.value( names::sound ) == true ); // not until the model says so
    CHECK( t.deliver( model ) == 1 && !model.sound );
    CHECK( ui.drain( events ) == 1 && ui.value( names::sound ) == false );

    // do with text, answered with its correlation; do with nothing, twice.
    const DoResult opening = ui.do_( names::open, "notes.txt" );
    CHECK( opening.result == SendResult::sent && opening.correlation == 1 );
    const DoResult saving = ui.do_( names::save );
    CHECK( saving.correlation == 2 );
    CHECK( t.deliver( model ) == 2 );
    CHECK( ui.drain( events ) == 2 && events.events.size() == 2 );
    CHECK( events.events[0].address().name == names::opened.name() );
    CHECK( events.events[0].correlation() == 1 && events.texts[0] == "notes.txt" );
    CHECK( events.events[1].address().name == names::saved.name() );
    CHECK( events.events[1].correlation() == 2 );

    // A row's button: do with a uid.
    CHECK( ui.do_( names::item_use, 17U ).result == SendResult::sent );
    CHECK( t.deliver( model ) == 1 && model.used == 17 );

    // Every value type the model may state, read back as its type.
    CHECK( t.state( names::volume, 0.75 ) == SendResult::sent );
    CHECK( t.state( names::ticks, 42 ) == SendResult::sent );
    CHECK( t.state( names::pos, std::array<float, 2>{ 1.5F, -2.0F } ) == SendResult::sent );
    CHECK( t.state( names::item_name, 17U, "fur coat" ) == SendResult::sent );
    CHECK( t.state( names::item_name, 18U, "a coat with a much longer name" ) == SendResult::sent );
    CHECK( ui.drain() == 5 );
    CHECK( ui.value( names::volume ) == 0.75 && ui.value( names::ticks ) == 42 );
    CHECK( ui.value( names::pos ) == ( std::array<float, 2>{ 1.5F, -2.0F } ) );
    CHECK( ui.value( names::item_name, 17U ) == "fur coat" );
    CHECK( ui.value( names::item_name, 18U ) == "a coat with a much longer name" );
    CHECK( !ui.value( names::item_name, 19U ) );
    std::puts( "round trips: set, do, answer with its correlation, every value type back as its "
               "type" );
}

// ---- states held back -------------------------------------------------------------------------
void held_back()
{
    auto line = std::make_unique<TelegraphLine>();
    Telegraph &t = line->model();
    TelegraphUi &ui = line->ui();
    constexpr auto ring = static_cast<std::int64_t>( telegraph_size::ring );

    // The UI is not draining: the ring fills, then one address coalesces.
    for( std::int64_t i = 0; i < ring; ++i ) {
        CHECK( t.state( names::ticks, i ) == SendResult::sent );
    }
    for( std::int64_t i = ring; i < ring + 1000; ++i ) {
        CHECK( t.state( names::ticks, i ) == SendResult::pending );
    }
    CHECK( t.pending() == 1 );
    // An event would overtake the states before it: refused while they wait.
    CHECK( t.event( names::saved ) == SendResult::full );
    // Text is held too.
    CHECK( t.state( names::item_name, 17U, "a name long enough for the text area" ) ==
           SendResult::pending );
    CHECK( t.pending() == 2 );

    // The UI takes the ring: the first ring's worth.
    CHECK( ui.drain() == ring && ui.value( names::ticks ) == ring - 1 );
    // The model's next turn sends what was held: only the latest.
    Bare bare;
    t.deliver( bare );
    CHECK( t.pending() == 0 );
    CHECK( ui.drain() == 2 && ui.value( names::ticks ) == ring + 999 );
    CHECK( ui.value( names::item_name, 17U ) == "a name long enough for the text area" );
    CHECK( t.event( names::saved ) == SendResult::sent );

    // While a state waits, a newer one for the same address waits behind it;
    // another address may go straight on if there is room.
    for( std::int64_t i = 0; i < ring - 1; ++i ) {
        CHECK( t.state( names::ticks, i ) == SendResult::sent );
    }
    CHECK( t.state( names::ticks, 1 ) == SendResult::pending ); // the event took a slot
    CHECK( ui.drain() == ring );
    CHECK( t.state( names::ticks, 2 ) == SendResult::sent ); // flushed first, then sent
    CHECK( ui.drain() == 2 && ui.value( names::ticks ) == 2 );

    // Distinct addresses: the pending table fills, then full (at once inline; a thread waits first).
    for( std::int64_t i = 0; i < ring; ++i ) {
        CHECK( t.state( names::ticks, i ) == SendResult::sent );
    }
    std::uint32_t held = 0;
    SendResult last = SendResult::pending;
    for( std::uint32_t uid = 1; uid < 10000 && last == SendResult::pending; ++uid ) {
        last = t.state( names::item_name, uid, "x" );
        held += last == SendResult::pending ? 1 : 0;
    }
    CHECK( last == SendResult::full && held == telegraph_size::pending );

    // Too large: over a quarter of the data ring, both ways.
    const std::string huge( telegraph_size::data / 4 + 1, 'x' );
    CHECK( t.state( names::item_name, 1U, std::string_view( huge ) ) == SendResult::too_large );
    CHECK( ui.do_( names::open, std::string_view( huge ) ).result == SendResult::too_large );
    std::puts( "held back: a full ring coalesces states, keeps them in order before events, and "
               "flushes the latest" );
}

void full_from_ui()
{
    auto line = std::make_unique<TelegraphLine>();
    TelegraphUi &ui = line->ui();
    for( std::size_t i = 0; i < telegraph_size::ring; ++i ) {
        CHECK( ui.set( names::sound, true ) == SendResult::sent );
    }
    CHECK( ui.set( names::sound, true ) == SendResult::full );
    const DoResult refused = ui.do_( names::save );
    CHECK( refused.result == SendResult::full && refused.correlation == 0 );
    Bare bare;
    CHECK( line->model().deliver( bare ) == telegraph_size::ring );
    // The refused do did not use up a correlation.
    CHECK( ui.do_( names::save ).correlation == 1 );
    std::puts( "full from the UI: refused, never waits; a refused do keeps its correlation" );
}

// ---- lists ---------------------------------------------------------------------------------------
void lists()
{
    auto line = std::make_unique<TelegraphLine>();
    Telegraph &t = line->model();
    TelegraphUi &ui = line->ui();
    Bare bare;
    Inventory inv;
    inv.order.resize( 1000 );
    std::iota( inv.order.begin(), inv.order.end(), 1U );

    CHECK( t.list( names::inventory, inv ) );
    CHECK( !t.list( names::inventory, inv ) ); // once
    ui.drain();
    const Address count{ Name{ "inventory.count" }, 0 };
    CHECK( ui.find( count ) && ui.find( count )->telegram.as_integer() == 1000 );

    // The widget asks; the model's turn answers; the rows and their items arrive.
    ListWindow window( telegraph_size::list_view );
    window.set_count( 1000 );
    const auto ask = window.visible( 100, 30 );
    CHECK( ask && ui.view( names::inventory, *ask ) == SendResult::sent );
    CHECK( t.deliver( bare ) == 1 ); // answered by the list, not given to the model
    ui.drain();
    for( std::uint32_t row = 100; row < 130; ++row ) {
        CHECK( ui.value( names::inventory, row ) == std::int64_t{ inv.order[row] } );
        CHECK( ui.value( names::item_name, inv.order[row] ) == "item" );
    }

    // A sort: the rows that changed arrive.
    std::mt19937 random( 66 ); // NOLINT(cert-msc32-c,cert-msc51-cpp): repeatable on purpose
    std::shuffle( inv.order.begin(), inv.order.end(), random );
    t.changed( names::inventory );
    ui.drain();
    for( std::uint32_t row = 100; row < 130; ++row ) {
        CHECK( ui.value( names::inventory, row ) == std::int64_t{ inv.order[row] } );
        CHECK( ui.value( names::item_name, inv.order[row] ) == "item" );
    }
    // One item, on screen: published again (one state); off screen: nothing.
    t.changed( names::inventory, inv.order[110] );
    t.changed( names::inventory, inv.order[900] );
    CHECK( ui.drain() == 1 );

    // A new widget, by the list's name: a reset brings the count; the view
    // after it is answered in full, though the model had sent those rows.
    CHECK( ui.reset_view( names::inventory.name(), "inventory" ) == SendResult::sent );
    CHECK( t.deliver( bare ) == 1 );
    CHECK( ui.drain() == 1 && ui.find( count )->telegram.as_integer() == 1000 );
    CHECK( ui.view( names::inventory.name(), {}, *ask ) == SendResult::sent );
    CHECK( t.deliver( bare ) == 1 );
    CHECK( ui.drain() == 2 * std::size_t{ ask->rows } ); // each row, and its item
    std::puts( "lists: declared, asked by the widget, answered by the list, a sort and one item; "
               "a reset" );
}

// ---- lifecycle ---------------------------------------------------------------------------------------
void lifecycle()
{
    auto line = std::make_unique<TelegraphLine>();
    Telegraph &t = line->model();
    TelegraphUi &ui = line->ui();
    Model model;
    const auto turn = [&] {
        model.log.clear();
        t.deliver( model );
        return model.log;
    };

    CHECK( turn().empty() );
    ui.background();
    ui.low_memory();
    CHECK( turn() == ( std::vector<std::string>{ "low memory", "background" } ) );
    ui.foreground( 1234 );
    CHECK( turn() == ( std::vector<std::string>{ "foreground 1234" } ) );
    // Both before a turn: the one that ends where the app is goes last.
    ui.background();
    ui.foreground( 5 );
    CHECK( turn() == ( std::vector<std::string>{ "background", "foreground 5" } ) );
    ui.foreground( 6 );
    ui.background();
    CHECK( turn() == ( std::vector<std::string>{ "foreground 6", "background" } ) );
    // Quit while visible: saved first, then quitting.
    ui.foreground( 7 );
    turn();
    CHECK( !t.quitting() );
    ui.quit();
    CHECK( turn() == ( std::vector<std::string>{ "background" } ) && t.quitting() );

    // A model with no hooks takes the signals too.
    auto other = std::make_unique<TelegraphLine>();
    Bare bare;
    other->ui().background();
    other->ui().low_memory();
    other->model().deliver( bare );
    std::puts( "lifecycle: low memory first, background and foreground in the order that ends "
               "where the app is, quit saves" );
}

// A model that allocates nothing, for the frames below.
struct Quiet
{
        void receive( Telegraph &t, const Received &m )
        {
            if( m.telegram.address().name == names::sound.name() ) {
                static_cast<void>( t.state( names::sound, m.telegram.as_boolean() ) );
            }
        }
};

void no_allocation()
{
    auto line = std::make_unique<TelegraphLine>();
    Telegraph &t = line->model();
    TelegraphUi &ui = line->ui();
    Quiet quiet;
    const std::size_t before = allocations;
    for( std::uint32_t frame = 0; frame < 2000; ++frame ) {
        static_cast<void>( ui.set( names::sound, frame % 2 == 0 ) );
        static_cast<void>( ui.do_( names::open, "a file name long enough to need the text area" ) );
        t.deliver( quiet );
        static_cast<void>( t.state( names::ticks, frame ) );
        static_cast<void>( t.state( names::item_name, frame % 64 + 1, "an item named at length" ) );
        static_cast<void>( ui.drain() );
        static_cast<void>( ui.value( names::item_name, frame % 64 + 1 ) );
    }
    CHECK( allocations == before );
    std::puts( "no allocation: 2,000 frames through both ends" );
}

} // namespace

int main()
{
    round_trips();
    held_back();
    full_from_ui();
    lists();
    lifecycle();
    no_allocation();
    std::printf( "telegraph_test: ok (sizeof(TelegraphLine) %zu bytes at the default knobs)\n",
                 sizeof( TelegraphLine ) );
    return 0;
}
