export module sq.messaging:composite;

export import :model_runner;
import sq.time;
import std;

export namespace sq::messaging
{

/**
 * @brief Given for a part of a `Composite` that is made with no arguments,
 * where the others are given theirs: `Composite<Shell, Files>( places, no_args )`.
 */
struct NoArgs
{
        explicit NoArgs() = default;
};
inline constexpr NoArgs no_args{};

namespace detail
{

template<typename P>
concept PartTakes = requires {
    { P::takes.begin() };
    { P::takes.end() };
    requires std::same_as<std::remove_cvref_t<decltype( *P::takes.begin() )>, Name>;
};

template<typename P>
concept PartReads = requires( P &part, Telegraph &telegraph ) { part.readable( telegraph, 0 ); };

template<typename P>
concept PartWatches = requires( const P &part ) {
    { part.watches( 0 ) } -> std::convertible_to<bool>;
};

template<typename P>
concept PartStarts = requires( P &part, Telegraph &telegraph ) { part.start( telegraph ); };
template<typename P>
concept PartTicks = requires( P &part, Telegraph &telegraph ) { part.tick( telegraph, 0.0 ); };
template<typename P>
concept PartLowMemory = requires( P &part, Telegraph &telegraph ) { part.low_memory( telegraph ); };
template<typename P>
concept PartBackground =
    requires( P &part, Telegraph &telegraph ) { part.background( telegraph ); };
template<typename P>
concept PartForeground =
    requires( P &part, Telegraph &telegraph ) { part.foreground( telegraph, std::uint64_t{} ); };
template<typename P>
concept PartDue =
    requires( P &part, Telegraph &telegraph, time::TimePoint now ) { part.due( telegraph, now ); };

template<typename P, typename S>
concept PartSaves = requires( P &part, S &save ) { part.save( save ); };

template<typename P>
concept PartTimed = requires( P &part ) {
    { part.wake_at() } -> std::convertible_to<std::optional<time::TimePoint>>;
};

/// Every name the parts take, once each: two parts taking one name is a mistake.
template<typename... Parts>
consteval bool names_taken_once()
{
    std::vector<std::uint64_t> ids;
    (
        [&] {
            if constexpr( PartTakes<Parts> ) {
                for( const Name name : Parts::takes ) {
                    ids.push_back( name.id );
                }
            }
        }(),
        ... );
    std::ranges::sort( ids );
    return std::ranges::adjacent_find( ids ) == ids.end();
}

/// One part, made in place (a part need not move: a pty, a thread's handle).
template<typename P>
struct PartSlot
{
        P part;

        PartSlot()
            : part()
        {}
        explicit PartSlot( NoArgs /*no_args*/ )
            : part()
        {}
        template<typename A>
            requires( !std::same_as<std::remove_cvref_t<A>, NoArgs> )
        explicit PartSlot( A &&argument )
            : part( std::forward<A>( argument ) )
        {}
};

} // namespace detail

/**
 * @brief One model made of parts: an app runs one model (`App::run`), and a
 * model that does several things (a shell, the workspaces, an editor) is
 * several parts, each a model of its own, each with its own file.
 *
 * Each part is a `TelegraphModel` (it has `receive`), and says which names
 * it takes, as a `static constexpr` array of `Name`s:
 *
 *     struct Shell {
 *         static constexpr std::array takes{ term_input.name(), term_size.name() };
 *         void receive( sq::messaging::Telegraph &, const sq::messaging::Received & );
 *         ...
 *     };
 *     using Model = sq::messaging::Composite<Shell, Workspaces, Editor>;
 *     app.run<Model>( places, sq::messaging::no_args, editor_options );
 *
 * A telegram goes to the part that takes its name. One part may say nothing
 * (no `takes`): it has every name no other part takes (its saved values
 * too), as a single model does. Two parts taking one name does not compile.
 *
 * The other hooks go to each part that has them, in the order the parts are
 * named: `start`, `tick`, `low_memory`, `background`, `foreground`, and
 * `save` (a part that keeps values). Timers: the composite wakes at the
 * earliest of its parts' `wake_at()`, and calls `due` on each whose time has
 * come. Watched descriptors: a part that watches one and has `readable`
 * says which are its own with `bool watches( int fd ) const` when more than
 * one part reads; a ready descriptor no part says is its own is unwatched,
 * so it cannot keep waking the model.
 *
 * Made with no arguments, every part is made with none; or with one
 * argument a part, in order, `no_args` for a part made with none. Parts are
 * made in order on the model's thread, destroyed in reverse.
 */
template<typename... Parts>
class Composite final
{
        static_assert( sizeof...( Parts ) > 0, "a composite has parts" );
        static_assert( ( TelegraphModel<Parts> && ... ),
                       "each part has receive(Telegraph&, const Received&)" );
        static_assert( ( ( !detail::PartTakes<Parts> ? 1 : 0 ) + ... ) <= 1,
                       "at most one part takes what no other does (has no `takes`)" );
        static_assert( detail::names_taken_once<Parts...>(), "two parts take the same name" );
        static constexpr std::size_t readers = ( ( detail::PartReads<Parts> ? 1 : 0 ) + ... );
        static_assert(
            readers <= 1 || ( ( !detail::PartReads<Parts> || detail::PartWatches<Parts> ) && ... ),
            "with more than one part reading descriptors, each says which are its own (watches)" );

        struct Storage : detail::PartSlot<Parts>...
        {
                Storage() = default;
                template<typename... Args>
                explicit Storage( Args &&...args )
                    : detail::PartSlot<Parts>( std::forward<Args>( args ) )...
                {}
        };

    public:
        Composite() = default;

        /** @brief Each part from its argument, in order (`no_args`: made with none). */
        template<typename... Args>
            requires( sizeof...( Args ) == sizeof...( Parts ) && sizeof...( Args ) > 0 )
        explicit Composite( Args &&...args )
            : parts_( std::forward<Args>( args )... )
        {}

        /** @brief A part, by its type: what the app's own code reaches it by. */
        template<typename P>
        [[nodiscard]] P &part() noexcept
        {
            return static_cast<detail::PartSlot<P> &>( parts_ ).part;
        }
        template<typename P>
        [[nodiscard]] const P &part() const noexcept
        {
            return static_cast<const detail::PartSlot<P> &>( parts_ ).part;
        }

        void receive( Telegraph &telegraph, const Received &received ) noexcept
        {
            const Name name = received.telegram.address().name;
            bool taken = false;
            (
                [&] {
                    if constexpr( detail::PartTakes<Parts> ) {
                        if( !taken &&
                            std::ranges::find( Parts::takes, name ) != Parts::takes.end() ) {
                            part<Parts>().receive( telegraph, received );
                            taken = true;
                        }
                    }
                }(),
                ... );
            if( !taken ) {
                (
                    [&] {
                        if constexpr( !detail::PartTakes<Parts> ) {
                            part<Parts>().receive( telegraph, received );
                        }
                    }(),
                    ... );
            }
        }

        void start( Telegraph &telegraph ) noexcept
            requires( detail::PartStarts<Parts> || ... )
        {
            each( [&]( auto &p ) {
                if constexpr( requires { p.start( telegraph ); } ) {
                    p.start( telegraph );
                }
            } );
        }

        void tick( Telegraph &telegraph, double seconds ) noexcept
            requires( detail::PartTicks<Parts> || ... )
        {
            each( [&]( auto &p ) {
                if constexpr( requires { p.tick( telegraph, seconds ); } ) {
                    p.tick( telegraph, seconds );
                }
            } );
        }

        void low_memory( Telegraph &telegraph ) noexcept
            requires( detail::PartLowMemory<Parts> || ... )
        {
            each( [&]( auto &p ) {
                if constexpr( requires { p.low_memory( telegraph ); } ) {
                    p.low_memory( telegraph );
                }
            } );
        }

        void background( Telegraph &telegraph ) noexcept
            requires( detail::PartBackground<Parts> || ... )
        {
            each( [&]( auto &p ) {
                if constexpr( requires { p.background( telegraph ); } ) {
                    p.background( telegraph );
                }
            } );
        }

        void foreground( Telegraph &telegraph, std::uint64_t elapsed ) noexcept
            requires( detail::PartForeground<Parts> || ... )
        {
            each( [&]( auto &p ) {
                if constexpr( requires { p.foreground( telegraph, elapsed ); } ) {
                    p.foreground( telegraph, elapsed );
                }
            } );
        }

        void readable( Telegraph &telegraph, int fd ) noexcept
            requires( readers > 0 )
        {
            bool claimed = false;
            each( [&]( auto &p ) {
                using P = std::remove_cvref_t<decltype( p )>;
                if constexpr( detail::PartReads<P> ) {
                    bool mine = true;
                    if constexpr( detail::PartWatches<P> ) {
                        mine = p.watches( fd );
                    }
                    if( mine && !claimed ) {
                        claimed = true;
                        p.readable( telegraph, fd );
                    }
                }
            } );
            if( !claimed ) {
                telegraph.unwatch( fd ); // no one's: it would wake the model for ever
            }
        }

        [[nodiscard]] std::optional<time::TimePoint> wake_at() noexcept
            requires( detail::PartTimed<Parts> || ... )
        {
            std::optional<time::TimePoint> earliest;
            each( [&]( auto &p ) {
                using P = std::remove_cvref_t<decltype( p )>;
                if constexpr( detail::PartTimed<P> ) {
                    const std::optional<time::TimePoint> at = p.wake_at();
                    if( at && ( !earliest || *at < *earliest ) ) {
                        earliest = at;
                    }
                }
            } );
            return earliest;
        }

        void due( Telegraph &telegraph, time::TimePoint now ) noexcept
            requires( detail::PartDue<Parts> || ... )
        {
            each( [&]( auto &p ) {
                using P = std::remove_cvref_t<decltype( p )>;
                if constexpr( detail::PartTimed<P> && detail::PartDue<P> ) {
                    if( const std::optional<time::TimePoint> at = p.wake_at();
                        at && !( now < *at ) ) {
                        p.due( telegraph, now );
                    }
                }
            } );
        }

        /** @brief Each part's values (sq::Save), for a model that keeps them. */
        template<typename S>
            requires( detail::PartSaves<Parts, S> || ... )
        void save( S &save ) noexcept
        {
            each( [&]( auto &p ) {
                if constexpr( detail::PartSaves<std::remove_cvref_t<decltype( p )>, S> ) {
                    p.save( save );
                }
            } );
        }

    private:
        template<typename F>
        void each( const F &f ) noexcept
        {
            ( f( part<Parts>() ), ... );
        }

        Storage parts_;
};

} // namespace sq::messaging
