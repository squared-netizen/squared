// The latest-value cache (6.4): every value type back as it went in, text in
// the entry and in the text area, rewritten in place, compacted, the clock's
// order, bound entries kept, full and too large, no allocation; and a long
// random run against a model.

#include <squared/messaging/value_cache.hpp>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <random>
#include <string_view>
#include <utility>
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
            std::fprintf( stderr, "value_cache_test.cpp:%d: %s\n", __LINE__, #condition );         \
            std::abort();                                                                          \
        }                                                                                          \
    } while( false )

// Storage for a cache of N entries and T text bytes.
template<std::size_t N, std::size_t T>
struct Storage
{
        std::array<std::byte, N * 32> entries{};
        std::array<std::byte, ValueCache::index_bytes( N )> index{};
        std::array<std::byte, T> text{};
};

Address at( std::uint32_t uid ) noexcept
{
    return { Name{ "test.cache" }, uid };
}

std::span<const std::byte> bytes_of( std::string_view text ) noexcept
{
    return std::as_bytes( std::span<const char>( text.data(), text.size() ) );
}

std::string_view text_of( std::span<const std::byte> bytes ) noexcept
{
    return { reinterpret_cast<const char *>( bytes.data() ), bytes.size() };
}

Telegram text( Address to ) noexcept
{
    return Telegram::data( Verb::state, to, ValueType::text, {} );
}

std::string_view found_text( const ValueCache &cache, Address address )
{
    const auto got = cache.find( address );
    CHECK( got && got->telegram.type() == ValueType::text );
    CHECK( got->telegram.as_data().offset == 0 &&
           got->telegram.as_data().length == got->data.size() );
    return text_of( got->data );
}

// ---- every value type ---------------------------------------------------------------
void value_types()
{
    Storage<16, 512> s; // a quarter: 128 bytes
    ValueCache cache( s.entries, s.index, s.text );
    CHECK( !cache.find( at( 0 ) ) );

    const std::array<float, 4> floats{ 1.5F, -2.0F, 0.25F, 8.0F };
    const std::array<Telegram, 5> plain{
        Telegram::empty( Verb::state, at( 1 ) ),
        Telegram::boolean( Verb::state, at( 2 ), true ),
        Telegram::integer( Verb::state, at( 3 ), -7 ),
        Telegram::number( Verb::state, at( 4 ), 2.5 ),
        Telegram::floats( Verb::state, at( 5 ), floats ),
    };
    for( const Telegram &t : plain ) {
        CHECK( cache.store( t.with_correlation( 9 ) ) == CacheResult::stored );
    }
    for( const Telegram &t : plain ) {
        const auto got = cache.find( t.address() );
        // The value as it went in; the correlation is not kept.
        CHECK( got && got->telegram == t && got->data.empty() );
    }

    // Text: empty, short, exactly 16 (in the entry), 17 and 100 (in the text area).
    const std::string_view hundred =
        "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567"
        "890123456789";
    const std::array<std::string_view, 5> texts{ "", "fur coat", "0123456789abcdef",
                                                 "0123456789abcdefg", hundred };
    for( std::uint32_t i = 0; i < texts.size(); ++i ) {
        CHECK( cache.store( text( at( 10 + i ) ), bytes_of( texts[i] ) ) == CacheResult::stored );
    }
    CHECK( cache.stats().text_used == ( 4 + 17 ) + ( 4 + 100 ) );
    for( std::uint32_t i = 0; i < texts.size(); ++i ) {
        CHECK( found_text( cache, at( 10 + i ) ) == texts[i] );
    }

    // Uid lists: three (12 bytes, in the entry) and five (20, in the text area).
    const std::array<std::uint32_t, 3> three{ 17, 4, 22 };
    const std::array<std::uint32_t, 5> five{ 1, 2, 3, 4, 5 };
    const Telegram uids = Telegram::data( Verb::state, at( 20 ), ValueType::uids, {} );
    CHECK( cache.store( uids, std::as_bytes( std::span( three ) ) ) == CacheResult::stored );
    auto got = cache.find( at( 20 ) );
    CHECK( got && got->telegram.type() == ValueType::uids && got->data.size() == 12 );
    CHECK( std::memcmp( got->data.data(), three.data(), 12 ) == 0 );
    CHECK( cache.store( uids, std::as_bytes( std::span( five ) ) ) == CacheResult::stored );
    got = cache.find( at( 20 ) );
    CHECK( got && got->data.size() == 20 && std::memcmp( got->data.data(), five.data(), 20 ) == 0 );

    // A value replaced by another type, and back.
    CHECK( cache.store( text( at( 3 ) ), bytes_of( hundred ) ) == CacheResult::stored );
    CHECK( found_text( cache, at( 3 ) ) == hundred );
    CHECK( cache.store( Telegram::integer( Verb::state, at( 3 ), 42 ) ) == CacheResult::stored );
    CHECK( cache.find( at( 3 ) )->telegram.as_integer() == 42 );
    CHECK( found_text( cache, at( 14 ) ) == hundred ); // the others untouched
    std::puts( "value types: every type back as it went in; text in the entry up to 16 bytes" );
}

// ---- text: in place, appended, compacted ----------------------------------------------
void text_room()
{
    Storage<8, 256> s;
    ValueCache cache( s.entries, s.index, s.text );
    const std::string_view forty = "0123456789012345678901234567890123456789";
    CHECK( cache.store( text( at( 1 ) ), bytes_of( forty ) ) == CacheResult::stored );
    CHECK( cache.stats().text_used == 44 );
    // Shorter: in its old room, nothing appended.
    CHECK( cache.store( text( at( 1 ) ), bytes_of( forty.substr( 0, 30 ) ) ) ==
           CacheResult::stored );
    CHECK( cache.stats().text_used == 44 && found_text( cache, at( 1 ) ) == forty.substr( 0, 30 ) );
    // Back to 40: still fits the room.
    CHECK( cache.store( text( at( 1 ) ), bytes_of( forty ) ) == CacheResult::stored );
    CHECK( cache.stats().text_used == 44 );

    // Growing values at three addresses: appended until the area is full,
    // then compacted; every value stays right, and the area never overflows.
    std::array<std::string, 3> latest{};
    std::mt19937 random( 64 ); // NOLINT(cert-msc32-c,cert-msc51-cpp): repeatable on purpose
    for( int round = 0; round < 2000; ++round ) {
        const std::uint32_t which = random() % 3;
        const std::size_t length = 17 + random() % 48; // up to a quarter of 256
        std::string value( length, static_cast<char>( 'a' + round % 26 ) );
        value[0] = static_cast<char>( '0' + which );
        CHECK( cache.store( text( at( 2 + which ) ), bytes_of( value ) ) == CacheResult::stored );
        latest[which] = value;
        for( std::uint32_t i = 0; i < 3; ++i ) {
            if( !latest[i].empty() ) {
                CHECK( found_text( cache, at( 2 + i ) ) == latest[i] );
            }
        }
        CHECK( cache.stats().text_used <= 256 );
    }
    CHECK( found_text( cache, at( 1 ) ) == forty );
    CHECK( cache.stats().evicted == 0 ); // compaction was enough: 4 values fit
    std::puts( "text: rewritten in place when it fits, else appended; compacted when full" );
}

// ---- the clock, binding ---------------------------------------------------------------
void clock_and_binding()
{
    Storage<4, 0> s;
    ValueCache cache( s.entries, s.index, s.text );
    CHECK( cache.bind( at( 0 ) ) ); // entry 0, no value yet
    CHECK( !cache.find( at( 0 ) ) );
    for( std::uint32_t i = 0; i < 4; ++i ) {
        CHECK( cache.store( Telegram::integer( Verb::state, at( i ), i ) ) == CacheResult::stored );
    }
    CHECK( cache.stats().used == 4 && cache.stats().bound == 1 );
    // Full. The hand starts at 0: the bound entry is passed, 1 to 3 lose
    // their marks, the bound one is passed again, and 1 is taken.
    CHECK( cache.store( Telegram::integer( Verb::state, at( 4 ), 4 ) ) == CacheResult::stored );
    CHECK( cache.stats().evicted == 1 );
    CHECK( !cache.find( at( 1 ) ) );
    for( const std::uint32_t uid : { 0U, 2U, 3U, 4U } ) {
        CHECK( cache.find( at( uid ) )->telegram.as_integer() == uid );
    }
    // 2 is written again, so it keeps its mark: 3 goes next.
    CHECK( cache.store( Telegram::integer( Verb::state, at( 2 ), 22 ) ) == CacheResult::stored );
    CHECK( cache.store( Telegram::integer( Verb::state, at( 5 ), 5 ) ) == CacheResult::stored );
    CHECK( !cache.find( at( 3 ) ) && cache.find( at( 2 ) )->telegram.as_integer() == 22 );

    // Every entry bound: a new address is dropped and counted, and cannot be bound.
    Storage<2, 0> t;
    ValueCache full( t.entries, t.index, t.text );
    CHECK( full.bind( at( 1 ) ) && full.bind( at( 2 ) ) );
    CHECK( full.store( Telegram::integer( Verb::state, at( 3 ), 3 ) ) == CacheResult::dropped );
    CHECK( !full.bind( at( 3 ) ) && full.stats().dropped == 1 );
    // A bound address still takes values.
    CHECK( full.store( Telegram::integer( Verb::state, at( 1 ), 1 ) ) == CacheResult::stored );
    // Unbound, it can go: the clock passes it once, then takes it.
    full.unbind( at( 2 ) );
    CHECK( full.stats().bound == 1 );
    CHECK( full.store( Telegram::integer( Verb::state, at( 3 ), 3 ) ) == CacheResult::stored );
    CHECK( full.find( at( 1 ) ) && full.find( at( 3 ) ) );

    // Bound by many: 255 at most; each unbind takes one away.
    Storage<2, 0> u;
    ValueCache many( u.entries, u.index, u.text );
    for( int i = 0; i < 255; ++i ) {
        CHECK( many.bind( at( 7 ) ) );
    }
    CHECK( !many.bind( at( 7 ) ) );
    for( int i = 0; i < 255; ++i ) {
        many.unbind( at( 7 ) );
    }
    CHECK( many.stats().bound == 0 );
    std::puts( "clock: unbound entries taken roughly oldest first; bound ones kept, 255 binds" );
}

// ---- full text, too large -----------------------------------------------------------------
void full_text()
{
    Storage<8, 128> s; // a quarter: 32 bytes
    ValueCache cache( s.entries, s.index, s.text );
    const std::string_view thirty_two = "0123456789abcdef0123456789ABCDEF";
    CHECK( cache.store( text( at( 9 ) ), bytes_of( "0123456789abcdefg" ) ) == CacheResult::stored );
    CHECK( cache.store( text( at( 9 ) ), bytes_of( std::string( 33, 'x' ) ) ) ==
           CacheResult::too_large );
    CHECK( found_text( cache, at( 9 ) ) == "0123456789abcdefg" ); // kept the value it had
    CHECK( cache.store( text( at( 9 ) ), bytes_of( "short" ) ) == CacheResult::stored );

    // Three texts fill 108 of 128 bytes; a fourth needs 36: an unbound
    // text-holder gives its entry up.
    for( std::uint32_t i = 0; i < 3; ++i ) {
        CHECK( cache.store( text( at( i ) ), bytes_of( thirty_two ) ) == CacheResult::stored );
    }
    CHECK( cache.store( text( at( 3 ) ), bytes_of( thirty_two ) ) == CacheResult::stored );
    CHECK( cache.stats().evicted == 1 && found_text( cache, at( 3 ) ) == thirty_two );
    CHECK( found_text( cache, at( 9 ) ) == "short" ); // in its entry: not a text-holder

    // All text bound: dropped, and the address keeps its value.
    Storage<8, 128> t;
    ValueCache bound( t.entries, t.index, t.text );
    for( std::uint32_t i = 0; i < 3; ++i ) {
        CHECK( bound.bind( at( i ) ) );
        CHECK( bound.store( text( at( i ) ), bytes_of( thirty_two ) ) == CacheResult::stored );
    }
    CHECK( bound.store( Telegram::integer( Verb::state, at( 3 ), 3 ) ) == CacheResult::stored );
    CHECK( bound.store( text( at( 3 ) ), bytes_of( thirty_two ) ) == CacheResult::dropped );
    CHECK( bound.find( at( 3 ) )->telegram.as_integer() == 3 && bound.stats().dropped == 1 );

    // No text area: text up to 16 bytes only.
    Storage<4, 0> u;
    ValueCache bare( u.entries, u.index, u.text );
    CHECK( bare.store( text( at( 1 ) ), bytes_of( "0123456789abcdef" ) ) == CacheResult::stored );
    CHECK( bare.store( text( at( 1 ) ), bytes_of( "0123456789abcdefg" ) ) ==
           CacheResult::too_large );
    std::puts( "full text: unbound text-holders give way, bound ones do not; too large refused" );
}

// The pending table's walk (6.6a): bindings, and the bound entries in order.
void bound_walk()
{
    Storage<8, 256> s;
    ValueCache cache( s.entries, s.index, s.text );
    CHECK( cache.bindings( at( 1 ) ) == 0 );
    CHECK( !cache.bound_from( 0 ) );
    CHECK( cache.store( Telegram::integer( Verb::state, at( 1 ), 1 ) ) == CacheResult::stored );
    CHECK( cache.bind( at( 2 ) ) && cache.bind( at( 2 ) ) ); // twice, no value yet
    CHECK( cache.store( text( at( 3 ) ), bytes_of( "a text longer than sixteen" ) ) ==
           CacheResult::stored );
    CHECK( cache.bind( at( 3 ) ) );
    CHECK( cache.bindings( at( 1 ) ) == 0 && cache.bindings( at( 2 ) ) == 2 );
    CHECK( cache.bindings( at( 3 ) ) == 1 && cache.bindings( at( 9 ) ) == 0 );

    // Bound: 2 (entry 1, no value) and 3 (entry 2); 1 is not.
    auto found = cache.bound_from( 0 );
    CHECK( found && found->entry == 1 && found->address == at( 2 ) && !found->value );
    found = cache.bound_from( found->entry + 1 );
    CHECK( found && found->entry == 2 && found->address == at( 3 ) && found->value );
    CHECK( text_of( found->value->data ) == "a text longer than sixteen" );
    // Unbinding the one found does not disturb the walk.
    cache.unbind( at( 3 ) );
    CHECK( !cache.bound_from( found->entry + 1 ) );
    CHECK( cache.bound_from( 0 )->address == at( 2 ) );
    std::puts( "bound walk: bindings counted, bound entries found in order" );
}

void no_allocation()
{
    Storage<64, 1024> s;
    ValueCache cache( s.entries, s.index, s.text );
    const std::size_t before = allocations;
    for( std::uint32_t i = 0; i < 2000; ++i ) {
        CHECK( cache.store( Telegram::integer( Verb::state, at( i % 100 ), i ) ) ==
               CacheResult::stored );
        const auto thirty = bytes_of( "a text of thirty bytes, more" );
        CHECK( cache.store( text( at( 200 + i % 50 ) ), thirty ) == CacheResult::stored );
        CHECK( cache.bind( at( i % 7 ) ) );
        cache.unbind( at( i % 7 ) );
        static_cast<void>( cache.find( at( i % 100 ) ) );
    }
    CHECK( allocations == before );
    std::puts( "no allocation: 2,000 rounds of store, bind, unbind and find" );
}

// ---- a long random run, against a model -----------------------------------------------------
struct Value
{
        Telegram telegram = Telegram::empty( Verb::state, {} );
        std::vector<std::byte> data;
};

void against_a_model()
{
    constexpr std::uint32_t entries = 32;
    Storage<entries, 512> s; // a quarter: 128 bytes
    ValueCache cache( s.entries, s.index, s.text );
    std::map<std::uint32_t, Value> latest; // by uid: the last value stored
    std::map<std::uint32_t, int> binds;    // by uid: how many binds
    std::map<std::uint32_t, bool> kept;    // stored while bound: must be found
    std::mt19937 random( 20261020 ); // NOLINT(cert-msc32-c,cert-msc51-cpp): repeatable on purpose
    std::size_t stored = 0;
    std::size_t dropped = 0;
    const Name names[2] = { Name{ "model.a" }, Name{ "model.b" } };
    const auto address = [&]( std::uint32_t key ) { return Address{ names[key % 2], key / 2 }; };
    const auto bound_count = [&] {
        std::uint32_t n = 0;
        for( const auto &[k, count] : binds ) {
            n += count > 0 ? 1 : 0;
        }
        return n;
    };

    for( int step = 0; step < 300000; ++step ) {
        const std::uint32_t key = random() % 120; // 120 addresses over 32 entries
        const Address to = address( key );
        const std::uint32_t op = random() % 10;
        // The last third only binds, so the cache fills with bound entries
        // and the dropped paths run too.
        const bool filling = step >= 200000;
        if( op < 7 ) {
            Telegram telegram = Telegram::integer( Verb::state, to, step );
            std::vector<std::byte> data;
            const std::uint32_t kind = random() % 3;
            if( kind == 1 ) {
                data.resize( random() % 129 );
                for( std::size_t i = 0; i < data.size(); ++i ) {
                    data[i] = static_cast<std::byte>( ( step + i ) & 0xFF );
                }
                telegram = text( to );
            } else if( kind == 2 ) {
                data.resize( ( random() % 33 ) * 4 );
                for( std::size_t i = 0; i < data.size(); ++i ) {
                    data[i] = static_cast<std::byte>( ( step * 7 + i ) & 0xFF );
                }
                telegram = Telegram::data( Verb::state, to, ValueType::uids, {} );
            }
            const bool was_known = cache.find( to ).has_value() || binds[key] > 0;
            const CacheResult result = cache.store( telegram, data );
            CHECK( result != CacheResult::too_large );
            if( result == CacheResult::stored ) {
                latest[key] = { telegram, data };
                kept[key] = binds[key] > 0;
                ++stored;
            } else {
                // Dropped only for want of room: a new address with every
                // entry bound, or long text with the text area held by others.
                CHECK( data.size() > 16 || ( !was_known && bound_count() == entries ) );
                ++dropped;
            }
        } else if( op == 7 || filling ) { // binds a tenth of the time, unbinds a fifth
            if( binds[key] < 3 && cache.bind( to ) ) {
                ++binds[key];
            } else if( binds[key] < 3 ) {
                CHECK( bound_count() == entries );
            }
        } else {
            // Unbind one of the addresses that are bound, so binds do not pile up.
            std::vector<std::uint32_t> bound;
            for( const auto &[k, n] : binds ) {
                if( n > 0 ) {
                    bound.push_back( k );
                }
            }
            if( !bound.empty() ) {
                const std::uint32_t k = bound[random() % bound.size()];
                cache.unbind( address( k ) );
                if( --binds[k] == 0 ) {
                    kept[k] = false;
                }
            }
        }

        if( step % 97 == 0 ) {
            for( std::uint32_t k = 0; k < 120; ++k ) {
                const auto got = cache.find( address( k ) );
                if( got ) {
                    // Whatever is found is the latest value stored.
                    const Value &want = latest.at( k );
                    CHECK( got->telegram.type() == want.telegram.type() );
                    if( want.data.empty() && want.telegram.type() == ValueType::integer ) {
                        CHECK( got->telegram == want.telegram );
                    }
                    CHECK( got->data.size() == want.data.size() );
                    CHECK( std::equal( got->data.begin(), got->data.end(), want.data.begin() ) );
                } else {
                    // A value stored while bound, and still bound, is never lost.
                    CHECK( !kept[k] );
                }
            }
            const CacheStats stats = cache.stats();
            CHECK( stats.used <= entries && stats.bound == bound_count() );
            CHECK( stats.text_used <= 512 );
        }
    }
    std::printf( "against a model: 300,000 random steps over 120 addresses and 32 entries, %zu "
                 "stored, %zu dropped for room, %u evicted; every value found is the latest, "
                 "bound ones never lost\n",
                 stored, dropped, cache.stats().evicted );
}

// Many addresses on one index: everything inserted is found, through
// collisions and evictions (the backward shift).
void index_stress()
{
    constexpr std::uint32_t entries = 1024;
    static Storage<entries, 0> s;
    ValueCache cache( s.entries, s.index, s.text );
    for( std::uint32_t round = 0; round < 8; ++round ) {
        for( std::uint32_t i = 0; i < entries; ++i ) {
            const std::uint32_t uid = round * entries + i;
            CHECK( cache.store( Telegram::integer( Verb::state, at( uid ), uid ) ) ==
                   CacheResult::stored );
        }
        // The round just stored is all there; earlier rounds were evicted.
        for( std::uint32_t i = 0; i < entries; ++i ) {
            const std::uint32_t uid = round * entries + i;
            const auto got = cache.find( at( uid ) );
            CHECK( got && got->telegram.as_integer() == uid );
        }
        CHECK( cache.stats().used == entries );
    }
    std::puts( "index: 8 rounds of 1,024 addresses on 1,024 entries, each round all found" );
}

} // namespace

int main()
{
    static_assert( sizeof( ValueCache ) == 88 );
    value_types();
    text_room();
    clock_and_binding();
    full_text();
    bound_walk();
    no_allocation();
    against_a_model();
    index_stress();
    std::puts( "value_cache_test: ok" );
    return 0;
}
