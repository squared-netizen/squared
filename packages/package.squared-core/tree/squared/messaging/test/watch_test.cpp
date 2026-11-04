// Watched descriptors (12.3d): a model on its own thread woken by a pipe
// with nothing else to do (no tick, no telegram), its `readable` called
// after `receive` in the turn, closing seen as readable, unwatching, the
// table full, a model with no `readable` refused, a busy UI not starving
// the descriptor, and the inline runner looking at it each turn. Run under
// ThreadSanitizer on desktop Linux before each package.
//
// Every wait here has a timeout, so a lost wakeup fails a CHECK instead of
// hanging the test.

#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

import sq.messaging;

import std;

using namespace sq::messaging;

namespace
{

#define CHECK( condition )                                                                         \
    do {                                                                                           \
        if( !( condition ) ) {                                                                     \
            std::fprintf( stderr, "watch_test.cpp:%d: %s\n", __LINE__, #condition );               \
            std::abort();                                                                          \
        }                                                                                          \
    } while( false )

using Clock = std::chrono::steady_clock;
using Milliseconds = std::chrono::milliseconds;

// Generous: ThreadSanitizer is slow, and the phone may be busy.
constexpr int lost_wakeup_ms = 3000;

namespace names
{
inline constexpr Key<std::int64_t, state> bytes{ "test.bytes" };
inline constexpr Key<std::int64_t, state> closed{ "test.closed" };
inline constexpr Key<std::int64_t, state | set> echo{ "test.echo" };
inline constexpr Key<std::int64_t, state> order{ "test.order" };
inline constexpr Key<void, do_> stop{ "test.stop" };
} // namespace names

// A pipe, both ends non-blocking and closed on exec.
struct Pipe
{
        int read = -1;
        int write = -1;

        Pipe() noexcept
        {
            std::array<int, 2> ends{ -1, -1 };
            CHECK( ::pipe( ends.data() ) == 0 );
            read = ends[0];
            write = ends[1];
            for( const int fd : ends ) {
                CHECK( ::fcntl( fd, F_SETFL, ::fcntl( fd, F_GETFL ) | O_NONBLOCK ) == 0 );
                CHECK( ::fcntl( fd, F_SETFD, FD_CLOEXEC ) == 0 );
            }
        }
        ~Pipe()
        {
            close_write();
            if( read >= 0 ) {
                ::close( read );
            }
        }
        Pipe( const Pipe & ) = delete;
        Pipe &operator=( const Pipe & ) = delete;
        Pipe( Pipe && ) = delete;
        Pipe &operator=( Pipe && ) = delete;

        void send( std::string_view text ) const noexcept
        {
            CHECK( ::write( write, text.data(), text.size() ) ==
                   static_cast<ssize_t>( text.size() ) );
        }
        void close_write() noexcept
        {
            if( write >= 0 ) {
                ::close( write );
                write = -1;
            }
        }
};

// Counts what the pipe brings; states the count, and closing as its own state.
// The order of hooks in a turn is recorded: `receive` first, then `readable`.
class Reader final
{
    public:
        Reader( int fd, std::atomic<std::uint32_t> *calls ) noexcept
            : fd_( fd )
            , calls_( calls )
        {}

        void start( Telegraph &telegraph ) noexcept { CHECK( telegraph.watch( fd_ ) ); }

        void receive( Telegraph &telegraph, const Received &received ) noexcept
        {
            const Name name = received.telegram.address().name;
            if( name == names::echo.name() ) {
                last_ = 'r';
                static_cast<void>( telegraph.state( names::echo, received.telegram.as_integer() ) );
            } else if( name == names::stop.name() ) {
                telegraph.unwatch( fd_ );
                CHECK( telegraph.watching() == 0 );
            }
        }

        void readable( Telegraph &telegraph, int fd ) noexcept
        {
            CHECK( fd == fd_ );
            if( calls_ != nullptr ) {
                calls_->fetch_add( 1 );
            }
            if( last_ == 'r' ) {
                static_cast<void>( telegraph.state( names::order, std::int64_t{ 1 } ) );
            }
            last_ = 'w';
            std::array<char, 64> buffer{};
            for( ;; ) {
                const ssize_t got = ::read( fd, buffer.data(), buffer.size() );
                if( got > 0 ) {
                    total_ += got;
                    continue;
                }
                if( got == 0 ) { // the writer closed: stop watching, or it stays ready
                    telegraph.unwatch( fd );
                    static_cast<void>( telegraph.state( names::closed, std::int64_t{ 1 } ) );
                }
                break;
            }
            static_cast<void>( telegraph.state( names::bytes, total_ ) );
        }

    private:
        int fd_;
        std::atomic<std::uint32_t> *calls_;
        std::int64_t total_ = 0;
        char last_ = ' ';
};

// No `readable`: watching is refused, so a ready descriptor can never spin it.
struct Deaf
{
        int fd = -1;
        bool watched = true;
        void start( Telegraph &telegraph ) noexcept { watched = telegraph.watch( fd ); }
        void receive( Telegraph &telegraph, const Received & /*received*/ ) noexcept
        {
            static_cast<void>( telegraph.state( names::echo, std::int64_t{ watched ? 1 : 0 } ) );
        }
};

// Drain until `done()`, sleeping on the wake descriptor between drains as the
// looper will. False if a sleep ran out: a wakeup was lost.
template<typename Done>
bool wait_for( TelegraphUi &ui, Done done )
{
    struct Nothing
    {
    } nothing;
    for( ;; ) {
        ui.drain( nothing );
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

template<typename K>
std::int64_t value_of( TelegraphUi &ui, const K &key )
{
    return ui.value( key ).value_or( -1 );
}

// The model sleeps with no timeout (no tick, no timer): only the pipe wakes it.
void wakes_on_the_pipe_alone()
{
    auto line = std::make_unique<TelegraphLine>();
    Pipe pipe;
    std::atomic<std::uint32_t> calls{ 0 };
    {
        ModelRunner<Reader> runner( *line, pipe.read, &calls );
        TelegraphUi &ui = line->ui();
        std::this_thread::sleep_for( Milliseconds( 50 ) ); // asleep by now
        pipe.send( "hello" );
        CHECK( wait_for( ui, [&] { return value_of( ui, names::bytes ) == 5; } ) );
        pipe.send( ", world" );
        CHECK( wait_for( ui, [&] { return value_of( ui, names::bytes ) == 12; } ) );
        // Asleep with the pipe empty: readable is not called again.
        const std::uint32_t settled = calls.load();
        std::this_thread::sleep_for( Milliseconds( 100 ) );
        CHECK( calls.load() == settled );
        // Closing is readable too: read gives 0, and the model unwatches.
        pipe.close_write();
        CHECK( wait_for( ui, [&] { return value_of( ui, names::closed ) == 1; } ) );
        const std::uint32_t after_close = calls.load();
        std::this_thread::sleep_for( Milliseconds( 100 ) );
        CHECK( calls.load() == after_close ); // unwatched: no spinning on POLLHUP
    }
    std::printf( "  the pipe alone wakes the model; closing is seen once\n" );
}

// In one turn, what the UI sent is received before the descriptor is read.
void receive_before_readable()
{
    auto line = std::make_unique<TelegraphLine>();
    Pipe pipe;
    {
        ModelRunner<Reader> runner( *line, pipe.read, nullptr );
        TelegraphUi &ui = line->ui();
        std::this_thread::sleep_for( Milliseconds( 50 ) );
        // Both before the model wakes: the set and the bytes land in one turn.
        CHECK( ui.set( names::echo, std::int64_t{ 7 } ) == SendResult::sent );
        pipe.send( "x" );
        CHECK( wait_for( ui, [&] { return value_of( ui, names::bytes ) == 1; } ) );
        CHECK( value_of( ui, names::echo ) == 7 );
    }
    std::printf( "  receive, then readable, in a turn\n" );
}

// A UI sending without pause keeps the model from sleeping; the pipe is still read.
void busy_ui_still_reads()
{
    auto line = std::make_unique<TelegraphLine>();
    Pipe pipe;
    {
        ModelRunner<Reader> runner( *line, pipe.read, nullptr );
        TelegraphUi &ui = line->ui();
        pipe.send( "abc" );
        const Clock::time_point give_up = Clock::now() + Milliseconds( lost_wakeup_ms );
        std::int64_t n = 0;
        struct Nothing
        {
        } nothing;
        while( value_of( ui, names::bytes ) != 3 && Clock::now() < give_up ) {
            static_cast<void>( ui.set( names::echo, ++n ) );
            ui.drain( nothing );
        }
        CHECK( value_of( ui, names::bytes ) == 3 );
    }
    std::printf( "  a busy UI does not starve a watched descriptor\n" );
}

// Unwatching from receive: bytes written after it are not read.
void unwatch_stops_it()
{
    auto line = std::make_unique<TelegraphLine>();
    Pipe pipe;
    std::atomic<std::uint32_t> calls{ 0 };
    {
        ModelRunner<Reader> runner( *line, pipe.read, &calls );
        TelegraphUi &ui = line->ui();
        pipe.send( "ab" );
        CHECK( wait_for( ui, [&] { return value_of( ui, names::bytes ) == 2; } ) );
        CHECK( ui.do_( names::stop ).result == SendResult::sent );
        CHECK( ui.set( names::echo, std::int64_t{ 9 } ) == SendResult::sent );
        CHECK( wait_for( ui, [&] { return value_of( ui, names::echo ) == 9; } ) );
        const std::uint32_t before = calls.load();
        pipe.send( "cd" );
        std::this_thread::sleep_for( Milliseconds( 100 ) );
        CHECK( calls.load() == before );
    }
    std::printf( "  unwatch stops it\n" );
}

// The table: TELEGRAPH_WATCHES at most; the same descriptor twice is one;
// negative refused; a model without readable refused.
struct Counter
{
        void receive( Telegraph & /*telegraph*/, const Received & /*received*/ ) noexcept {}
        void readable( Telegraph & /*telegraph*/, int /*fd*/ ) noexcept {}
};

void the_table()
{
    auto line = std::make_unique<TelegraphLine>();
    {
        ModelRunner<Counter> runner( *line, on_ui_thread );
        Telegraph &t = line->model();
        std::vector<std::unique_ptr<Pipe>> pipes;
        for( std::size_t i = 0; i <= telegraph_size::watches; ++i ) {
            pipes.push_back( std::make_unique<Pipe>() );
        }
        CHECK( !t.watch( -1 ) );
        for( std::size_t i = 0; i < telegraph_size::watches; ++i ) {
            CHECK( t.watch( pipes[i]->read ) );
        }
        CHECK( t.watch( pipes[0]->read ) ); // already: true, not a second entry
        CHECK( t.watching() == telegraph_size::watches );
        CHECK( !t.watch( pipes.back()->read ) );
        t.unwatch( pipes[1]->read );
        t.unwatch( 12345 ); // not watched: nothing
        CHECK( t.watching() == telegraph_size::watches - 1 );
        CHECK( t.watch( pipes.back()->read ) );
    }
    // The runner gone: the table is empty for the next.
    {
        Pipe pipe;
        ModelRunner<Deaf> runner( *line, on_ui_thread, -1 );
        Telegraph &t = line->model();
        CHECK( t.watching() == 0 );
        CHECK( !t.watch( pipe.read ) ); // no readable: refused
    }
    std::printf( "  the table: %zu at most, refused without readable\n", telegraph_size::watches );
}

// Inline: each turn looks at the descriptor (no sleep on this thread).
void inline_runner()
{
    auto line = std::make_unique<TelegraphLine>();
    Pipe pipe;
    std::atomic<std::uint32_t> calls{ 0 };
    {
        ModelRunner<Reader> runner( *line, on_ui_thread, pipe.read, &calls );
        TelegraphUi &ui = line->ui();
        struct Nothing
        {
        } nothing;
        static_cast<void>( runner.turn() );
        CHECK( calls.load() == 0 ); // nothing to read yet
        pipe.send( "four" );
        static_cast<void>( runner.turn() );
        CHECK( calls.load() == 1 );
        ui.drain( nothing );
        CHECK( value_of( ui, names::bytes ) == 4 );
        static_cast<void>( runner.turn() );
        CHECK( calls.load() == 1 );
    }
    std::printf( "  inline: a look each turn\n" );
}

// A deaf model on its own thread: its start's watch was refused, and says so.
void deaf_on_a_thread()
{
    auto line = std::make_unique<TelegraphLine>();
    Pipe pipe;
    {
        ModelRunner<Deaf> runner( *line, Deaf{ pipe.read } );
        TelegraphUi &ui = line->ui();
        CHECK( ui.set( names::echo, std::int64_t{ 5 } ) == SendResult::sent );
        CHECK( wait_for( ui, [&] { return ui.value( names::echo ).has_value(); } ) );
        CHECK( ui.value( names::echo ) == 0 );
    }
    std::printf( "  no readable, no watch\n" );
}

} // namespace

int main()
{
    std::printf( "watch_test\n" );
    wakes_on_the_pipe_alone();
    receive_before_readable();
    busy_ui_still_reads();
    unwatch_stops_it();
    the_table();
    inline_runner();
    deaf_on_a_thread();
    std::printf( "watch_test: ok\n" );
    return 0;
}
