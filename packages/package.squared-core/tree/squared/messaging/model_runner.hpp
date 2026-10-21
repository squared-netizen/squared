#pragma once

#include <squared/messaging/telegraph.hpp>

#include <cassert>
#include <chrono>
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
 * `low_memory(Telegraph&)`, `background(Telegraph&)` (save here) and
 * `foreground(Telegraph&, std::uint64_t elapsed_milliseconds)`.
 */
template<typename M>
concept TelegraphModel = requires( M &model, Telegraph &telegraph, const Received &received ) {
    model.receive( telegraph, received );
};

namespace detail
{

/// Names the calling thread, as `top -H` and a debugger show it (15 characters at most).
void name_this_thread( const char *name ) noexcept;

/// When a model's `tick` is due, and the seconds since the last one.
class TickClock final
{
    public:
        /** @brief Milliseconds until the next tick: 0 when due, -1 when there is none. */
        [[nodiscard]] int until_due( std::uint32_t interval_ms ) noexcept;

        /** @brief A tick happens now: the seconds since the last, at most a quarter second. */
        [[nodiscard]] double take() noexcept;

    private:
        using Clock = std::chrono::steady_clock;
        Clock::time_point last_{};
        Clock::time_point next_{};
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
 * between turns until the UI sends, Android's lifecycle reaches it, or a
 * `tick` is due. A turn is `deliver`, then `tick` if due. The runner's
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

        /** @brief The model on the UI thread, made now: call `turn()` once a frame. */
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
                tick_if_due( telegraph );
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
                const int until_tick = tick_if_due( telegraph );
                telegraph.idle( until_tick );
            }
        }

        // The milliseconds to the next tick, -1 for none.
        int tick_if_due( Telegraph &telegraph ) noexcept
        {
            if constexpr( requires { model_->tick( telegraph, 0.0 ); } ) {
                if( clock_.until_due( telegraph.tick_interval() ) == 0 ) {
                    model_->tick( telegraph, clock_.take() );
                }
                return clock_.until_due( telegraph.tick_interval() );
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
