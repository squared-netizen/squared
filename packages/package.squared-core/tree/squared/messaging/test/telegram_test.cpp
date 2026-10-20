// Keys (6.1b) and the 32-byte telegram with its text form (6.2): what the
// compiler refuses, the exact lines, what is read and refused, a fuzz test
// both ways, and no allocation.

#include <squared/messaging/key.hpp>
#include <squared/messaging/name_table.hpp>
#include <squared/messaging/telegram.hpp>
#include <squared/messaging/telegram_data.hpp>
#include <squared/messaging/telegram_text.hpp>

#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
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
void operator delete( void *p ) noexcept
{
    std::free( p ); // NOLINT(cppcoreguidelines-no-malloc)
}
void operator delete( void *p, std::size_t /*size*/ ) noexcept
{
    std::free( p ); // NOLINT(cppcoreguidelines-no-malloc)
}

using namespace sq::messaging;

// ---- keys: what compiles and what does not ---------------------------------------------
namespace names
{
inline constexpr Key<std::int64_t, state> counter_ticks{ "counter.ticks" };
inline constexpr Key<bool, state | set> settings_sound{ "settings.sound" };
inline constexpr Key<std::string_view, do_> file_open{ "file.open" };
inline constexpr Key<void, event> game_over{ "game.over" };
inline constexpr Key<std::string_view, state, with_uid> item_name{ "item.name" };
inline constexpr Key<std::array<float, 2>, state> player_pos{ "player.pos" };
} // namespace names

static_assert( sizeof( names::counter_ticks ) == 16 );
static_assert( names::counter_ticks.name() == Name( "counter.ticks" ) );
static_assert( names::counter_ticks.text() == "counter.ticks" );
static_assert( decltype( names::counter_ticks )::type == ValueType::integer );
static_assert( decltype( names::item_name )::takes_uid );
static_assert( !decltype( names::counter_ticks )::takes_uid );
// The right verb and type compile...
static_assert( sendable<decltype( names::counter_ticks ), Verb::state, int> );
static_assert( sendable<decltype( names::counter_ticks ), Verb::state, std::int64_t> );
static_assert( sendable<decltype( names::settings_sound ), Verb::set, bool> );
static_assert( sendable<decltype( names::settings_sound ), Verb::state, bool> );
static_assert( sendable<decltype( names::file_open ), Verb::do_, const char *> );
static_assert( sendable<decltype( names::game_over ), Verb::event, void> );
static_assert( sendable<decltype( names::player_pos ), Verb::state, std::array<float, 2>> );
// ...and these do not: the wrong type, a verb the address does not carry, a bool for an int.
static_assert( !sendable<decltype( names::counter_ticks ), Verb::state, const char *> );
static_assert( !sendable<decltype( names::counter_ticks ), Verb::state, bool> );
static_assert( !sendable<decltype( names::counter_ticks ), Verb::set, int> );
static_assert( !sendable<decltype( names::settings_sound ), Verb::state, int> );
static_assert( !sendable<decltype( names::file_open ), Verb::state, const char *> );
static_assert( !sendable<decltype( names::game_over ), Verb::event, int> );
static_assert( !sendable<decltype( names::counter_ticks ), Verb::state, void> );
static_assert( !sendable<int, Verb::state, int> ); // not a key
static_assert( state.has( Verb::state ) && !state.has( Verb::set ) );
static_assert( ( state | set ).has( Verb::set ) );

namespace
{

std::array<std::byte, 8192> name_storage{};
NameTable names_table{ name_storage };

std::string line_of( const Telegram &telegram, const TelegramData &data )
{
    std::array<char, 4096> out{};
    const std::size_t length = write_telegram( telegram, data.bytes(), names_table, out );
    return { out.data(), length };
}

std::span<const std::byte> as_bytes( std::string_view text )
{
    return { reinterpret_cast<const std::byte *>( text.data() ), text.size() };
}

/// Equal but for where the data is: the data's bytes are compared instead.
bool same( const Telegram &a, const TelegramData &a_data, const Telegram &b,
           const TelegramData &b_data )
{
    if( a.type() == ValueType::text || a.type() == ValueType::uids ) {
        if( a.type() != b.type() || a.verb() != b.verb() || !( a.address() == b.address() ) ||
            a.correlation() != b.correlation() ) {
            return false;
        }
        const auto x = a_data.at( a.as_data() );
        const auto y = b_data.at( b.as_data() );
        return x.size() == y.size() &&
               ( x.empty() || std::memcmp( x.data(), y.data(), x.size() ) == 0 );
    }
    return a == b;
}

} // namespace

int main()
{
    for( const char *name : { "settings.sound", "file.open", "item.name", "player.pos", "game.over",
                              "counter.ticks", "inventory" } ) {
        assert( names_table.add( name ) == NameTable::AddResult::added );
    }
    std::array<std::byte, 4096> storage{};
    TelegramData data{ storage };

    // ---- the telegram ----------------------------------------------------------------
    static_assert( sizeof( Telegram ) == 32 );
    const Telegram hp = Telegram::integer( Verb::state, { "counter.ticks", 0 }, 42 );
    assert( hp.valid() && hp.verb() == Verb::state && hp.type() == ValueType::integer );
    assert( hp.as_integer() == 42 && hp.address() == ( Address{ "counter.ticks", 0 } ) );
    // Equal telegrams are equal byte for byte: the unused bytes are zero.
    assert( TelegramBytes::of( Telegram::boolean( Verb::set, { "settings.sound", 0 }, true ) ) ==
            TelegramBytes::of( Telegram::boolean( Verb::set, { "settings.sound", 0 }, true ) ) );
    {
        auto bytes =
            TelegramBytes::of( Telegram::boolean( Verb::set, { "settings.sound", 0 }, true ) );
        bytes[31] = std::byte{ 1 }; // an unused value byte
        assert( !TelegramBytes::from( bytes ).valid() );
        bytes = TelegramBytes::of( hp );
        bytes[15] = std::byte{ 0x80 }; // a spare bit of the kind
        assert( !TelegramBytes::from( bytes ).valid() );
        bytes = TelegramBytes::of( hp );
        bytes[14] = std::byte{ 0x3E }; // type 15
        assert( !TelegramBytes::from( bytes ).valid() );
    }

    // ---- exact lines ------------------------------------------------------------------
    struct Exact
    {
            Telegram telegram;
            std::string_view line;
    };
    data.clear();
    const DataRef notes = *data.append( as_bytes( "notes.txt" ) );
    const DataRef coat = *data.append( as_bytes( "fur \"coat\"\n\t\\\x01\x7f\xc3\xa9" ) );
    const std::array<std::uint32_t, 3> uid_list{ 17, 4, 22 };
    const DataRef uid_ref = *data.append( std::as_bytes( std::span( uid_list ) ) );
    const DataRef nothing = *data.append( {} );
    const std::array<float, 4> four{ 1.5F, 2, 0, 1 };
    const std::array<Exact, 16> exact{ {
        { Telegram::boolean( Verb::set, { "settings.sound", 0 }, false ),
          "set settings.sound false" },
        { Telegram::data( Verb::do_, { "file.open", 0 }, ValueType::text, notes )
              .with_correlation( 12 ),
          "do file.open \"notes.txt\" @12" },
        { Telegram::empty( Verb::event, { "game.over", 0 } ).with_correlation( 12 ),
          "event game.over @12" },
        { Telegram::data( Verb::state, { "item.name", 17 }, ValueType::text, coat ),
          "state item[17].name \"fur \\\"coat\\\"\\n\\t\\\\\\x01\\x7f\xc3\xa9\"" },
        { Telegram::floats( Verb::state, { "player.pos", 0 }, std::span( four ).first( 2 ) ),
          "state player.pos (1.5 2)" },
        { Telegram::floats( Verb::state, { "player.pos", 0 }, four ),
          "state player.pos (1.5 2 0 1)" },
        { Telegram::integer( Verb::state, { "counter.ticks", 0 }, -7 ), "state counter.ticks -7" },
        { Telegram::integer( Verb::state, { "counter.ticks", 0 },
                             std::numeric_limits<std::int64_t>::min() ),
          "state counter.ticks -9223372036854775808" },
        { Telegram::number( Verb::state, { "counter.ticks", 0 }, 2.0 ), "state counter.ticks 2.0" },
        { Telegram::number( Verb::state, { "counter.ticks", 0 }, 0.1 ), "state counter.ticks 0.1" },
        { Telegram::number( Verb::state, { "counter.ticks", 0 }, -0.0 ),
          "state counter.ticks -0.0" },
        { Telegram::number( Verb::state, { "counter.ticks", 0 }, 1e21 ),
          "state counter.ticks 1e+21" },
        { Telegram::number( Verb::state, { "counter.ticks", 0 }, -INFINITY ),
          "state counter.ticks -inf" },
        { Telegram::data( Verb::state, { "inventory", 0 }, ValueType::uids, uid_ref ),
          "state inventory [17 4 22]" },
        { Telegram::data( Verb::state, { "inventory", 0 }, ValueType::uids, nothing ),
          "state inventory []" },
        { Telegram::integer( Verb::state, { "inventory", 20 }, 17 ), "state inventory[20] 17" },
    } };
    for( const Exact &e : exact ) {
        const std::string line = line_of( e.telegram, data );
        if( line != e.line ) {
            std::printf( "wrote  %s\nwanted %.*s\n", line.c_str(),
                         static_cast<int>( e.line.size() ), e.line.data() );
        }
        assert( line == e.line );
        std::array<std::byte, 512> back_storage{};
        TelegramData back{ back_storage };
        const auto read = read_telegram( line, back );
        assert( read && same( e.telegram, data, *read, back ) );
    }
    // An unregistered name is written as its id, and read back as the same telegram.
    {
        const Telegram unknown = Telegram::boolean( Verb::state, { "not.registered", 3 }, true );
        const std::string line = line_of( unknown, data );
        assert( line.starts_with( "state #" ) && line.ends_with( "[3] true" ) );
        assert( read_telegram( line, data ) == unknown );
    }

    // ---- not canonical, read; and refused --------------------------------------------
    struct Loose
    {
            std::string_view in;
            std::string_view canonical;
    };
    for( const Loose &l : std::array<Loose, 8>{ {
             { "  set   settings.sound    true  ", "set settings.sound true" },
             { "state counter.ticks 1.50", "state counter.ticks 1.5" },
             { "state counter.ticks 1E3", "state counter.ticks 1000.0" },
             { "state counter.ticks 007", "state counter.ticks 7" },
             { "state inventory[0] 1", "state inventory 1" },
             { "state player.pos ( 1   2 )", "state player.pos (1 2)" },
             { "state inventory [ 1  2 ]", "state inventory [1 2]" },
             { "do file.open \"\\x41\\x4A\" @0", "do file.open \"AJ\"" },
         } } ) {
        data.clear();
        const auto read = read_telegram( l.in, data );
        if( !read ) {
            std::printf( "not read: %.*s\n", static_cast<int>( l.in.size() ), l.in.data() );
        }
        assert( read );
        assert( line_of( *read, data ) == l.canonical );
    }
    data.clear();
    for( const std::string_view bad :
         { "", "say settings.sound true", "set Settings.sound true", "set settings.sound yes",
           "state counter.ticks nan", "state counter.ticks 1x", "state counter.ticks +5",
           "state counter.ticks 9223372036854775808", "state player.pos (1)",
           "state player.pos (1 2 3 4 5)", "state player.pos (1 nan)", "state player.pos (1 2",
           "do file.open \"open", "do file.open \"\\q\"", "do file.open \"\\x4\"",
           "state inventory [1 -2]", "state inventory [1 2", "event game.over @",
           "event game.over @65536", "event game.over @1 2", "set settings.sound true false",
           "state counter.ticks 1 @x",
           // these fail after their text or uids were read: nothing may stay behind
           "do file.open \"notes\" extra", "state inventory [1 2] x",
           "do file.open \"a\" @99999" } ) {
        assert( !read_telegram( bad, data ) );
    }
    assert( data.used() == 0 ); // a refused line leaves nothing behind

    // A data area too small: refused whole.
    {
        std::array<std::byte, 4> tiny_storage{};
        TelegramData tiny{ tiny_storage };
        assert( !read_telegram( "do file.open \"notes.txt\"", tiny ) && tiny.used() == 0 );
        assert( read_telegram( "do file.open \"note\"", tiny ) && tiny.used() == 4 );
    }

    // ---- fuzz: telegram -> line -> telegram -> line --------------------------------------
    std::mt19937_64 random( 20261020 );
    const auto pick = [&]( std::uint64_t n ) { return static_cast<std::size_t>( random() % n ); };
    const std::array<std::string_view, 5> fuzz_names{ "settings.sound", "item.name", "inventory",
                                                      "not.registered", "a.b.c" };
    const auto random_float = [&] {
        switch( pick( 6 ) ) {
            case 0:
                return std::numeric_limits<float>::infinity();
            case 1:
                return -0.0F;
            case 2:
                return std::numeric_limits<float>::denorm_min();
            default: {
                float f = 0;
                do {
                    const auto bits = static_cast<std::uint32_t>( random() );
                    std::memcpy( &f, &bits, sizeof( f ) );
                } while( std::isnan( f ) );
                return f;
            }
        }
    };
    std::size_t lines = 0;
    for( int round = 0; round < 20000; ++round ) {
        data.clear();
        const Address to{ Name::of( fuzz_names[pick( fuzz_names.size() )] ),
                          pick( 3 ) == 0 ? 0U : static_cast<std::uint32_t>( random() ) };
        const auto verb = static_cast<Verb>( pick( 4 ) );
        Telegram telegram = Telegram::empty( verb, to );
        switch( pick( 9 ) ) {
            case 0:
                break;
            case 1:
                telegram = Telegram::boolean( verb, to, pick( 2 ) == 0 );
                break;
            case 2:
                telegram = Telegram::integer( verb, to, static_cast<std::int64_t>( random() ) );
                break;
            case 3: {
                double d = 0;
                do {
                    const std::uint64_t bits = random();
                    std::memcpy( &d, &bits, sizeof( d ) );
                } while( std::isnan( d ) );
                telegram = Telegram::number(
                    verb, to, pick( 4 ) == 0 ? static_cast<double>( pick( 1000 ) ) : d );
                break;
            }
            case 4:
            case 5:
            case 6: {
                std::array<float, 4> f{ random_float(), random_float(), random_float(),
                                        random_float() };
                telegram = Telegram::floats( verb, to, std::span( f ).first( 2 + pick( 3 ) ) );
                break;
            }
            case 7: {
                std::array<std::byte, 64> text{};
                const std::size_t length = pick( text.size() );
                for( std::size_t i = 0; i < length; ++i ) {
                    text[i] = static_cast<std::byte>( random() );
                }
                telegram = Telegram::data( verb, to, ValueType::text,
                                           *data.append( std::span( text ).first( length ) ) );
                break;
            }
            default: {
                std::array<std::uint32_t, 16> uids{};
                const std::size_t count = pick( uids.size() );
                for( std::size_t i = 0; i < count; ++i ) {
                    uids[i] = static_cast<std::uint32_t>( random() );
                }
                telegram = Telegram::data(
                    verb, to, ValueType::uids,
                    *data.append( std::as_bytes( std::span( uids ).first( count ) ) ) );
                break;
            }
        }
        if( pick( 3 ) == 0 ) {
            telegram = telegram.with_correlation( static_cast<std::uint16_t>( random() ) );
        }
        assert( telegram.valid() );
        const std::string first = line_of( telegram, data );
        assert( !first.empty() );
        std::array<std::byte, 1024> back_storage{};
        TelegramData back{ back_storage };
        const auto read = read_telegram( first, back );
        if( !read || !same( telegram, data, *read, back ) ) {
            std::printf( "round %d: %s\n", round, first.c_str() );
        }
        assert( read && same( telegram, data, *read, back ) );
        assert( line_of( *read, back ) == first );
        ++lines;
    }

    // ---- fuzz: damaged lines are refused or read, never crash; what is read is canonical --
    std::size_t damaged_read = 0;
    for( int round = 0; round < 20000; ++round ) {
        std::string line( exact[pick( exact.size() )].line );
        for( std::size_t edits = 1 + pick( 4 ); edits > 0 && !line.empty(); --edits ) {
            const std::size_t at = pick( line.size() );
            switch( pick( 3 ) ) {
                case 0:
                    line[at] = static_cast<char>( " .[]()\"\\@x0-9et"[pick( 16 )] );
                    break;
                case 1:
                    line.erase( at, 1 );
                    break;
                default:
                    line.insert( at, 1, static_cast<char>( random() ) );
                    break;
            }
        }
        std::array<std::byte, 1024> back_storage{};
        TelegramData back{ back_storage };
        const auto read = read_telegram( line, back );
        if( read ) {
            ++damaged_read;
            assert( read->valid() );
            const std::string canonical = line_of( *read, back );
            TelegramData again{ back_storage };
            again.rewind( 0 );
            std::array<std::byte, 1024> again_storage{};
            TelegramData again_data{ again_storage };
            const auto reread = read_telegram( canonical, again_data );
            assert( reread && line_of( *reread, again_data ) == canonical );
        }
    }

    // ---- nothing allocates -------------------------------------------------------------
    {
        data.clear();
        std::array<char, 256> out{};
        const std::size_t before = allocations;
        const auto read = read_telegram( "state item[17].name \"fur coat\" @3", data );
        assert( read && write_telegram( *read, data.bytes(), names_table, out ) > 0 );
        const auto number = read_telegram( "state counter.ticks 0.1", data );
        assert( number && write_telegram( *number, data.bytes(), names_table, out ) > 0 );
        assert( allocations == before );
    }

    std::printf(
        "telegram: all assertions passed  sizeof(Telegram)=%zu sizeof(Key)=%zu; %zu "
        "random telegrams round-tripped, %zu of 20000 damaged lines read back canonically\n",
        sizeof( Telegram ), sizeof( names::counter_ticks ), lines, damaged_read );
    return 0;
}
