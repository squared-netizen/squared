// The Telegraph's dev tools (6.7a, 6.7b) over a real line, the model inline:
// `telegraph` (capacities, highest fills, held-back states, waits),
// `telegraph state` (every value, as its line, in address order, read back
// to the same values; names once the UI has used their keys), a value too
// long for a line, the groups; the tap (both ways, to the Console or a sink,
// off); recording, and replaying at the recorded pace and fast, through a
// full ring, past bad and overlong lines; refused files; no allocation.

#include <squared/console/console.hpp>
#include <squared/console/telegraph_tools.hpp>
#include <squared/dev/frame_allocations.hpp>
#include <squared/messaging/model_runner.hpp>
#include <squared/messaging/telegram_data.hpp>
#include <squared/messaging/telegram_text.hpp>

#include <sys/stat.h>
#include <unistd.h>

#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
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
            std::fprintf( stderr, "telegraph_tools_test.cpp:%d: %s\n", __LINE__, #condition );     \
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
inline constexpr Key<bool, state | set> music{ "settings.music" };
inline constexpr Key<std::string_view, state | set> nick{ "player.nick" };
} // namespace names

using namespace std::chrono_literals;
using sq::time::TimePoint;

// What the model was sent, for the replay tests.
struct Heard
{
        std::vector<std::string> lines;
        std::array<std::byte, 256> storage{};
        NameTable names{ storage };
};
Heard *heard = nullptr;

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
            if( heard != nullptr ) {
                const std::array<std::byte, 0> none{};
                std::array<char, 256> text{};
                const Telegram plain = received.telegram.with_correlation( 0 );
                const std::size_t length = write_telegram( plain, none, heard->names, text );
                heard->lines.emplace_back( text.data(), length );
            }
            if( name == names::music.name() ) {
                static_cast<void>( t.state( names::music, received.telegram.as_boolean() ) );
            }
            if( name == names::nick.name() ) {
                const auto *chars = reinterpret_cast<const char *>( received.data.data() );
                const std::string_view text( chars, received.data.size() );
                static_cast<void>( t.state( names::nick, text ) );
            }
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

std::string temporary_directory()
{
    const char *base = std::getenv( "TMPDIR" );
    std::string pattern = std::string( base != nullptr ? base : "/tmp" ) + "/sq_tools_XXXXXX";
    CHECK( ::mkdtemp( pattern.data() ) != nullptr );
    return pattern;
}

std::string read_file( const std::string &path )
{
    std::ifstream in( path );
    std::stringstream text;
    text << in.rdbuf();
    return text.str();
}

void write_file( const std::string &path, const std::string &text )
{
    std::ofstream out( path );
    out << text;
}

void commands()
{
    const auto line = std::make_unique<TelegraphLine>();
    TelegraphUi &ui = line->ui();
    auto console = std::make_unique<Console>();
    TelegraphTools tools( ui, nullptr );
    CHECK( tools.add_commands( *console ) == 0 );
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
    CHECK( has( state, "state #" ) ); // the UI has used none of these keys yet

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
    CHECK( wrong.size() == 5 && wrong[0] == "telegraph: each capacity and its highest fill" );
    CHECK( run( *console, "telegraph record" ).back().rfind( "telegraph: cannot record", 0 ) == 0 );

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

// Names: from the keys the UI uses, from register_names, and a list's.
void names_and_tap()
{
    const auto line = std::make_unique<TelegraphLine>();
    TelegraphUi &ui = line->ui();
    auto console = std::make_unique<Console>();
    TelegraphTools tools( ui, nullptr );
    CHECK( tools.add_commands( *console ) == 0 );
    CHECK( ui.names() == &tools.names() );
    ModelRunner<Model> runner( *line, on_ui_thread );
    static_cast<void>( ui.drain() );

    ui.register_names( names::hp, names::item );
    CHECK( !ui.value( names::sound ).has_value() || true ); // a value read names its key
    CHECK( ui.do_( names::declare ).result == SendResult::sent );
    CHECK( runner.turn() == 1 );
    static_cast<void>( ui.drain() );
    CHECK( ui.view( names::rows, RowSpan{ 0, 3 } ) == SendResult::sent ); // rows, .view, .count
    CHECK( runner.turn() == 1 );
    static_cast<void>( ui.drain() );
    const std::vector<std::string> state = run( *console, "telegraph state" );
    CHECK( has( state, "state player.hp 42" ) );
    CHECK( has( state, "state settings.sound false" ) );
    CHECK( has( state, "state item[1].name \"fur coat\"" ) );
    CHECK( has( state, "state test.rows.count 3" ) );
    CHECK( has( state, "state test.rows 1" ) );    // row 0: written with no uid
    CHECK( has( state, "state test[1].rows 2" ) ); // the uid after the first part
    CHECK( has( state, "state #" ) );              // player.pos and doc.title: never used by the UI

    // The tap: both ways, into the Console.
    CHECK( run( *console, "telegraph tap" ).back() == "telegraph: the tap is off" );
    CHECK( run( *console, "telegraph tap on" ).back() == "telegraph: the tap is on" );
    std::vector<std::string> tapped = run( *console, "telegraph" ); // no telegram: no line
    CHECK( !has( tapped, "ui>model" ) && !has( tapped, "model>ui" ) );
    std::size_t before = 0;
    for( const Line printed : console->lines() ) {
        static_cast<void>( printed );
        ++before;
    }
    CHECK( ui.set( names::music, true ) == SendResult::sent );
    CHECK( runner.turn() == 1 );
    static_cast<void>( ui.drain() );
    std::vector<std::string> since;
    std::size_t at = 0;
    for( const Line printed : console->lines() ) {
        if( at++ >= before ) {
            since.emplace_back( printed.text );
        }
    }
    CHECK( since.size() == 2 );
    CHECK( since[0] == "ui>model set settings.music true" );
    CHECK( since[1] == "model>ui state settings.music true" );

    // Text both ways: the tap sees the value's bytes where its line says.
    CHECK( ui.set( names::nick, std::string_view( "Ann \"A\" Lee" ) ) == SendResult::sent );
    CHECK( runner.turn() == 1 );
    static_cast<void>( ui.drain() );
    const std::vector<std::string> texts = { std::string( ( *console->lines().last( 2 ) ).text ),
                                             std::string( ( *console->lines().last( 1 ) ).text ) };
    CHECK( texts.size() == 2 && texts[0] == "ui>model set player.nick \"Ann \\\"A\\\" Lee\"" );
    CHECK( texts[1] == "model>ui state player.nick \"Ann \\\"A\\\" Lee\"" );

    // To a sink instead, as the dev log will take them.
    std::vector<std::string> sunk;
    const TelegraphTools::Sink collect = []( void *context, std::string_view text ) noexcept {
        static_cast<std::vector<std::string> *>( context )->emplace_back( text );
    };
    tools.set_sink( collect, &sunk );
    CHECK( ui.do_( names::fill, std::int64_t{ 1 } ).result == SendResult::sent );
    CHECK( sunk.size() == 1 && sunk[0] == "ui>model do test.fill 1 @2" );
    CHECK( runner.turn() == 1 );
    static_cast<void>( ui.drain() );
    CHECK( sunk.size() == 2 &&
           sunk[1].rfind( "model>ui state #", 0 ) == 0 ); // test.cell: never used by the UI
    CHECK( sunk[1].size() > 5 && sunk[1].substr( sunk[1].size() - 5 ) == "[1] 1" );

    // Off: nothing more, and no tap on the line.
    CHECK( run( *console, "telegraph tap off" ).back() == "telegraph: the tap is off" );
    CHECK( ui.set( names::music, false ) == SendResult::sent );
    CHECK( runner.turn() == 1 );
    static_cast<void>( ui.drain() );
    CHECK( sunk.size() == 2 );

    // Nothing allocates while the tap writes (into the sink's string, outside the count, aside).
    tools.set_sink( []( void *, std::string_view ) noexcept {}, nullptr );
    tools.tap( true );
    sq::dev::frame_begin();
    for( int i = 0; i < 100; ++i ) {
        CHECK( ui.set( names::music, i % 2 == 0 ) == SendResult::sent );
        CHECK( runner.turn() == 1 );
        static_cast<void>( ui.drain() );
    }
    const sq::dev::FrameAllocations used = sq::dev::frame_end();
    CHECK( used.count == 0 );
}

void record_and_replay()
{
    const std::string directory = temporary_directory();
    const auto line = std::make_unique<TelegraphLine>();
    TelegraphUi &ui = line->ui();
    auto console = std::make_unique<Console>();
    TelegraphTools tools( ui, directory.c_str() );
    CHECK( tools.add_commands( *console ) == 0 );
    ModelRunner<Model> runner( *line, on_ui_thread );
    static_cast<void>( ui.drain() );
    auto log = std::make_unique<Heard>();
    CHECK( log->names.add( "settings.music" ) == NameTable::AddResult::added );
    CHECK( log->names.add( "test.fill" ) == NameTable::AddResult::added );
    heard = log.get();

    // Record: what the UI sends, a line each with its milliseconds.
    CHECK( run( *console, "telegraph record" ).back() ==
           "telegraph: recording what the UI sends to files/replay.txt" );
    CHECK( tools.recording() && !tools.tapping() );
    CHECK( ui.set( names::music, true ) == SendResult::sent );
    CHECK( ui.do_( names::fill, std::int64_t{ 2 } ).result == SendResult::sent );
    CHECK( runner.turn() == 2 );
    static_cast<void>( ui.drain() ); // what the model sends is not recorded
    CHECK( run( *console, "telegraph record stop" ).back() == "telegraph: recorded 2 telegrams" );
    CHECK( !tools.recording() );
    const std::string recorded = read_file( directory + "/replay.txt" );
    std::istringstream lines( recorded );
    std::string first;
    std::string second;
    std::getline( lines, first );
    std::getline( lines, second );
    CHECK( first.substr( first.find( ' ' ) + 1 ) == "set settings.music true" );
    CHECK( second.substr( second.find( ' ' ) + 1 ) == "do test.fill 2 @1" );
    CHECK( std::stoll( first ) >= 0 && std::stoll( second ) >= std::stoll( first ) );

    // Replay it, fast: the model hears the same.
    log->lines.clear();
    CHECK( run( *console, "telegraph replay fast" ).back() ==
           "telegraph: replaying files/replay.txt, fast" );
    tools.update( sq::time::now() );
    CHECK( runner.turn() == 2 );
    CHECK( log->lines.size() == 2 && log->lines[0] == "set settings.music true" );
    CHECK( log->lines[1] == "do test.fill 2" );
    std::size_t before = 0;
    for( const Line printed : console->lines() ) {
        static_cast<void>( printed );
        ++before;
    }
    tools.update( sq::time::now() );
    CHECK( !tools.replaying() );
    CHECK( ( *console->lines().last( 1 ) ).text ==
           "telegraph: replay done, 2 sent, 0 lines not sent" );
    static_cast<void>( ui.drain() );

    // At the recorded pace, the time passed in: comments and blank lines
    // skipped, a line that is no telegram counted, a line longer than the
    // buffer left out.
    const std::string overlong = "50 set settings.music \"" + std::string( 5000, 'x' ) + "\"\n";
    write_file( directory + "/paced.txt", "0 set settings.music false\n# a comment\n\n"
                                          "100 set settings.music true\nnot a telegram\n" +
                                              overlong + "250 do test.fill 1\n" );
    log->lines.clear();
    CHECK( tools.replay( "paced.txt", false ) );
    const TimePoint t0 = TimePoint( 1000s );
    tools.update( t0 );
    CHECK( runner.turn() == 1 && log->lines.size() == 1 );
    tools.update( t0 + 50ms );
    CHECK( runner.turn() == 0 );
    tools.update( t0 + 100ms );
    CHECK( runner.turn() == 1 && log->lines[1] == "set settings.music true" );
    tools.update( t0 + 249ms );
    CHECK( runner.turn() == 0 && tools.replaying() );
    tools.update( t0 + 250ms );
    CHECK( runner.turn() == 1 && log->lines[2] == "do test.fill 1" );
    tools.update( t0 + 260ms );
    CHECK( !tools.replaying() );
    CHECK( ( *console->lines().last( 1 ) ).text ==
           "telegraph: replay done, 3 sent, 2 lines not sent" );
    // One warning, for the line that is no telegram: the overlong one is left out whole.
    std::size_t warnings = 0;
    for( const Line printed : console->lines() ) {
        warnings += printed.text.find( "is not one" ) != std::string_view::npos ? 1U : 0U;
    }
    CHECK( warnings == 1 );
    static_cast<void>( ui.drain() );

    // Through a full ring: 300 sets, 64 an update, the rest when room frees.
    std::string many;
    for( int i = 0; i < 300; ++i ) {
        many += "0 set settings.music " + std::string( i % 2 == 0 ? "true" : "false" ) + "\n";
    }
    write_file( directory + "/many.txt", many );
    log->lines.clear();
    CHECK( tools.replay( "many.txt", true ) );
    for( int i = 0; i < 4; ++i ) {
        tools.update( t0 );
    }
    CHECK( tools.replaying() ); // 256 sent: the ring is full
    sq::dev::frame_begin();     // replaying allocates nothing (the model's log aside)
    tools.update( t0 );
    const sq::dev::FrameAllocations used = sq::dev::frame_end();
    CHECK( used.count == 0 );
    heard = nullptr;
    CHECK( runner.turn() == 256 );
    static_cast<void>( ui.drain() );
    tools.update( t0 );
    CHECK( runner.turn() == 44 );
    tools.update( t0 );
    CHECK( !tools.replaying() );
    CHECK( ( *console->lines().last( 1 ) ).text ==
           "telegraph: replay done, 300 sent, 0 lines not sent" );
    static_cast<void>( ui.drain() );

    // Refused: a missing file, a name with a directory, no directory at all.
    CHECK( run( *console, "telegraph replay nothing.txt" ).back() ==
           "telegraph: cannot replay files/nothing.txt" );
    CHECK( !tools.replay( "../replay.txt", false ) && !tools.record( "a/b" ) );
    CHECK( !tools.record( "" ) && !tools.record( ".." ) );
    const std::string escaped = directory + "/../sq_tools_escaped.txt";
    const bool escapes =
        tools.record( "../sq_tools_escaped.txt" ); // would land beside the directory
    ::unlink( escaped.c_str() );
    CHECK( !escapes );
    int tapped_after = 0;
    {
        TelegraphTools elsewhere( ui, nullptr );
        CHECK( !elsewhere.record( "replay.txt" ) && !elsewhere.replay( "replay.txt", true ) );
        CHECK( ui.names() == &elsewhere.names() ); // the newest tools hold the line
        const TelegraphTools::Sink count = []( void *calls, std::string_view ) noexcept {
            ++*static_cast<int *>( calls );
        };
        elsewhere.set_sink( count, &tapped_after );
        elsewhere.tap( true );
        CHECK( ui.set( names::music, true ) == SendResult::sent );
        CHECK( tapped_after == 1 );
    }
    CHECK( ui.names() == nullptr );                            // gone: detached, names and tap
    CHECK( ui.set( names::music, true ) == SendResult::sent ); // no tap to call
    CHECK( tapped_after == 1 );
    CHECK( runner.turn() == 2 );
    static_cast<void>( ui.drain() );
    ::unlink( ( directory + "/replay.txt" ).c_str() );
    ::unlink( ( directory + "/paced.txt" ).c_str() );
    ::unlink( ( directory + "/many.txt" ).c_str() );
    ::rmdir( directory.c_str() );
}

} // namespace

int main()
{
    commands();
    names_and_tap();
    record_and_replay();
    std::printf( "telegraph_tools_test: ok (sizeof(TelegraphTools) %zu)\n",
                 sizeof( TelegraphTools ) );
    return 0;
}
