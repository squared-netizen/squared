#pragma once

#include <squared/messaging/telegraph.hpp>
#include <squared/time/now.hpp>
#include <squared/time/time_point.hpp>
#include <squared/time/timepiece.hpp>

#include <cassert>
#include <cstdint>
#include <optional>
#include <thread>
#include <type_traits>
#include <utility>

namespace sq::messaging
{

/** @brief Asks a `ModelRunner` for no thread: the UI calls `turn()` once a frame. */
struct OnUiThread
{
        explicit OnUiThread() = default;
};
inline constexpr OnUiThread on_ui_thread{};

/**
 * @brief A model: it takes what the UI sends. Its other hooks are optional:
 * `start(Telegraph&)`, `tick(Telegraph&, double seconds)`,
 * `low_memory(Telegraph&)`, `background(Telegraph&)` (save here),
 * `foreground(Telegraph&, std::uint64_t elapsed_milliseconds)`, and for
 * timers the pair `std::optional<sq::time::TimePoint> wake_at()` (when it
 * next needs a turn; a `DeadlineQueue`'s `next_due()`) and
 * `due(Telegraph&, sq::time::TimePoint now)` (that time has come).
 */
template<typename M>
concept TelegraphModel = requires( M &model, Telegraph &telegraph, const Received &received ) {
    model.receive( telegraph, received );
};

namespace detail
{

/// Names the calling thread, as `top -H` and a debugger show it (15 characters at most).
void name_this_thread( const char *name ) noexcept;

/// Whole milliseconds from `now` until `at`, rounded up: 0 when it has come.
[[nodiscard]] int milliseconds_until( time::TimePoint at, time::TimePoint now ) noexcept;

/// The earlier of two timeouts in milliseconds, -1 meaning none.
[[nodiscard]] constexpr int earliest( int first, int second ) noexcept
{
    if( first < 0 ) {
        return second;
    }
    if( second < 0 ) {
        return first;
    }
    return first < second ? first : second;
}

/// When a model's `tick` is due; its steps measured by a `time::Timepiece`.
class TickClock final
{
    public:
        /** @brief Milliseconds from `now` until the next tick: 0 when due, -1 when there is none. */
        [[nodiscard]] int until_due( std::uint32_t interval_ms, time::TimePoint now ) noexcept;

        /** @brief A tick happens at `now`: the seconds since the last, at most a quarter second. */
        [[nodiscard]] double take( time::TimePoint now ) noexcept;

    private:
        time::Timepiece time_;
        time::TimePoint next_{};
        std::uint32_t interval_ms_ = 0;
};

template<typename... Args>
inline constexpr bool starts_on_ui_thread = false;
template<typename First, typename... Rest>
inline constexpr bool starts_on_ui_thread<First, Rest...> =
    std::is_same_v<std::remove_cvref_t<First>, OnUiThread>;

} // namespace detail

/**
 * @brief Runs a model at the far end of a `TelegraphLine`: on its own
 * thread (named "model"), or on the UI thread a turn at a time.
 *
 * On its own thread the model is made there, from the arguments (copied,
 * as `std::thread` copies them: `std::ref` for a reference), and it sleeps
 * between turns until the UI sends, Android's lifecycle reaches it, a
 * `tick` is due, or its `wake_at()` time comes. A turn is `deliver`, then
 * `tick` if due, then `due` if its time has come; the clock is read once a
 * turn. The runner's
 * destructor asks the model to quit (it saves, in `background`), wakes it,
 * and waits for its thread: the model is destroyed on the thread it lived on.
 *
 * One runner to a line. The model lives inside the runner, not on a stack.
 */
template<typename M>
class ModelRunner final
{
        static_assert( TelegraphModel<M>, "a model has receive(Telegraph&, const Received&)" );

    public:
        /** @brief The model on its own thread, made there from `args`. */
        template<typename... Args>
            requires( !detail::starts_on_ui_thread<Args...> )
        explicit ModelRunner( TelegraphLine &line, Args &&...args ) noexcept
            : line_( line )
        {
            claim();
            line_.threaded_ = true; // before the thread starts, so it sees it
            // A thread that cannot be made is unrecoverable at start: std::thread
            // throws, and noexcept makes that a terminate.
            auto body = [this, ... copied = std::forward<Args>( args )]() mutable noexcept {
                detail::name_this_thread( "model" );
                model_.emplace( std::move( copied )... );
                run();
                model_.reset();
            };
            thread_ = std::thread( std::move( body ) );
        }

        /** @brief The model on the UI thread, made now: call `turn()` once a frame (ticks and timers run there). */
        template<typename... Args>
        ModelRunner( TelegraphLine &line, OnUiThread /*on_ui_thread*/, Args &&...args ) noexcept
            : line_( line )
        {
            claim();
            model_.emplace( std::forward<Args>( args )... );
            line_.model().start( *model_ );
        }

        ModelRunner( const ModelRunner & ) = delete;
        ModelRunner &operator=( const ModelRunner & ) = delete;
        ModelRunner( ModelRunner && ) = delete;
        ModelRunner &operator=( ModelRunner && ) = delete;

        ~ModelRunner()
        {
            line_.ui().quit();
            if( thread_.joinable() ) {
                thread_.join();
            } else {
                turn(); // the model saves, inline
                model_.reset();
            }
            line_.threaded_ = false;
            line_.has_runner_ = false;
        }

        /**
         * @brief Inline only: one turn of the model, on the UI thread. The
         * telegrams it handled; 0 once it has quit.
         */
        std::uint32_t turn() noexcept
        {
            assert( !thread_.joinable() && "the model has its own thread" );
            Telegraph &telegraph = line_.model();
            if( telegraph.quitting() ) {
                return 0;
            }
            const std::uint32_t handled = telegraph.deliver( *model_ );
            if( !telegraph.quitting() ) {
                static_cast<void>( after_deliver( telegraph ) );
            }
            return handled;
        }

        /** @brief Whether the model runs on its own thread. */
        [[nodiscard]] bool threaded() const noexcept { return thread_.joinable(); }

    private:
        void claim() noexcept
        {
            assert( !line_.has_runner_ && "one ModelRunner to a TelegraphLine" );
            line_.has_runner_ = true;
        }

        // The model's thread: turns, and sleep between them.
        void run() noexcept
        {
            Telegraph &telegraph = line_.model();
            telegraph.start( *model_ );
            for( ;; ) {
                telegraph.deliver( *model_ );
                if( telegraph.quitting() ) {
                    break;
                }
                telegraph.idle( after_deliver( telegraph ) );
            }
        }

        // The rest of a turn: the tick if due, then the timers if due. The
        // milliseconds until the next of either, -1 for neither.
        int after_deliver( Telegraph &telegraph ) noexcept
        {
            const time::TimePoint now = time::now(); // one clock read a turn
            const int until_tick = tick_if_due( telegraph, now );
            return detail::earliest( until_tick, timers_if_due( telegraph, now ) );
        }

        int tick_if_due( Telegraph &telegraph, time::TimePoint now ) noexcept
        {
            if constexpr( requires { model_->tick( telegraph, 0.0 ); } ) {
                const std::uint32_t interval = telegraph.tick_interval();
                if( clock_.until_due( interval, now ) == 0 ) {
                    model_->tick( telegraph, clock_.take( now ) );
                }
                return clock_.until_due( interval, now );
            } else {
                return -1;
            }
        }

        int timers_if_due( Telegraph &telegraph, time::TimePoint now ) noexcept
        {
            if constexpr( requires { model_->wake_at(); } ) {
                constexpr bool has_due = requires { model_->due( telegraph, now ); };
                static_assert( has_due, "a model with wake_at() has due(Telegraph&, TimePoint)" );
                std::optional<time::TimePoint> at = model_->wake_at();
                if( at && *at <= now ) {
                    model_->due( telegraph, now );
                    at = model_->wake_at();
                }
                return at ? detail::milliseconds_until( *at, now ) : -1;
            } else {
                return -1;
            }
        }

        TelegraphLine &line_;
        std::optional<M> model_;
        detail::TickClock clock_;
        std::thread thread_;
};

} // namespace sq::messaging
