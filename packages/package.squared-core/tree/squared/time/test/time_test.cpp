// The time module (6.6b): Timepiece with times passed in (the first tick,
// steps, the clamp, a time that goes back, start), the two clocks, and
// DeadlineQueue: order, the full and invalid cases, limits, cancelling,
// next_due, the ordered walk, no allocation, and 200,000 random steps
// against a model.

#include <squared/time/time.hpp>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <random>
#include <string>
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

using namespace sq::time;
using namespace std::chrono_literals;

namespace
{

// A test fails loudly, with its line, in any build.
#define CHECK( condition )                                                                         \
    do {                                                                                           \
        if( !( condition ) ) {                                                                     \
            std::fprintf( stderr, "time_test.cpp:%d: %s\n", __LINE__, #condition );                \
            std::abort();                                                                          \
        }                                                                                          \
    } while( false )

TimePoint at( Duration since_epoch ) noexcept
{
    return since_epoch;
}

void timepiece()
{
    Timepiece time;
    CHECK( !time.started() && time.ticks() == 0 );
    CHECK( time.tick( at( 5s ) ) == 0.0 ); // nothing to measure from
    CHECK( time.started() && time.ticks() == 1 && time.last() == at( 5s ) );
    CHECK( time.tick( at( 5s + 16ms ) ) == 0.016 );
    CHECK( time.step() == 16ms && time.ticks() == 2 );
    CHECK( time.tick( at( 5s + 16ms ) ) == 0.0 ); // the same time again: no step
    CHECK( time.tick( at( 9s ) ) == 0.25 );       // a stall: clamped
    CHECK( time.step() == Timepiece::longest_step && time.last() == at( 9s ) );
    CHECK( time.tick( at( 9s + 10ms ) ) == 0.01 ); // measured from the stall's end
    CHECK( time.tick( at( 8s ) ) == 0.0 );         // a time that went back
    CHECK( time.last() == at( 8s ) && time.ticks() == 6 );
    CHECK( time.tick( at( 8s + 1ms ) ) == 0.001 );

    time.start( at( 20s ) );
    CHECK( time.ticks() == 0 && time.step() == Duration::zero() );
    CHECK( time.tick( at( 20s + 100ms ) ) == 0.1 ); // started: the first tick measures
    CHECK( time.seconds() == 0.1 && time.ticks() == 1 );

    // The epoch itself is a time like any other.
    Timepiece zero;
    CHECK( zero.tick( at( 0s ) ) == 0.0 && zero.started() );
    CHECK( zero.tick( at( 40ms ) ) == 0.04 );
}

void clocks()
{
    const TimePoint first = now();
    const TimePoint boot = since_boot();
    const TimePoint second = now();
    CHECK( first.count() > 0 && second >= first );
    CHECK( boot >= first ); // boot time counts deep sleep too, so it is never behind
    for( int i = 0; i < 1000; ++i ) {
        const TimePoint later = now();
        CHECK( later >= second );
    }
}

using Queue = DeadlineQueue<int>;

void queue_basics()
{
    std::array<Queue::Entry, 4> storage{};
    Queue queue( storage );
    CHECK( queue.empty() && queue.capacity() == 4 && !queue.next_due() );

    const auto a = queue.schedule_at( at( 30ms ), 1 );
    const auto b = queue.schedule_at( at( 10ms ), 2 );
    const auto c = queue.schedule_at( at( 30ms ), 3 ); // same time as a: after it
    const auto d = queue.schedule_after( at( 5ms ), 15ms, 4 );
    CHECK( a && b && c && d && queue.size() == 4 );
    CHECK( a.ticket.value == 1 && d.ticket.value == 4 );
    CHECK( queue.next_due() == at( 10ms ) );
    CHECK( queue.schedule_at( at( 1ms ), 5 ).result == Queue::ScheduleResult::full );
    CHECK( !queue.schedule_at( at( 1ms ), 5 ).ticket );

    std::vector<int> order;
    order.reserve( 8 );
    queue.visit_ordered( [&]( TimePoint, Queue::Ticket, int value ) { order.push_back( value ); } );
    CHECK( ( order == std::vector<int>{ 2, 4, 1, 3 } ) );

    order.clear();
    const auto take = [&]( int value ) { order.push_back( value ); };
    CHECK( queue.poll_due( at( 9ms ), 10, take ) == 0 ); // nothing due yet
    CHECK( queue.poll_due( at( 30ms ), 2, take ) == 2 ); // a limit of 2
    CHECK( ( order == std::vector<int>{ 2, 4 } ) && queue.size() == 2 );
    CHECK( queue.next_due() == at( 30ms ) );

    CHECK( queue.cancel( a.ticket ) );
    CHECK( !queue.cancel( a.ticket ) ); // gone already
    CHECK( !queue.cancel( b.ticket ) ); // given back already
    CHECK( !queue.cancel( Queue::Ticket{} ) );
    CHECK( queue.poll_due( at( 1s ), 10, take ) == 1 );
    CHECK( ( order == std::vector<int>{ 2, 4, 3 } ) && queue.empty() && !queue.next_due() );

    // Times: none before the epoch, none past the largest.
    using Result = Queue::ScheduleResult;
    const TimePoint largest = TimePoint::max();
    CHECK( queue.schedule_at( at( -1ns ), 0 ).result == Result::invalid_time );
    CHECK( queue.schedule_after( at( 1s ), -1ns, 0 ).result == Result::invalid_time );
    CHECK( queue.schedule_after( at( -1s ), 1ns, 0 ).result == Result::invalid_time );
    CHECK( queue.schedule_after( largest, 1ns, 0 ).result == Result::invalid_time );
    CHECK( queue.schedule_after( largest - 1ns, 1ns, 0 ) ); // exactly the largest
    CHECK( queue.next_due() == largest );
}

// A value that owns memory: cancelling and polling release it, and the slot
// left behind holds nothing.
void queue_owning_values()
{
    std::array<DeadlineQueue<std::string>::Entry, 3> storage{};
    DeadlineQueue<std::string> queue( storage );
    const std::string long_text( 100, 'x' ); // past any short-string buffer
    const auto first = queue.schedule_at( at( 2s ), long_text );
    CHECK( queue.schedule_at( at( 1s ), "one" ) );
    std::string seen;
    CHECK( queue.cancel( first.ticket, [&]( std::string &value ) { seen = std::move( value ); } ) );
    CHECK( seen == long_text );
    for( const auto &entry : storage ) {
        CHECK( entry.value.size() <= 3 ); // the cancelled text is not left in a slot
    }
    const auto keep = [&]( std::string value ) { seen = std::move( value ); };
    CHECK( queue.poll_due( at( 5s ), 5, keep ) == 1 );
    CHECK( seen == "one" );
    for( const auto &entry : storage ) {
        CHECK( entry.value.empty() );
    }
}

// A value whose move leaves the source as it was (any trivially copyable
// type does): the slot a value leaves is still cleared, so nothing a value
// refers to is kept alive by the storage.
void queue_clears_slots()
{
    struct Sticky
    {
            int id = 0;
    };
    std::array<DeadlineQueue<Sticky>::Entry, 4> storage{};
    DeadlineQueue<Sticky> queue( storage );
    const auto kept = queue.schedule_at( at( 1s ), Sticky{ 1 } );
    CHECK( queue.schedule_at( at( 2s ), Sticky{ 2 } ) && kept );
    CHECK( queue.schedule_at( at( 3s ), Sticky{ 3 } ) );
    CHECK( queue.cancel( kept.ticket ) );
    int sum = 0;
    CHECK( queue.poll_due( at( 5s ), 5, [&]( Sticky value ) { sum += value.id; } ) == 2 );
    CHECK( sum == 5 );
    for( const auto &entry : storage ) {
        CHECK( entry.value.id == 0 );
    }
}

void queue_allocates_nothing()
{
    std::array<Queue::Entry, 64> storage{};
    Queue queue( storage );
    const std::size_t before = allocations;
    long long sum = 0;
    for( int round = 0; round < 100; ++round ) {
        for( int i = 0; i < 64; ++i ) {
            CHECK( queue.schedule_at( at( std::chrono::milliseconds( ( i * 37 ) % 64 ) ), i ) );
        }
        queue.visit_ordered( [&]( TimePoint, Queue::Ticket, int value ) { sum += value; } );
        const auto refused = queue.schedule_at( at( 1ms ), 0 ); // full
        CHECK( !refused && !queue.cancel( refused.ticket ) );
        CHECK( queue.poll_due( at( 1s ), 64, [&]( int value ) { sum += value; } ) == 64 );
    }
    CHECK( allocations == before );
    CHECK( sum == 100LL * 2 * ( 63 * 64 / 2 ) );
}

// Random schedules, cancels and polls against a std::map keyed by (due,
// sequence): the same values come back in the same order.
void queue_against_a_model()
{
    std::array<Queue::Entry, 32> storage{};
    Queue queue( storage );
    std::map<std::pair<long long, std::uint64_t>, int> model;
    std::vector<Queue::Ticket> tickets;
    std::mt19937 engine( 7 );
    // 32 bits a draw, whatever the engine's result type is on this library.
    const auto random = [&engine] { return static_cast<std::uint32_t>( engine() ); };
    long long clock = 0;
    for( int step = 0; step < 200000; ++step ) {
        const unsigned choice = random() % 10;
        if( choice < 5 ) {
            const long long due = clock + static_cast<long long>( random() % 50 );
            const int value = static_cast<int>( random() % 1000 );
            const TimePoint when = at( std::chrono::nanoseconds( due ) );
            const auto scheduled = queue.schedule_at( when, value );
            if( model.size() == queue.capacity() ) {
                CHECK( scheduled.result == Queue::ScheduleResult::full );
            } else {
                CHECK( scheduled );
                model[{ due, scheduled.ticket.value }] = value;
                tickets.push_back( scheduled.ticket );
            }
        } else if( choice < 7 && !tickets.empty() ) {
            const std::size_t pick = random() % tickets.size();
            const Queue::Ticket ticket = tickets[pick];
            tickets[pick] = tickets.back();
            tickets.pop_back();
            bool known = false;
            for( auto it = model.begin(); it != model.end(); ++it ) {
                if( it->first.second == ticket.value ) {
                    model.erase( it );
                    known = true;
                    break;
                }
            }
            CHECK( queue.cancel( ticket ) == known );
        } else {
            clock += static_cast<long long>( random() % 20 );
            const std::size_t limit = random() % 6;
            std::vector<int> got;
            const auto take = [&]( int value ) { got.push_back( value ); };
            queue.poll_due( at( std::chrono::nanoseconds( clock ) ), limit, take );
            std::vector<int> expected;
            const auto due_now = [&] { return model.begin()->first.first <= clock; };
            while( !model.empty() && expected.size() < limit && due_now() ) {
                expected.push_back( model.begin()->second );
                model.erase( model.begin() );
            }
            CHECK( got == expected );
        }
        CHECK( queue.size() == model.size() );
        if( model.empty() ) {
            CHECK( !queue.next_due() );
        } else {
            const TimePoint first = at( std::chrono::nanoseconds( model.begin()->first.first ) );
            CHECK( queue.next_due() == first );
        }
        if( step % 1000 == 0 ) {
            std::vector<int> walked;
            const auto walk = [&]( TimePoint, Queue::Ticket, int value ) {
                walked.push_back( value );
            };
            queue.visit_ordered( walk );
            std::vector<int> expected;
            for( const auto &[key, value] : model ) {
                expected.push_back( value );
            }
            CHECK( walked == expected );
        }
    }
}

} // namespace

int main()
{
    timepiece();
    clocks();
    queue_basics();
    queue_owning_values();
    queue_clears_slots();
    queue_allocates_nothing();
    queue_against_a_model();
    const std::size_t entry = sizeof( Queue::Entry );
    std::printf( "time_test: ok (sizeof(Timepiece) %zu bytes", sizeof( Timepiece ) );
    std::printf( ", DeadlineQueue<int> %zu, %zu an entry)\n", sizeof( Queue ), entry );
    return 0;
}
