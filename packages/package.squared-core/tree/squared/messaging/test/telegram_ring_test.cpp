// The Telegraph's ring (6.3): every value type through it, full slots and a
// full data ring, a value that skips the end, too large, no allocation; a
// long random run against a model; and two real threads, the consumer
// checking every telegram's order and bytes.

#include <squared/messaging/telegram_ring.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <random>
#include <string_view>
#include <thread>
#include <vector>

namespace
{
std::atomic<std::size_t> allocations{ 0 };
} // namespace

void *operator new( std::size_t size )
{
    allocations.fetch_add( 1, std::memory_order_relaxed );
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
            std::fprintf( stderr, "telegram_ring_test.cpp:%d: %s\n", __LINE__, #condition );       \
            std::abort();                                                                          \
        }                                                                                          \
    } while( false )

constexpr Address at( std::uint32_t uid ) noexcept
{
    return { Name{ "test.ring" }, uid };
}

std::span<const std::byte> bytes_of( std::string_view text ) noexcept
{
    return std::as_bytes( std::span<const char>( text.data(), text.size() ) );
}

std::string_view text_of( std::span<const std::byte> bytes ) noexcept
{
    return { reinterpret_cast<const char *>( bytes.data() ), bytes.size() };
}

Telegram text( std::uint32_t uid ) noexcept
{
    return Telegram::data( Verb::state, at( uid ), ValueType::text, {} );
}

// ---- every value type, in order --------------------------------------------------------
void value_types()
{
    std::array<std::byte, 8 * 32> slots{};
    std::array<std::byte, 256> data{};
    TelegramRing ring( slots, data );
    CHECK( !ring.front() );

    const std::array<float, 3> floats{ 1.5F, -2.0F, 0.25F };
    const std::array<std::uint32_t, 3> uids{ 17, 4, 22 };
    const std::array<Telegram, 7> sent{
        Telegram::empty( Verb::event, at( 0 ) ),
        Telegram::boolean( Verb::set, at( 1 ), true ),
        Telegram::integer( Verb::state, at( 2 ), -7 ),
        Telegram::number( Verb::state, at( 3 ), 2.5 ).with_correlation( 9 ),
        Telegram::floats( Verb::state, at( 4 ), floats ),
        Telegram::data( Verb::do_, at( 5 ), ValueType::text, {} ).with_correlation( 300 ),
        Telegram::data( Verb::state, at( 6 ), ValueType::uids, {} ),
    };
    CHECK( ring.push( sent[0] ) == RingResult::sent );
    CHECK( ring.push( sent[1] ) == RingResult::sent );
    CHECK( ring.push( sent[2] ) == RingResult::sent );
    CHECK( ring.push( sent[3] ) == RingResult::sent );
    CHECK( ring.push( sent[4] ) == RingResult::sent );
    CHECK( ring.push( sent[5], bytes_of( "notes.txt" ) ) == RingResult::sent );
    CHECK( ring.push( sent[6], std::as_bytes( std::span( uids ) ) ) == RingResult::sent );
    // Empty text takes no room in the data ring.
    CHECK( ring.push( text( 7 ), {} ) == RingResult::sent );

    for( std::size_t i = 0; i < 5; ++i ) {
        const auto got = ring.front();
        CHECK( got && got->telegram == sent[i] && got->data.empty() );
        ring.pop();
    }
    auto got = ring.front();
    CHECK( got && got->telegram.verb() == Verb::do_ && got->telegram.correlation() == 300 );
    CHECK( got->telegram.address() == at( 5 ) && text_of( got->data ) == "notes.txt" );
    CHECK( got->telegram.as_data().offset == 0 && got->telegram.as_data().length == 9 );
    ring.pop();
    got = ring.front();
    CHECK( got && got->telegram.type() == ValueType::uids && got->data.size() == 12 );
    std::array<std::uint32_t, 3> back{};
    std::memcpy( back.data(), got->data.data(), 12 );
    CHECK( back == uids );
    ring.pop();
    got = ring.front();
    CHECK( got && got->telegram.address() == at( 7 ) && got->data.empty() );
    ring.pop();
    CHECK( !ring.front() );

    const RingStats stats = ring.stats();
    CHECK( stats.slots == 8 && stats.data_bytes == 256 );
    CHECK( stats.highest_slots == 8 && stats.highest_data == 9 + 12 );
    std::puts( "value types: 8 telegrams in order, text and uids intact" );
}

// ---- full ------------------------------------------------------------------------------
void full_slots()
{
    std::array<std::byte, 4 * 32> slots{};
    TelegramRing ring( slots, {} );
    for( std::uint32_t i = 0; i < 4; ++i ) {
        CHECK( ring.push( Telegram::integer( Verb::state, at( i ), i ) ) == RingResult::sent );
    }
    CHECK( ring.push( Telegram::integer( Verb::state, at( 9 ), 9 ) ) == RingResult::full );
    // Full wrote nothing: the front is the first, and the fourth is last.
    CHECK( ring.front()->telegram.address() == at( 0 ) );
    ring.pop();
    CHECK( ring.push( Telegram::integer( Verb::state, at( 4 ), 4 ) ) == RingResult::sent );
    for( std::uint32_t i = 1; i <= 4; ++i ) {
        CHECK( ring.front()->telegram.as_integer() == i );
        ring.pop();
    }
    CHECK( !ring.front() );
    std::puts( "full slots: the fifth of 4 refused, nothing written; room again after a pop" );
}

void full_data()
{
    std::array<std::byte, 16 * 32> slots{};
    std::array<std::byte, 64> data{};
    TelegramRing ring( slots, data );
    const std::string_view sixteen = "0123456789abcdef";
    for( std::uint32_t i = 0; i < 4; ++i ) {
        CHECK( ring.push( text( i ), bytes_of( sixteen ) ) == RingResult::sent );
    }
    CHECK( ring.push( text( 9 ), bytes_of( "x" ) ) == RingResult::full );
    // A telegram with no data still goes: the slots have room.
    CHECK( ring.push( Telegram::boolean( Verb::set, at( 5 ), false ) ) == RingResult::sent );
    ring.pop();
    CHECK( ring.push( text( 6 ), bytes_of( sixteen ) ) == RingResult::sent );
    CHECK( ring.stats().highest_data == 64 );
    std::puts( "full data: 64 of 64 bytes held, text refused, other telegrams still sent" );
}

void skip_the_end()
{
    std::array<std::byte, 16 * 32> slots{};
    std::array<std::byte, 64> data{};
    TelegramRing ring( slots, data );
    const std::string_view ten = "abcdefghij";
    for( std::uint32_t i = 0; i < 5; ++i ) { // bytes 0..50
        CHECK( ring.push( text( i ), bytes_of( ten ) ) == RingResult::sent );
    }
    for( int i = 0; i < 3; ++i ) { // the consumer is at 30
        ring.pop();
    }
    // 16 bytes do not fit in 50..64: the last 14 are skipped and the value
    // starts at 0. Held: 20 (two values) + 14 (skipped) + 16 = 50.
    const std::string_view sixteen = "ponmlkjihgfedcba";
    CHECK( ring.push( text( 5 ), bytes_of( sixteen ) ) == RingResult::sent );
    CHECK( ring.stats().highest_data == 50 );
    // 12 bytes more: 16..28 is free (the consumer is at 30).
    CHECK( ring.push( text( 6 ), bytes_of( "123456789012" ) ) == RingResult::sent );
    // and 3 more would reach 31, past the consumer.
    CHECK( ring.push( text( 7 ), bytes_of( "xyz" ) ) == RingResult::full );

    CHECK( text_of( ring.front()->data ) == ten );
    ring.pop();
    CHECK( text_of( ring.front()->data ) == ten );
    ring.pop();
    const auto skipped = ring.front();
    CHECK( skipped->telegram.as_data().offset == 0 && text_of( skipped->data ) == sixteen );
    ring.pop();
    CHECK( text_of( ring.front()->data ) == "123456789012" );
    ring.pop();
    CHECK( !ring.front() );
    // Everything popped: the consumer is at 28. Three 16-byte values fit
    // (28, 44, then 0, skipping 60..64); a fourth would reach the consumer.
    for( std::uint32_t i = 0; i < 3; ++i ) {
        CHECK( ring.push( text( 10 + i ), bytes_of( sixteen ) ) == RingResult::sent );
    }
    CHECK( ring.push( text( 13 ), bytes_of( sixteen ) ) == RingResult::full );
    CHECK( ring.front()->telegram.as_data().offset == 28 );
    std::puts( "skip the end: a value that would cross it starts at 0; the consumer follows" );
}

void too_large()
{
    std::array<std::byte, 4 * 32> slots{};
    std::array<std::byte, 64> data{};
    TelegramRing ring( slots, data );
    const std::string_view seventeen = "0123456789abcdefg";
    CHECK( ring.push( text( 0 ), bytes_of( seventeen ) ) == RingResult::too_large );
    CHECK( ring.push( text( 0 ), bytes_of( seventeen.substr( 1 ) ) ) == RingResult::sent );

    // A ring with no data ring carries empty text and nothing longer.
    TelegramRing bare( slots, {} );
    CHECK( bare.push( text( 1 ), {} ) == RingResult::sent );
    CHECK( bare.push( text( 2 ), bytes_of( "a" ) ) == RingResult::too_large );
    std::puts( "too large: more than a quarter of the data ring refused, for good" );
}

void no_allocation()
{
    std::array<std::byte, 8 * 32> slots{};
    std::array<std::byte, 256> data{};
    TelegramRing ring( slots, data );
    const std::size_t before = allocations.load();
    for( std::uint32_t i = 0; i < 1000; ++i ) {
        CHECK( ring.push( text( i ), bytes_of( "some text" ) ) == RingResult::sent );
        CHECK( ring.push( Telegram::integer( Verb::state, at( i ), i ) ) == RingResult::sent );
        CHECK( ring.front() );
        ring.pop();
        ring.pop();
        CHECK( ring.stats().slots == 8 );
    }
    CHECK( allocations.load() == before );
    std::puts( "no allocation: 2,000 pushes and pops" );
}

// ---- a long random run, against a model -----------------------------------------------
struct Expected
{
        std::uint32_t uid;
        std::vector<std::byte> data;
};

std::vector<std::byte> pattern( std::uint32_t seed, std::size_t length )
{
    std::vector<std::byte> bytes( length );
    for( std::size_t i = 0; i < length; ++i ) {
        bytes[i] = static_cast<std::byte>( ( seed * 31U + i * 7U ) & 0xFFU );
    }
    return bytes;
}

bool same( std::span<const std::byte> got, const std::vector<std::byte> &expected )
{
    return std::equal( got.begin(), got.end(), expected.begin(), expected.end() );
}

void against_a_model()
{
    std::array<std::byte, 16 * 32> slots{};
    std::array<std::byte, 128> data{};
    TelegramRing ring( slots, data );
    std::deque<Expected> model;
    std::mt19937 random( 20261020 ); // NOLINT(cert-msc32-c,cert-msc51-cpp): repeatable on purpose
    std::uint32_t next = 0;
    std::size_t sent = 0;
    std::size_t refused = 0;
    for( int step = 0; step < 200000; ++step ) {
        if( random() % 2 == 0 ) {
            const std::size_t length = random() % 33; // up to a quarter of 128
            const std::vector<std::byte> bytes = pattern( next, length );
            const RingResult result = ring.push( text( next ), bytes );
            if( result == RingResult::sent ) {
                model.push_back( { next, bytes } );
                ++sent;
            } else {
                CHECK( result == RingResult::full );
                // Full only with something waiting: an empty ring takes any
                // value up to a quarter of it.
                CHECK( !model.empty() );
                ++refused;
            }
            ++next;
        } else {
            const auto got = ring.front();
            CHECK( got.has_value() == !model.empty() );
            if( got ) {
                CHECK( got->telegram.address().uid == model.front().uid );
                CHECK( same( got->data, model.front().data ) );
                ring.pop();
                model.pop_front();
            }
        }
    }
    std::printf( "against a model: 200,000 random steps, %zu sent, %zu refused as full, all in "
                 "order and intact\n",
                 sent, refused );
}

// ---- two threads ---------------------------------------------------------------------
void two_threads()
{
    constexpr std::uint32_t count = 200000;
    std::array<std::byte, 16 * 32> slots{};
    std::array<std::byte, 256> data{};
    TelegramRing ring( slots, data );

    std::atomic<std::uint32_t> full{ 0 };
    std::thread producer( [&] {
        std::uint32_t refused = 0;
        for( std::uint32_t i = 0; i < count; ++i ) {
            // Every third an integer, the rest text or a uid list of up to
            // 64 bytes, a quarter of the ring.
            const std::vector<std::byte> bytes =
                i % 3 == 0 ? std::vector<std::byte>{}
                           : pattern( i, i % 3 == 1 ? i % 65 : ( i % 17 ) * 4 );
            const Telegram telegram =
                i % 3 == 0 ? Telegram::integer( Verb::state, at( i ), i )
                           : Telegram::data( Verb::state, at( i ),
                                             i % 3 == 1 ? ValueType::text : ValueType::uids, {} );
            while( ring.push( telegram, bytes ) == RingResult::full ) {
                ++refused;
                std::this_thread::yield();
            }
        }
        full.store( refused );
    } );

    std::uint32_t received = 0;
    while( received < count ) {
        const auto got = ring.front();
        if( !got ) {
            std::this_thread::yield();
            continue;
        }
        const std::uint32_t i = received;
        CHECK( got->telegram.address().uid == i );
        if( i % 3 == 0 ) {
            CHECK( got->telegram.as_integer() == i && got->data.empty() );
        } else {
            const std::vector<std::byte> bytes = pattern( i, i % 3 == 1 ? i % 65 : ( i % 17 ) * 4 );
            CHECK( same( got->data, bytes ) );
        }
        ring.pop();
        ++received;
    }
    producer.join();
    CHECK( !ring.front() );
    const RingStats stats = ring.stats();
    CHECK( stats.highest_slots <= 16 && stats.highest_data <= 256 );
    std::printf( "two threads: %u telegrams in order and intact; the producer found it full %u "
                 "times; highest fill %u of 16 slots, %u of 256 bytes\n",
                 count, full.load(), stats.highest_slots, stats.highest_data );
}

} // namespace

int main()
{
    static_assert( sizeof( TelegramRing ) == 192 );
    value_types();
    full_slots();
    full_data();
    skip_the_end();
    too_large();
    no_allocation();
    against_a_model();
    two_threads();
    std::puts( "telegram_ring_test: ok" );
    return 0;
}
