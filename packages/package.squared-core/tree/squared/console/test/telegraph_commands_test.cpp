// The Telegraph's Console commands (6.7a) over a real line, the model inline:
// `telegraph` (capacities, highest fills, held-back states, waits),
// `telegraph state` (every value, as its line, in address order, read back
// to the same values), a value too long for a line, the groups, and no
// allocation.

#include <squared/console/console.hpp>
#include <squared/console/telegraph_commands.hpp>
#include <squared/dev/frame_allocations.hpp>
#include <squared/messaging/model_runner.hpp>
#include <squared/messaging/telegram_data.hpp>
#include <squared/messaging/telegram_text.hpp>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

using namespace sq::console;
using namespace sq::messaging;

namespace
{

// A test fails loudly, with its line, in any build.
#define CHECK( condition )                                                                         \
    do {                                                                                           \
        if( !( condition ) ) {                                                                     \
            std::fprintf( stderr, "telegraph_commands_test.cpp:%d: %s\n", __LINE__, #condition );  \
            std::abort();                                                                          \
        }                                                                                          \
    } while( false )

namespace names
{
inline constexpr Key<std::int64_t, state> hp{ "player.hp" };
inline constexpr Key<bool, state> sound{ "settings.sound" };
inline constexpr Key<std::array<float, 2>, state> pos{ "player.pos" };
inline constexpr Key<std::string_view, state> title{ "doc.title" };
inline constexpr Key<std::string_view, state, with_uid> item{ "item.name" };
inline constexpr Key<std::int64_t, do_> flood{ "test.flood" };
inline constexpr Key<std::int64_t, event> flooded{ "test.flooded" };
inline constexpr Key<std::int64_t, do_> fill{ "test.fill" };
inline constexpr Key<std::int64_t, state, with_uid> cell{ "test.cell" };
inline constexpr Key<void, do_> declare{ "test.declare" };
inline constexpr Key<std::int64_t, state, with_uid> rows{ "test.rows" };
} // namespace names

const std::string long_title( 700, 't' ); // longer than a Console line (512)

// A list of three rows, for the `lists` figure.
struct Rows
{
        [[nodiscard]] std::uint32_t count() const noexcept { return 3; }
        [[nodiscard]] std::uint32_t uid_at( std::uint32_t row ) const noexcept { return row + 1; }
        void publish( Telegraph & /*telegraph*/, std::uint32_t /*uid*/ ) noexcept {}
};

struct Model
{
        Rows list;

        void start( Telegraph &t ) noexcept
        {
            static_cast<void>( t.state( names::hp, std::int64_t{ 42 } ) );
            static_cast<void>( t.state( names::sound, false ) );
            static_cast<void>( t.state( names::pos, std::array<float, 2>{ 1.5F, 2.0F } ) );
            static_cast<void>( t.state( names::title, std::string_view( long_title ) ) );
            for( std::uint32_t uid = 1; uid <= 3; ++uid ) {
                static_cast<void>( t.state( names::item, uid, std::string_view( "fur coat" ) ) );
            }
        }

        void receive( Telegraph &t, const Received &received ) noexcept
        {
            const Name name = received.telegram.address().name;
            const std::int64_t count = received.telegram.as_integer();
            if( name == names::flood.name() ) {
                for( std::int64_t i = 0; i < count; ++i ) {
                    static_cast<void>( t.event( names::flooded, i ) );
                }
            } else if( name == names::declare.name() ) {
                CHECK( t.list( names::rows, list ) );
            } else if( name == names::fill.name() ) {
                for( std::int64_t i = 1; i <= count; ++i ) {
                    static_cast<void>( t.state( names::cell, static_cast<std::uint32_t>( i ), i ) );
                }
            }
        }
};

std::vector<std::string> run( Console &console, const char *line )
{
    std::size_t before = 0;
    for( const Line printed : console.lines() ) {
        static_cast<void>( printed );
        ++before;
    }
    console.run( line );
    std::vector<std::string> out;
    std::size_t at = 0;
    for( const Line printed : console.lines() ) {
        if( at++ > before ) { // past the echo
            out.emplace_back( printed.text );
        }
    }
    return out;
}

bool has( const std::vector<std::string> &lines, const std::string &start )
{
    for( const std::string &line : lines ) {
        if( line.rfind( start, 0 ) == 0 ) {
            return true;
        }
    }
    return false;
}

void commands()
{
    const auto line = std::make_unique<TelegraphLine>();
    TelegraphUi &ui = line->ui();
    auto console = std::make_unique<Console>();
    CHECK( add_telegraph_commands( *console, ui ) == 0 );
    ModelRunner<Model> runner( *line, on_ui_thread );
    static_cast<void>( ui.drain() );
    CHECK( ui.bind( { names::hp.name(), 9 } ) ); // bound, with no value yet

    // telegraph state: every value as its line, in address order.
    std::vector<std::string> state = run( *console, "telegraph state" );
    CHECK( state.back() == "telegraph: 7 values; 1 bound, with no value yet" );
    state.pop_back();
    CHECK( state.size() == 7 ); // 6 lines, and the warning for the long title
    std::array<std::byte, 256> storage{};
    std::optional<Address> previous;
    std::size_t lines_read = 0;
    for( const std::string &text : state ) {
        if( text.find( "longer than a line" ) != std::string::npos ) {
            CHECK( text.rfind( "state #", 0 ) == 0 );
            CHECK( text.find( ": 700 bytes of value" ) != std::string::npos );
            const Address title{ names::title.name(), 0 };
            CHECK( !previous || previous->name.id < title.name.id );
            previous = title;
            continue;
        }
        TelegramData data( storage );
        const std::optional<Telegram> read = read_telegram( text, data );
        CHECK( read && read->verb() == Verb::state );
        const Address address = read->address();
        if( previous ) { // ascending: by name id, then uid
            const bool later_name = previous->name.id < address.name.id;
            const bool same_name = previous->name.id == address.name.id;
            const bool after = later_name || ( same_name && previous->uid < address.uid );
            CHECK( after );
        }
        previous = address;
        const std::optional<CachedValue> cached = ui.find( address );
        CHECK( cached && cached->telegram.type() == read->type() );
        ++lines_read;
    }
    CHECK( lines_read == 6 );
    CHECK( has( state, "state #" ) ); // names are not registered yet (6.7b)

    // telegraph: the capacities and what has been used.
    std::vector<std::string> stats = run( *console, "telegraph" );
    CHECK( has( stats, "to the model: " ) && has( stats, "to the UI:    " ) );
    CHECK( has( stats, "cache:        8 of 512 entries, 1 bound" ) );
    CHECK( has( stats, "held back:    0 now, 0 at most, of 256" ) );
    CHECK( has( stats, "lists:        0 of 16" ) );
    CHECK( has( stats, "model sends:  waited 0 times, refused 0 (full)" ) );

    // States beyond the ring are held back; inline, events beyond it are refused.
    CHECK( ui.do_( names::fill, std::int64_t{ 300 } ).result == SendResult::sent );
    CHECK( runner.turn() == 1 );
    stats = run( *console, "telegraph" );
    CHECK( has( stats, "held back:    44 now, 44 at most, of 256" ) );
    static_cast<void>( ui.drain() );
    CHECK( runner.turn() == 0 ); // the turn flushes what is held
    static_cast<void>( ui.drain() );
    CHECK( ui.do_( names::flood, std::int64_t{ 260 } ).result == SendResult::sent );
    CHECK( runner.turn() == 1 );
    stats = run( *console, "telegraph" );
    CHECK( has( stats, "held back:    0 now, 44 at most, of 256" ) );
    CHECK( has( stats, "to the UI:    256 of 256 telegrams at most" ) );
    CHECK( has( stats, "model sends:  waited 0 times, refused 4 (full)" ) );
    static_cast<void>( ui.drain() );

    // A list declared: counted.
    CHECK( ui.do_( names::declare ).result == SendResult::sent );
    CHECK( runner.turn() == 1 );
    stats = run( *console, "telegraph" );
    CHECK( has( stats, "lists:        1 of 16" ) );
    static_cast<void>( ui.drain() );

    // Wrong words: the usage, as an error.
    const std::vector<std::string> wrong = run( *console, "telegraph nonsense" );
    CHECK( wrong.size() == 1 && wrong[0].rfind( "telegraph [state]", 0 ) == 0 );

    // The groups: dump state, dump telegraph.
    console->run( "dump state telegraph" );
    int ran = 0;
    while( console->dumping() ) {
        console->update();
        ++ran;
    }
    CHECK( ran == 2 );

    // Nothing allocates: both commands, many times.
    sq::dev::frame_begin();
    for( int i = 0; i < 50; ++i ) {
        console->run( "telegraph" );
        console->run( "telegraph state" );
    }
    const sq::dev::FrameAllocations used = sq::dev::frame_end();
    CHECK( used.count == 0 );
}

} // namespace

int main()
{
    commands();
    std::printf( "telegraph_commands_test: ok\n" );
    return 0;
}
