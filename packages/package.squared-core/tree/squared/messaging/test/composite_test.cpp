// A model made of parts (Composite): each telegram to the part that takes its
// name, the rest to the one part that takes no names; parts made from their
// own arguments (or none), in place, one that cannot move among them; start
// to each part that has it; the earliest of the parts' timers, and `due`
// to the parts whose time has come; a watched descriptor to the part that
// says it is its own, and one no part claims unwatched.

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

import sq.messaging;
import sq.time;
import std;

using namespace sq::messaging;

namespace
{

#define CHECK( condition )                                                                         \
    do {                                                                                           \
        if( !( condition ) ) {                                                                     \
            std::fprintf( stderr, "composite_test.cpp:%d: %s\n", __LINE__, #condition );           \
            std::abort();                                                                          \
        }                                                                                          \
    } while( false )

namespace names
{
inline constexpr Key<std::int64_t, state | do_> shell_in{ "shell.in" };
inline constexpr Key<std::int64_t, state> shell_seen{ "shell.seen" };
inline constexpr Key<std::int64_t, state | do_> files_in{ "files.in" };
inline constexpr Key<std::int64_t, state> files_seen{ "files.seen" };
inline constexpr Key<std::int64_t, state | do_> other{ "other.anything" };
inline constexpr Key<std::int64_t, state> rest_seen{ "rest.seen" };
inline constexpr Key<std::int64_t, state> started{ "parts.started" };
inline constexpr Key<std::int64_t, state> read{ "parts.read" };
inline constexpr Key<std::int64_t, do_> after{ "files.after" };
inline constexpr Key<std::int64_t, state> fired{ "files.fired" };
inline constexpr Key<void, do_> count{ "shell.count" };
inline constexpr Key<std::int64_t, state> watching{ "shell.watching" };
inline constexpr Key<std::int64_t, state> files_reads{ "files.reads" };
} // namespace names

// Moves nothing (a pty's part need not): made in place, from its argument.
struct Shell
{
        static constexpr std::array takes{ names::shell_in.name(), names::count.name() };

        struct Made
        {
                std::int64_t base = 0;
                int fd = -1;    // its own, watched at start
                int stray = -1; // watched at start, no part's
        };
        explicit Shell( Made made ) noexcept
            : base_( made.base )
            , fd_( made.fd )
            , stray_( made.stray )
        {}
        Shell( const Shell & ) = delete;
        Shell &operator=( const Shell & ) = delete;
        Shell( Shell && ) = delete;
        Shell &operator=( Shell && ) = delete;
        ~Shell() = default;

        void start( Telegraph &telegraph ) noexcept
        {
            for( const int fd : { fd_, stray_ } ) {
                if( fd >= 0 ) {
                    CHECK( telegraph.watch( fd ) );
                }
            }
            static_cast<void>( telegraph.state( names::started, std::int64_t{ 1 } ) );
        }
        void receive( Telegraph &telegraph, const Received &received ) noexcept
        {
            if( received.telegram.address().name == names::count.name() ) {
                static_cast<void>( telegraph.state(
                    names::watching, static_cast<std::int64_t>( telegraph.watching() ) ) );
                return;
            }
            static_cast<void>(
                telegraph.state( names::shell_seen, base_ + received.telegram.as_integer() ) );
        }
        void readable( Telegraph &telegraph, int fd ) noexcept
        {
            std::array<char, 16> buffer{};
            while( ::read( fd, buffer.data(), buffer.size() ) > 0 ) {
            }
            static_cast<void>( telegraph.state( names::read, std::int64_t{ 1 } ) );
        }
        [[nodiscard]] bool watches( int fd ) const noexcept { return fd == fd_; }

    private:
        std::int64_t base_;
        int fd_;
        int stray_;
};

// Made with no arguments (no_args); a timer.
struct Files
{
        static constexpr std::array takes{ names::files_in.name(), names::after.name() };

        void receive( Telegraph &telegraph, const Received &received ) noexcept
        {
            const Name name = received.telegram.address().name;
            if( name == names::after.name() ) {
                at_ = sq::time::now() + std::chrono::milliseconds( received.telegram.as_integer() );
                return;
            }
            static_cast<void>(
                telegraph.state( names::files_seen, received.telegram.as_integer() * 2 ) );
        }
        [[nodiscard]] std::optional<sq::time::TimePoint> wake_at() const noexcept { return at_; }
        void due( Telegraph &telegraph, sq::time::TimePoint /*now*/ ) noexcept
        {
            at_.reset();
            static_cast<void>( telegraph.state( names::fired, ++fired_ ) );
        }
        void readable( Telegraph &telegraph, int /*fd*/ ) noexcept
        {
            static_cast<void>( telegraph.state( names::files_reads, std::int64_t{ 1 } ) );
        }
        [[nodiscard]] bool watches( int /*fd*/ ) const noexcept { return false; } // watches none

    private:
        std::optional<sq::time::TimePoint> at_;
        std::int64_t fired_ = 0;
};

// Takes no names: everything no other part takes.
struct Rest
{
        explicit Rest( std::string label ) noexcept
            : label_( std::move( label ) )
        {}
        void receive( Telegraph &telegraph, const Received &received ) noexcept
        {
            static_cast<void>( telegraph.state( names::rest_seen,
                                                received.telegram.as_integer() +
                                                    static_cast<std::int64_t>( label_.size() ) ) );
        }

    private:
        std::string label_;
};

using Model = Composite<Shell, Files, Rest>;

static_assert( TelegraphModel<Model> );

struct NoHandler
{};

template<typename K>
std::int64_t value_of( TelegraphUi &ui, const K &key )
{
    return ui.value( key ).value_or( -1 );
}

void routes_by_name()
{
    auto line = std::make_unique<TelegraphLine>();
    TelegraphUi &ui = line->ui();
    NoHandler handler;
    ModelRunner<Model> runner( *line, on_ui_thread, Shell::Made{ .base = 100 }, no_args,
                               std::string( "rest" ) );
    CHECK( runner.turn() == 0 );
    ui.drain( handler );
    CHECK( value_of( ui, names::started ) == 1 ); // the parts' start

    CHECK( ui.do_( names::shell_in, std::int64_t{ 5 } ).result == SendResult::sent );
    CHECK( ui.do_( names::files_in, std::int64_t{ 7 } ).result == SendResult::sent );
    CHECK( ui.do_( names::other, std::int64_t{ 1 } ).result == SendResult::sent );
    CHECK( runner.turn() == 3 );
    ui.drain( handler );
    CHECK( value_of( ui, names::shell_seen ) == 105 ); // Shell, made from 100
    CHECK( value_of( ui, names::files_seen ) == 14 );  // Files
    CHECK( value_of( ui, names::rest_seen ) == 5 );    // the rest: 1 + "rest"
    std::puts( "  ok   each telegram to the part that takes its name; the rest to the one that "
               "takes none" );
}

void timers()
{
    auto line = std::make_unique<TelegraphLine>();
    TelegraphUi &ui = line->ui();
    NoHandler handler;
    ModelRunner<Model> runner( *line, on_ui_thread, Shell::Made{}, no_args, std::string() );
    CHECK( ui.do_( names::after, std::int64_t{ 0 } ).result == SendResult::sent );
    CHECK( runner.turn() == 1 ); // set, and due in the same turn
    ui.drain( handler );
    CHECK( value_of( ui, names::fired ) == 1 );
    CHECK( ui.do_( names::after, std::int64_t{ 30 } ).result == SendResult::sent );
    CHECK( runner.turn() == 1 );
    ui.drain( handler );
    CHECK( value_of( ui, names::fired ) == 1 ); // not yet
    std::this_thread::sleep_for( std::chrono::milliseconds( 35 ) );
    CHECK( runner.turn() == 0 );
    ui.drain( handler );
    CHECK( value_of( ui, names::fired ) == 2 );
    std::puts(
        "  ok   the parts' timers: the earliest wakes it; due to the part whose time has come" );
}

void descriptors()
{
    auto line = std::make_unique<TelegraphLine>();
    TelegraphUi &ui = line->ui();
    NoHandler handler;
    std::array<int, 2> mine{ -1, -1 };
    std::array<int, 2> stray{ -1, -1 };
    CHECK( ::pipe( mine.data() ) == 0 && ::pipe( stray.data() ) == 0 );
    for( const int fd : { mine[0], stray[0] } ) {
        CHECK( ::fcntl( fd, F_SETFL, ::fcntl( fd, F_GETFL ) | O_NONBLOCK ) == 0 );
    }
    {
        ModelRunner<Model> runner( *line, on_ui_thread,
                                   Shell::Made{ .fd = mine[0], .stray = stray[0] }, no_args,
                                   std::string() );
        CHECK( ::write( mine[1], "x", 1 ) == 1 );
        CHECK( ::write( stray[1], "y", 1 ) == 1 );
        static_cast<void>( runner.turn() );
        CHECK( ui.do_( names::count ).result == SendResult::sent );
        static_cast<void>( runner.turn() );
        ui.drain( handler );
        CHECK( value_of( ui, names::read ) == 1 );         // Shell's, to Shell
        CHECK( value_of( ui, names::files_reads ) == -1 ); // not to Files
        CHECK( value_of( ui, names::watching ) == 1 );     // the stray one unwatched
    }
    for( const int fd : { mine[0], mine[1], stray[0], stray[1] } ) {
        ::close( fd );
    }
    std::puts( "  ok   a descriptor to the part that watches it; one no part claims, unwatched" );
}

} // namespace

int main()
{
    routes_by_name();
    timers();
    descriptors();
    std::puts( "composite test passed" );
    return 0;
}
