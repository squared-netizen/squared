// The Telegraph on two threads (6.6c): the model on its own, the UI sleeping
// on its wake descriptor as a looper would. Round trips with no wakeup lost,
// a model that sleeps rather than spins, ticks, the bounded wait (ended by
// its timeout, by a drain, by quit), the order an event keeps behind held
// states, the lifecycle waking the model, quit, the inline runner, and
// timers (6.6b) on both. Run
// under ThreadSanitizer on desktop Linux before each package (Android's
// clang ships no ThreadSanitizer runtime).
//
// Every wait here has a timeout, so a lost wakeup fails a CHECK instead of
// hanging the test.

#include <squared/messaging/model_runner.hpp>
#include <squared/time/deadline_queue.hpp>

#include <poll.h>
#include <sys/prctl.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <thread>

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
// posix_memalign, not aligned_alloc: Android's C library has aligned_alloc
// only from API 28 (see telegraph_test.cpp).
void *operator new( std::size_t size, std::align_val_t align )
{
    allocations.fetch_add( 1, std::memory_order_relaxed );
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
            std::fprintf( stderr, "telegraph_thread_test.cpp:%d: %s\n", __LINE__, #condition );    \
            std::abort();                                                                          \
        }                                                                                          \
    } while( false )

using Clock = std::chrono::steady_clock;
using sq::time::TimePoint;
using namespace std::chrono_literals;
using Milliseconds = std::chrono::milliseconds;

// Generous: ThreadSanitizer is slow, and the phone may be busy.
constexpr int lost_wakeup_ms = 3000;

namespace names
{
inline constexpr Key<std::int64_t, state | set> echo{ "test.echo" };
inline constexpr Key<std::int64_t, do_> flood{ "test.flood" };
inline constexpr Key<std::int64_t, event> flooded{ "test.flooded" };
inline constexpr Key<std::int64_t, do_> round{ "test.round" };
inline constexpr Key<std::int64_t, event> round_done{ "test.round_done" };
inline constexpr Key<std::int64_t, do_> fill{ "test.fill" };
inline constexpr Key<std::int64_t, state, with_uid> cell{ "test.cell" };
inline constexpr Key<std::int64_t, state> ticks{ "test.ticks" };
inline constexpr Key<void, event> pinged{ "test.pinged" };
inline constexpr Key<std::int64_t, do_> after{ "test.after" };
inline constexpr Key<std::int64_t, event> fired{ "test.fired" };
} // namespace names

constexpr std::uint32_t cells = 300; // more than a ring holds: some are held back

// What the model and the test share. Atomics where the test reads while the
// model runs; the rest is read after the runner is gone (its join orders it).
struct Shared
{
        std::atomic<std::uint32_t> tick_ms{ 0 };
        std::atomic<std::uint32_t> ticks{ 0 };
        std::atomic<double> shortest_tick{ 1.0 };
        std::atomic<double> longest_tick{ 0.0 };
        std::atomic<std::uint32_t> low_memory{ 0 };
        std::atomic<std::uint32_t> saves{ 0 };
        std::atomic<std::uint32_t> flood_sent{ 0 };
        std::atomic<std::uint32_t> flood_full{ 0 };
        std::atomic<std::int64_t> flood_milliseconds{ -1 };
        std::atomic<bool> flood_done{ false };
        std::atomic<bool> flooding{ false };
        std::atomic<std::uint32_t> due_calls{ 0 };
        std::thread::id made_on;
        std::thread::id destroyed_on;
        std::string thread_name;
};

class Model final
{
    public:
        explicit Model( Shared &shared ) noexcept
            : shared_( shared )
        {
            shared_.made_on = std::this_thread::get_id();
            // prctl, not pthread_getname_np: Android's C library has that only
            // from API 26, and Termux's clang builds for API 24.
            std::array<char, 17> name{};
            if( ::prctl( PR_GET_NAME, name.data() ) == 0 ) {
                shared_.thread_name = name.data();
            }
        }
        ~Model() { shared_.destroyed_on = std::this_thread::get_id(); }
        Model( const Model & ) = delete;
        Model &operator=( const Model & ) = delete;
        Model( Model && ) = delete;
        Model &operator=( Model && ) = delete;

        void start( Telegraph &telegraph ) noexcept
        {
            telegraph.tick_every( shared_.tick_ms.load() );
        }

        void receive( Telegraph &telegraph, const Received &received ) noexcept
        {
            const Telegram &t = received.telegram;
            const Name name = t.address().name;
            if( name == names::echo.name() ) {
                static_cast<void>( telegraph.state( names::echo, t.as_integer() ) );
            } else if( name == names::flood.name() ) {
                flood( telegraph, static_cast<std::uint32_t>( t.as_integer() ) );
            } else if( name == names::round.name() ) {
                const std::int64_t round = t.as_integer();
                for( std::uint32_t uid = 1; uid <= cells; ++uid ) {
                    CHECK( telegraph.state( names::cell, uid, round ) != SendResult::full );
                }
                CHECK( telegraph.event( names::round_done, round ) == SendResult::sent );
            } else if( name == names::fill.name() ) {
                const std::int64_t value = t.as_integer();
                for( std::uint32_t uid = 1; uid <= cells; ++uid ) {
                    CHECK( telegraph.state( names::cell, uid, value ) != SendResult::full );
                }
            }
        }

        void tick( Telegraph &telegraph, double seconds ) noexcept
        {
            const std::uint32_t n = shared_.ticks.fetch_add( 1 ) + 1;
            if( seconds < shared_.shortest_tick.load() ) {
                shared_.shortest_tick.store( seconds );
            }
            if( seconds > shared_.longest_tick.load() ) {
                shared_.longest_tick.store( seconds );
            }
            static_cast<void>( telegraph.state( names::ticks, static_cast<std::int64_t>( n ) ) );
        }

        void low_memory( Telegraph &telegraph ) noexcept
        {
            shared_.low_memory.fetch_add( 1 );
            static_cast<void>( telegraph.event( names::pinged ) );
        }

        void background( Telegraph & /*telegraph*/ ) noexcept { shared_.saves.fetch_add( 1 ); }

    private:
        // `count` events, as fast as it can: the bounded wait, seen from the model.
        void flood( Telegraph &telegraph, std::uint32_t count ) noexcept
        {
            shared_.flooding.store( true );
            const Clock::time_point began = Clock::now();
            for( std::uint32_t i = 0; i < count; ++i ) {
                const SendResult result = telegraph.event( names::flooded, std::int64_t{ i } );
                if( result == SendResult::sent ) {
                    shared_.flood_sent.fetch_add( 1 );
                } else {
                    CHECK( result == SendResult::full );
                    shared_.flood_full.fetch_add( 1 );
                }
            }
            const auto took = std::chrono::duration_cast<Milliseconds>( Clock::now() - began );
            shared_.flood_milliseconds.store( took.count() );
            shared_.flood_done.store( true );
        }

        Shared &shared_;
};

// A model with timers: `do test.after <ms>` schedules one; when it is due,
// `event test.fired <ms>`. It sleeps until then: no tick, no polling.
class TimerModel final
{
    public:
        explicit TimerModel( Shared &shared ) noexcept
            : shared_( shared )
        {}

        void receive( Telegraph & /*telegraph*/, const Received &received ) noexcept
        {
            if( received.telegram.address().name == names::after.name() ) {
                const std::int64_t delay = received.telegram.as_integer();
                const auto wait = std::chrono::milliseconds( delay );
                const auto scheduled = timers_.schedule_after( sq::time::now(), wait, delay );
                CHECK( scheduled );
            }
        }

        [[nodiscard]] std::optional<TimePoint> wake_at() const noexcept
        {
            return timers_.next_due();
        }

        void due( Telegraph &telegraph, TimePoint now ) noexcept
        {
            shared_.due_calls.fetch_add( 1 );
            const auto fire = [&]( std::int64_t delay ) noexcept {
                CHECK( telegraph.event( names::fired, delay ) == SendResult::sent );
            };
            timers_.poll_due( now, timers_.capacity(), fire );
        }

    private:
        Shared &shared_;
        std::array<sq::time::DeadlineQueue<std::int64_t>::Entry, 8> storage_{};
        sq::time::DeadlineQueue<std::int64_t> timers_{ storage_ };
};

// The UI's side of a test: what the model sent, and a wait as a looper's.
struct Handler
{
        std::array<std::int64_t, 4> fired{};
        std::uint32_t fired_count = 0;
        Clock::time_point first_fired{};
        std::uint32_t flooded = 0;
        std::uint32_t pinged = 0;
        std::int64_t round_done = 0;
        bool stale = false;
        TelegraphUi *ui = nullptr;

        void event( const Received &received ) noexcept
        {
            const Name name = received.telegram.address().name;
            if( name == names::fired.name() ) {
                if( fired_count == 0 ) {
                    first_fired = Clock::now();
                }
                if( fired_count < fired.size() ) {
                    fired[fired_count] = received.telegram.as_integer();
                }
                ++fired_count;
            } else if( name == names::flooded.name() ) {
                ++flooded;
            } else if( name == names::pinged.name() ) {
                ++pinged;
            } else if( name == names::round_done.name() ) {
                round_done = received.telegram.as_integer();
                // Every state sent before the event is in the cache already.
                for( std::uint32_t uid = 1; uid <= cells; ++uid ) {
                    if( ui->value( names::cell, uid ) != round_done ) {
                        stale = true;
                    }
                }
            }
        }
};

// Drain until `done()`, sleeping on the wake descriptor between drains as
// the looper will. False if a sleep ran out: a wakeup was lost.
template<typename Done>
bool wait_for( TelegraphUi &ui, Handler &handler, Done done )
{
    for( ;; ) {
        ui.drain( handler );
        if( done() ) {
            return true;
        }
        if( ui.ready_to_sleep() ) {
            pollfd watched{ ui.wake_fd(), POLLIN, 0 };
            const int ready = ::poll( &watched, 1, lost_wakeup_ms );
            ui.woke();
            if( ready == 0 ) {
                return false;
            }
        }
    }
}

double cpu_seconds() noexcept
{
    timespec now{};
    ::clock_gettime( CLOCK_PROCESS_CPUTIME_ID, &now );
    return static_cast<double>( now.tv_sec ) + static_cast<double>( now.tv_nsec ) * 1e-9;
}

void sleep_ms( int milliseconds )
{
    std::this_thread::sleep_for( Milliseconds( milliseconds ) );
}

// Wait, with a bound, for something the model does without telling the UI.
template<typename Done>
bool spin_until( Done done )
{
    const Clock::time_point give_up = Clock::now() + Milliseconds( lost_wakeup_ms );
    while( !done() ) {
        if( Clock::now() > give_up ) {
            return false;
        }
        sleep_ms( 1 );
    }
    return true;
}

// ---- the tests -------------------------------------------------------------------------------

void tick_clock()
{
    // Times passed in: exact, and no sleeping.
    const TimePoint t0 = TimePoint( 100s );
    detail::TickClock clock;
    CHECK( clock.until_due( 0, t0 ) == -1 );
    CHECK( clock.until_due( 20, t0 ) == 20 );
    CHECK( clock.until_due( 20, t0 + 5ms ) == 15 );
    CHECK( clock.until_due( 20, t0 + 19ms + 1ns ) == 1 ); // rounded up, never early
    CHECK( clock.until_due( 20, t0 + 25ms ) == 0 );
    CHECK( clock.take( t0 + 25ms ) == 0.025 );       // measured from when the interval began
    CHECK( clock.until_due( 20, t0 + 25ms ) == 15 ); // the cadence kept: next at 40
    CHECK( clock.until_due( 20, t0 + 40ms ) == 0 );
    CHECK( clock.take( t0 + 40ms ) == 0.015 );
    CHECK( clock.until_due( 20, t0 + 400ms ) == 0 );  // a stall
    CHECK( clock.take( t0 + 400ms ) == 0.25 );        // clamped
    CHECK( clock.until_due( 20, t0 + 400ms ) == 20 ); // the cadence starts again: no burst
    CHECK( clock.until_due( 50, t0 + 410ms ) == 50 ); // a new interval starts now
    CHECK( clock.until_due( 0, t0 + 420ms ) == -1 );

    CHECK( detail::milliseconds_until( t0, t0 + 1s ) == 0 );
    CHECK( detail::milliseconds_until( t0 + 1ns, t0 ) == 1 );
    CHECK( detail::milliseconds_until( TimePoint::max(), t0 ) == std::numeric_limits<int>::max() );
    CHECK( detail::earliest( -1, 5 ) == 5 && detail::earliest( 7, -1 ) == 7 );
    CHECK( detail::earliest( 3, 9 ) == 3 && detail::earliest( -1, -1 ) == -1 );
}

// Two thousand round trips, each one a sleep and a wake on both sides.
void round_trips()
{
    const auto owned = std::make_unique<TelegraphLine>();
    TelegraphLine &line = *owned;
    Shared shared;
    TelegraphUi &ui = line.ui();
    Handler handler{ .ui = &ui };
    CHECK( ui.wake_fd() >= 0 );
    ModelRunner<Model> runner( line, std::ref( shared ) );
    CHECK( runner.threaded() );
    CHECK( ui.set( names::echo, std::int64_t{ 0 } ) == SendResult::sent );
    CHECK( wait_for( ui, handler, [&] { return ui.value( names::echo ) == 0; } ) );
    const std::size_t before = allocations.load();
    for( std::int64_t i = 1; i <= 2000; ++i ) {
        CHECK( ui.set( names::echo, i ) == SendResult::sent );
        CHECK( wait_for( ui, handler, [&] { return ui.value( names::echo ) == i; } ) );
    }
    CHECK( allocations.load() == before ); // no allocation once running, on either side

    // Idle: the model sleeps rather than spins.
    sleep_ms( 20 );
    const double cpu = cpu_seconds();
    sleep_ms( 200 );
    CHECK( cpu_seconds() - cpu < 0.05 );

    // Something waiting: the UI does not go to sleep.
    CHECK( ui.set( names::echo, std::int64_t{ -1 } ) == SendResult::sent );
    CHECK( spin_until( [&] { return !ui.ready_to_sleep(); } ) );
    ui.woke();
    CHECK( wait_for( ui, handler, [&] { return ui.value( names::echo ) == -1; } ) );
}

void ticks()
{
    const auto owned = std::make_unique<TelegraphLine>();
    TelegraphLine &line = *owned;
    Shared shared;
    TelegraphUi &ui = line.ui();
    Handler handler{ .ui = &ui };
    shared.tick_ms.store( 10 );
    {
        ModelRunner<Model> runner( line, std::ref( shared ) );
        CHECK( wait_for( ui, handler, [&] { return ui.value( names::ticks ) >= 1; } ) );
        sleep_ms( 150 );
        CHECK( wait_for( ui, handler, [&] { return ui.value( names::ticks ) >= 5; } ) );
    }
    const std::uint32_t n = shared.ticks.load();
    CHECK( n >= 5 && n <= 40 ); // about 15: wide, for a busy phone and the sanitizer
    CHECK( shared.shortest_tick.load() > 0.0 && shared.longest_tick.load() <= 0.25 );
}

// The UI does not drain: the model waits its 100 ms, then `full`, counted.
void wait_times_out()
{
    const auto owned = std::make_unique<TelegraphLine>();
    TelegraphLine &line = *owned;
    Shared shared;
    TelegraphUi &ui = line.ui();
    Handler handler{ .ui = &ui };
    const WaitStats before = ui.wait_stats();
    ModelRunner<Model> runner( line, std::ref( shared ) );
    const auto count = static_cast<std::int64_t>( telegraph_size::ring + 1 );
    CHECK( ui.do_( names::flood, count ).result == SendResult::sent );
    CHECK( spin_until( [&] { return shared.flood_done.load(); } ) );
    CHECK( shared.flood_sent.load() == telegraph_size::ring );
    CHECK( shared.flood_full.load() == 1 );
    CHECK( shared.flood_milliseconds.load() >= telegraph_size::wait_ms - 5 );
    const WaitStats after = ui.wait_stats();
    CHECK( after.waits == before.waits + 1 && after.full == before.full + 1 );
    CHECK( wait_for( ui, handler, [&] { return handler.flooded == telegraph_size::ring; } ) );
}

// The UI drains after a moment: the model's wait ends with room, nothing refused.
void wait_ended_by_drain()
{
    const auto owned = std::make_unique<TelegraphLine>();
    TelegraphLine &line = *owned;
    Shared shared;
    TelegraphUi &ui = line.ui();
    Handler handler{ .ui = &ui };
    const WaitStats before = ui.wait_stats();
    ModelRunner<Model> runner( line, std::ref( shared ) );
    const std::uint32_t count = telegraph_size::ring * 3;
    CHECK( ui.do_( names::flood, std::int64_t{ count } ).result == SendResult::sent );
    CHECK( spin_until( [&] { return shared.flooding.load(); } ) );
    sleep_ms( 20 );
    CHECK( wait_for( ui, handler, [&] { return handler.flooded == count; } ) );
    CHECK( spin_until( [&] { return shared.flood_done.load(); } ) );
    CHECK( shared.flood_full.load() == 0 && shared.flood_sent.load() == count );
    // Woken by each drain: well inside one wait's deadline, not at its end.
    CHECK( shared.flood_milliseconds.load() < telegraph_size::wait_ms );
    const WaitStats after = ui.wait_stats();
    CHECK( after.waits > before.waits && after.full == before.full );
}

// An event never overtakes the states held back before it, with the UI
// draining while the model sends.
void event_keeps_order()
{
    const auto owned = std::make_unique<TelegraphLine>();
    TelegraphLine &line = *owned;
    Shared shared;
    TelegraphUi &ui = line.ui();
    Handler handler{ .ui = &ui };
    for( std::uint32_t uid = 1; uid <= cells; ++uid ) {
        CHECK( ui.bind( { names::cell.name(), uid } ) );
    }
    {
        ModelRunner<Model> runner( line, std::ref( shared ) );
        for( std::int64_t round = 1; round <= 60; ++round ) {
            CHECK( ui.do_( names::round, round ).result == SendResult::sent );
            CHECK( wait_for( ui, handler, [&] { return handler.round_done == round; } ) );
            CHECK( !handler.stale );
        }
    }
    for( std::uint32_t uid = 1; uid <= cells; ++uid ) {
        ui.unbind( { names::cell.name(), uid } );
    }
}

// States alone, more than the ring holds, with nothing sent after them: the
// held-back ones still arrive, the model woken from its sleep by the drain.
void held_states_arrive()
{
    const auto owned = std::make_unique<TelegraphLine>();
    TelegraphLine &line = *owned;
    Shared shared;
    TelegraphUi &ui = line.ui();
    Handler handler{ .ui = &ui };
    ModelRunner<Model> runner( line, std::ref( shared ) );
    for( std::int64_t value = 1; value <= 20; ++value ) {
        CHECK( ui.do_( names::fill, value ).result == SendResult::sent );
        const auto all = [&] {
            for( std::uint32_t uid = 1; uid <= cells; ++uid ) {
                if( ui.value( names::cell, uid ) != value ) {
                    return false;
                }
            }
            return true;
        };
        sleep_ms( 2 ); // the model fills the ring and sleeps on what it holds
        CHECK( wait_for( ui, handler, all ) );
    }
}

// Android's lifecycle wakes a sleeping model; quit saves, and ends it on its thread.
void lifecycle_and_quit()
{
    const auto owned = std::make_unique<TelegraphLine>();
    TelegraphLine &line = *owned;
    Shared shared;
    TelegraphUi &ui = line.ui();
    Handler handler{ .ui = &ui };
    const std::uint32_t saves = shared.saves.load();
    {
        ModelRunner<Model> runner( line, std::ref( shared ) );
        sleep_ms( 20 ); // asleep by now, with nothing to tick
        ui.low_memory();
        CHECK( wait_for( ui, handler, [&] { return handler.pinged == 1; } ) );
        CHECK( shared.low_memory.load() == 1 );
    }
    CHECK( shared.saves.load() == saves + 1 );
    CHECK( shared.made_on == shared.destroyed_on );
    CHECK( shared.made_on != std::this_thread::get_id() );
    CHECK( shared.thread_name == "model" );
}

// Quit ends a wait for room at once: a model flooding a UI that is going.
void quit_ends_the_wait()
{
    const auto owned = std::make_unique<TelegraphLine>();
    TelegraphLine &line = *owned;
    Shared shared;
    TelegraphUi &ui = line.ui();
    Clock::time_point quit_at;
    {
        ModelRunner<Model> runner( line, std::ref( shared ) );
        const std::uint32_t before = ui.wait_stats().waits;
        CHECK( ui.do_( names::flood, std::int64_t{ 2000 } ).result == SendResult::sent );
        CHECK( spin_until( [&] { return ui.wait_stats().waits > before; } ) );
        quit_at = Clock::now();
    } // 1,700 sends of 100 ms each, but for quit
    const auto took = std::chrono::duration_cast<Milliseconds>( Clock::now() - quit_at );
    CHECK( took.count() < telegraph_size::wait_ms * 6 / 10 ); // woken, not timed out
    CHECK( shared.flood_done.load() );
}

// Timers on the model's thread: it sleeps until the first is due, wakes for
// it alone, and again for the next; `due` is called once each time.
void timers()
{
    const auto owned = std::make_unique<TelegraphLine>();
    TelegraphLine &line = *owned;
    Shared shared;
    TelegraphUi &ui = line.ui();
    Handler handler{ .ui = &ui };
    ModelRunner<TimerModel> runner( line, std::ref( shared ) );
    const Clock::time_point began = Clock::now();
    CHECK( ui.do_( names::after, std::int64_t{ 60 } ).result == SendResult::sent );
    CHECK( ui.do_( names::after, std::int64_t{ 30 } ).result == SendResult::sent );
    CHECK( wait_for( ui, handler, [&] { return handler.fired_count == 2; } ) );
    const auto first = std::chrono::duration_cast<Milliseconds>( handler.first_fired - began );
    CHECK( first.count() >= 30 && first.count() < 60 );
    CHECK( handler.fired[0] == 30 && handler.fired[1] == 60 );
    CHECK( shared.due_calls.load() == 2 ); // woken for each, not polling
}

// Timers inline: a turn runs what is due.
void inline_timers()
{
    const auto owned = std::make_unique<TelegraphLine>();
    TelegraphLine &line = *owned;
    Shared shared;
    TelegraphUi &ui = line.ui();
    Handler handler{ .ui = &ui };
    ModelRunner<TimerModel> runner( line, on_ui_thread, std::ref( shared ) );
    CHECK( ui.do_( names::after, std::int64_t{ 0 } ).result == SendResult::sent );
    CHECK( runner.turn() == 1 ); // scheduled, and due in the same turn
    ui.drain( handler );
    CHECK( handler.fired_count == 1 && handler.fired[0] == 0 );
    CHECK( ui.do_( names::after, std::int64_t{ 30 } ).result == SendResult::sent );
    CHECK( runner.turn() == 1 );
    CHECK( runner.turn() == 0 );
    ui.drain( handler );
    CHECK( handler.fired_count == 1 && shared.due_calls.load() == 1 ); // not yet
    sleep_ms( 35 );
    CHECK( runner.turn() == 0 );
    ui.drain( handler );
    CHECK( handler.fired_count == 2 && handler.fired[1] == 30 && shared.due_calls.load() == 2 );
}

// No thread: the UI runs the model a turn at a time; a full ring is `full` at once.
void inline_runner()
{
    const auto owned = std::make_unique<TelegraphLine>();
    TelegraphLine &line = *owned;
    Shared shared;
    TelegraphUi &ui = line.ui();
    Handler handler{ .ui = &ui };
    const WaitStats before = ui.wait_stats();
    const std::uint32_t saves = shared.saves.load();
    {
        ModelRunner<Model> runner( line, on_ui_thread, std::ref( shared ) );
        CHECK( !runner.threaded() );
        CHECK( shared.made_on == std::this_thread::get_id() );
        CHECK( ui.set( names::echo, std::int64_t{ 77 } ) == SendResult::sent );
        CHECK( runner.turn() == 1 );
        ui.drain( handler );
        CHECK( ui.value( names::echo ) == 77 );

        const std::uint32_t count = telegraph_size::ring + 10;
        CHECK( ui.do_( names::flood, std::int64_t{ count } ).result == SendResult::sent );
        const Clock::time_point began = Clock::now();
        CHECK( runner.turn() == 1 );
        CHECK( Clock::now() - began < Milliseconds( telegraph_size::wait_ms ) );
        CHECK( shared.flood_full.load() == 10 );
        const WaitStats after = ui.wait_stats();
        CHECK( after.waits == before.waits && after.full == before.full + 10 );
        ui.drain( handler );
    }
    CHECK( shared.saves.load() == saves + 1 );
    CHECK( shared.destroyed_on == std::this_thread::get_id() );
}

} // namespace

int main()
{
    tick_clock();
    // A line apiece, as an app has one: a model that quit stays quit.
    round_trips();
    ticks();
    wait_times_out();
    wait_ended_by_drain();
    event_keeps_order();
    held_states_arrive();
    lifecycle_and_quit();
    quit_ends_the_wait();
    inline_runner();
    timers();
    inline_timers();
    std::printf( "telegraph_thread_test: ok\n" );
    return 0;
}
