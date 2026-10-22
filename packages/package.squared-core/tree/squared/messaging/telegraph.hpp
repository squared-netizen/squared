#pragma once

#include <squared/messaging/address.hpp>
#include <squared/messaging/key.hpp>
#include <squared/messaging/list_window.hpp>
#include <squared/messaging/model_list.hpp>
#include <squared/messaging/src/telegraph_config.hpp>
#include <squared/messaging/src/wake.hpp>
#include <squared/messaging/telegram.hpp>
#include <squared/messaging/telegram_ring.hpp>
#include <squared/messaging/value_cache.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

namespace sq::messaging
{

class TelegraphLine;
template<typename M>
class ModelRunner;

/** @brief What a send did. */
enum class SendResult : std::uint8_t {
    /** In the ring. */
    sent,
    /** A `state` held back, the ring being full: it goes when room frees, the latest value of its address. */
    pending,
    /** No room now; nothing was sent. */
    full,
    /** The text or uid list is over a quarter of the data ring: it can never be sent. */
    too_large,
};

/**
 * @brief How often the model found no room toward the UI: the `telegraph`
 * command shows these, so a model that outpaces its UI is seen.
 */
struct WaitStats
{
        /** Times the model waited for the UI to drain (on its own thread only). */
        std::uint32_t waits = 0;
        /** Sends refused `full`: after the wait on its own thread, at once inline. */
        std::uint32_t full = 0;
};

/**
 * @brief The model's side, as the UI may read it (the `telegraph` command):
 * states held back now and at most, and lists declared.
 */
struct ModelStats
{
        std::uint32_t held = 0;
        std::uint32_t highest_held = 0;
        std::uint32_t lists = 0;
};

/** @brief What `TelegraphUi::do_` did, and the correlation the answering `event` will carry. */
struct DoResult
{
        SendResult result = SendResult::full;
        std::uint16_t correlation = 0;
};

namespace detail
{

/// A telegram and its text or uid list, from a key's value.
struct Packed
{
        Telegram telegram;
        std::span<const std::byte> data;
};

template<typename T, typename V>
[[nodiscard]] Packed pack( Verb verb, Address to, const V &value ) noexcept
{
    if constexpr( std::is_same_v<T, bool> ) {
        return { Telegram::boolean( verb, to, value ), {} };
    } else if constexpr( std::is_same_v<T, std::int64_t> ) {
        return { Telegram::integer( verb, to, static_cast<std::int64_t>( value ) ), {} };
    } else if constexpr( std::is_same_v<T, double> ) {
        return { Telegram::number( verb, to, static_cast<double>( value ) ), {} };
    } else if constexpr( std::is_same_v<T, std::string_view> ) {
        const std::string_view text( value );
        const auto bytes = std::as_bytes( std::span<const char>( text.data(), text.size() ) );
        return { Telegram::data( verb, to, ValueType::text, {} ), bytes };
    } else if constexpr( std::is_same_v<T, std::span<const std::uint32_t>> ) {
        const std::span<const std::uint32_t> uids( value );
        return { Telegram::data( verb, to, ValueType::uids, {} ), std::as_bytes( uids ) };
    } else {
        const T floats( value ); // std::array<float, 2..4>
        return { Telegram::floats( verb, to, std::span<const float>( floats ) ), {} };
    }
}

template<typename K>
using key_type = std::remove_cvref_t<K>;

/// What reading a key gives: its value type, or nothing.
template<typename K>
using value_of = std::optional<typename key_type<K>::value_type>;

} // namespace detail

/**
 * @brief The model's end of the Telegraph: what a model sees, on its thread.
 *
 * It sends `state` and `event` by key, declares virtualized lists, and runs
 * the model's side of a turn (`deliver`): lifecycle signals, held-back
 * states, then every telegram from the UI. A sending mistake (a wrong type,
 * a verb the key does not carry, a missing or extra uid) does not compile.
 */
class Telegraph final
{
    public:
        Telegraph( const Telegraph & ) = delete;
        Telegraph &operator=( const Telegraph & ) = delete;
        Telegraph( Telegraph && ) = delete;
        Telegraph &operator=( Telegraph && ) = delete;
        ~Telegraph() = default;

        /**
         * @brief `state <key> <value>`. When the ring is full it is held
         * back, coalesced: `pending`. When the held-back table is full too,
         * the model's thread waits up to `TELEGRAPH_WAIT_MS` for the UI to
         * drain, then `full`.
         */
        template<typename K, typename V>
            requires sendable<K, Verb::state, V> && ( !detail::key_type<K>::takes_uid )
        SendResult state( const K &key, const V &value ) noexcept
        {
            using T = typename detail::key_type<K>::value_type;
            const auto packed = detail::pack<T>( Verb::state, { key.name(), 0 }, value );
            return send_state( packed.telegram, packed.data );
        }

        /** @brief `state <key>[uid] <value>`, for a key that takes a uid. */
        template<typename K, typename V>
            requires sendable<K, Verb::state, V> && detail::key_type<K>::takes_uid
        SendResult state( const K &key, std::uint32_t uid, const V &value ) noexcept
        {
            using T = typename detail::key_type<K>::value_type;
            const auto packed = detail::pack<T>( Verb::state, { key.name(), uid }, value );
            return send_state( packed.telegram, packed.data );
        }

        /**
         * @brief `event <key>`, for a key with no value. Never held back:
         * with no room, the model's thread waits up to `TELEGRAPH_WAIT_MS`,
         * then `full`.
         */
        template<typename K>
            requires sendable<K, Verb::event, void> && ( !detail::key_type<K>::takes_uid )
        SendResult event( const K &key ) noexcept
        {
            return send_event( Telegram::empty( Verb::event, { key.name(), 0 } ), {} );
        }

        /** @brief `event <key> <value>`. */
        template<typename K, typename V>
            requires sendable<K, Verb::event, V> && ( !detail::key_type<K>::takes_uid )
        SendResult event( const K &key, const V &value ) noexcept
        {
            using T = typename detail::key_type<K>::value_type;
            const auto packed = detail::pack<T>( Verb::event, { key.name(), 0 }, value );
            return send_event( packed.telegram, packed.data );
        }

        /** @brief `event <key>[uid] <value>`, for a key that takes a uid. */
        template<typename K, typename V>
            requires sendable<K, Verb::event, V> && detail::key_type<K>::takes_uid
        SendResult event( const K &key, std::uint32_t uid, const V &value ) noexcept
        {
            using T = typename detail::key_type<K>::value_type;
            const auto packed = detail::pack<T>( Verb::event, { key.name(), uid }, value );
            return send_event( packed.telegram, packed.data );
        }

        /** @brief `event <key>` answering `request` (a `do`): it carries the request's correlation. */
        template<typename K>
            requires sendable<K, Verb::event, void> && ( !detail::key_type<K>::takes_uid )
        SendResult answer( const Received &request, const K &key ) noexcept
        {
            const Telegram t = Telegram::empty( Verb::event, { key.name(), 0 } );
            return send_event( t.with_correlation( request.telegram.correlation() ), {} );
        }

        /** @brief `event <key> <value>` answering `request`. */
        template<typename K, typename V>
            requires sendable<K, Verb::event, V> && ( !detail::key_type<K>::takes_uid )
        SendResult answer( const Received &request, const K &key, const V &value ) noexcept
        {
            using T = typename detail::key_type<K>::value_type;
            const auto packed = detail::pack<T>( Verb::event, { key.name(), 0 }, value );
            const std::uint16_t correlation = request.telegram.correlation();
            return send_event( packed.telegram.with_correlation( correlation ), packed.data );
        }

        /**
         * @brief A virtualized list: `rows` (a key taking a uid: the row) and
         * where its rows come from. Its count is sent at once. False when
         * `TELEGRAPH_LISTS` lists exist already, or this one does.
         */
        template<typename K, typename S>
            requires is_key<detail::key_type<K>> && detail::key_type<K>::takes_uid &&
                     ListRows<S, Telegraph>
        bool list( const K &rows, S &source ) noexcept
        {
            return add_list( rows.name(), list_source<Telegraph>( source ) );
        }

        /** @brief The list's rows changed (a sort, an insert, a removal): what changed on screen is sent. */
        template<typename K>
            requires is_key<detail::key_type<K>>
        void changed( const K &rows ) noexcept
        {
            changed_list( rows.name() );
        }

        /** @brief One item's fields changed: published again if it is on screen. */
        template<typename K>
            requires is_key<detail::key_type<K>>
        void changed( const K &rows, std::uint32_t uid ) noexcept
        {
            changed_item( rows.name(), uid );
        }

        /** @brief Ask for `tick` every `milliseconds` (0: no tick, the default). `ModelRunner` calls it. */
        void tick_every( std::uint32_t milliseconds ) noexcept;
        [[nodiscard]] std::uint32_t tick_interval() const noexcept;

        /** @brief Calls `model.start(*this)`, if it has one. Once, before `deliver`. */
        template<typename M>
        void start( M &model ) noexcept;

        /**
         * @brief One turn of the model's side: the lifecycle signals (low
         * memory first; background and foreground in the order that ends
         * where the app is now; quit last, after a save), then the held-back
         * states, then each telegram from the UI: a list's `view` to its
         * list, everything else to `model.receive(*this, telegram)`. The
         * telegrams handled.
         */
        template<typename M>
        std::uint32_t deliver( M &model ) noexcept;

        /** @brief States held back, waiting for room. */
        [[nodiscard]] std::uint32_t pending() const noexcept;

        /** @brief Whether quit has been asked for: the model saved, and is to stop. */
        [[nodiscard]] bool quitting() const noexcept;

    private:
        friend class TelegraphLine;
        template<typename M>
        friend class ModelRunner;
        explicit Telegraph( TelegraphLine &line ) noexcept
            : line_( line )
        {}

        struct Signals
        {
                bool low_memory = false;
                bool background = false;
                bool foreground = false;
                bool quit = false;
                bool visible = true; // where the app is now
                std::uint64_t elapsed = 0;
        };

        SendResult send_state( const Telegram &telegram, std::span<const std::byte> data ) noexcept;
        SendResult try_state( const Telegram &telegram, std::span<const std::byte> data ) noexcept;
        SendResult send_event( const Telegram &telegram, std::span<const std::byte> data ) noexcept;
        SendResult try_event( const Telegram &telegram, std::span<const std::byte> data ) noexcept;
        SendResult hold( const Telegram &telegram, std::span<const std::byte> data ) noexcept;
        RingResult push_to_ui( const Telegram &telegram, std::span<const std::byte> data ) noexcept;
        bool flush() noexcept;
        // On the model's thread: wait for the UI to drain, `attempt` again
        // each time it does, until it succeeds (true) or the wait ends.
        template<typename F>
        bool wait_for_room( F attempt ) noexcept;
        // On the model's thread, between turns: send what is held back, wake
        // the UI for what this turn sent, and sleep until the UI sends, a
        // lifecycle signal, room for what is held, or `timeout_ms` (-1: none).
        void idle( int timeout_ms ) noexcept;
        void wake_ui() noexcept;
        void count_full() noexcept;
        void publish_held() noexcept;
        bool add_list( Name rows, ListSource source ) noexcept;
        void changed_list( Name rows ) noexcept;
        void changed_item( Name rows, std::uint32_t uid ) noexcept;
        [[nodiscard]] ModelList *list_named( Name rows ) noexcept;
        // A `do <list>.view (first last)`: answered, and true; false if not one.
        bool answer_view( const Received &received ) noexcept;
        [[nodiscard]] ListOutput output() noexcept;
        [[nodiscard]] Signals take_signals() noexcept;
        [[nodiscard]] std::optional<Received> next_from_ui() const noexcept;
        void pop_from_ui() noexcept;
        void mark_quitting() noexcept;

        TelegraphLine &line_;
};

/**
 * @brief The UI's end of the Telegraph: the Stage's, on the UI thread. An
 * app reaches it through its screens, not directly.
 *
 * It sends `set` and `do`, asks lists for rows, takes what the model sent
 * into the latest-value cache (`drain`), reads values back, binds the ones
 * on screen, and passes Android's lifecycle to the model. It never waits: a
 * full ring is `full`. Each send wakes the model's thread if it sleeps; when
 * the UI sleeps itself, `wake_fd` wakes it for what the model sends.
 */
class TelegraphUi final
{
    public:
        TelegraphUi( const TelegraphUi & ) = delete;
        TelegraphUi &operator=( const TelegraphUi & ) = delete;
        TelegraphUi( TelegraphUi && ) = delete;
        TelegraphUi &operator=( TelegraphUi && ) = delete;
        ~TelegraphUi() = default;

        /** @brief `set <key> <value>`: the user changed a value. */
        template<typename K, typename V>
            requires sendable<K, Verb::set, V> && ( !detail::key_type<K>::takes_uid )
        SendResult set( const K &key, const V &value ) noexcept
        {
            using T = typename detail::key_type<K>::value_type;
            const auto packed = detail::pack<T>( Verb::set, { key.name(), 0 }, value );
            return push( packed.telegram, packed.data );
        }

        /** @brief `set <key>[uid] <value>`. */
        template<typename K, typename V>
            requires sendable<K, Verb::set, V> && detail::key_type<K>::takes_uid
        SendResult set( const K &key, std::uint32_t uid, const V &value ) noexcept
        {
            using T = typename detail::key_type<K>::value_type;
            const auto packed = detail::pack<T>( Verb::set, { key.name(), uid }, value );
            return push( packed.telegram, packed.data );
        }

        /** @brief `do <key>`: an intent, with no value. */
        template<typename K>
            requires sendable<K, Verb::do_, void> && ( !detail::key_type<K>::takes_uid )
        DoResult do_( const K &key ) noexcept
        {
            return request( Telegram::empty( Verb::do_, { key.name(), 0 } ), {} );
        }

        /** @brief `do <key> <value>`. */
        template<typename K, typename V>
            requires sendable<K, Verb::do_, V> && ( !detail::key_type<K>::takes_uid )
        DoResult do_( const K &key, const V &value ) noexcept
        {
            using T = typename detail::key_type<K>::value_type;
            return request_packed( detail::pack<T>( Verb::do_, { key.name(), 0 }, value ) );
        }

        /** @brief `do <key>[uid]`, for a key with no value that takes a uid (a row's button). */
        template<typename K>
            requires sendable<K, Verb::do_, void> && detail::key_type<K>::takes_uid
        DoResult do_( const K &key, std::uint32_t uid ) noexcept
        {
            return request( Telegram::empty( Verb::do_, { key.name(), uid } ), {} );
        }

        /** @brief `do <list>.view (first last)`: the rows a list widget shows (`ListWindow`). */
        template<typename K>
            requires is_key<detail::key_type<K>> && detail::key_type<K>::takes_uid
        SendResult view( const K &rows, RowSpan span ) noexcept
        {
            return send_view( rows.name(), span );
        }

        /**
         * @brief Take what the model sent: each `state` into the cache, each
         * `event` to `handler.event(received)` if it has one. At most a
         * ring's worth, so one frame never takes more. The telegrams taken.
         */
        template<typename H>
        std::uint32_t drain( H &handler ) noexcept;

        /** @brief `drain`, with events dropped. */
        std::uint32_t drain() noexcept;

        /** @brief The latest value at `address`, as it came. */
        [[nodiscard]] std::optional<CachedValue> find( Address address ) const noexcept;

        /** @brief The latest value of `key`, as its type (text: valid until the next `drain`). */
        template<typename K>
            requires is_key<detail::key_type<K>> && ( !detail::key_type<K>::takes_uid )
        [[nodiscard]] detail::value_of<K> value( const K &key ) const noexcept
        {
            return read<typename detail::key_type<K>::value_type>( find( { key.name(), 0 } ) );
        }

        /** @brief The latest value of `key` at `uid`, as its type. */
        template<typename K>
            requires is_key<detail::key_type<K>> && detail::key_type<K>::takes_uid
        [[nodiscard]] detail::value_of<K> value( const K &key, std::uint32_t uid ) const noexcept
        {
            return read<typename detail::key_type<K>::value_type>( find( { key.name(), uid } ) );
        }

        /** @brief An actor shows `address`: its value is kept in the cache. */
        [[nodiscard]] bool bind( Address address ) noexcept;
        void unbind( Address address ) noexcept;

        /** @brief Android's lifecycle, for the model: it gets them at its next turn. */
        void background() noexcept;
        void foreground( std::uint64_t elapsed_milliseconds ) noexcept;
        void low_memory() noexcept;
        void quit() noexcept;

        /**
         * @brief The descriptor that becomes readable when the model sends
         * while the UI sleeps: poll it beside the others (Android's looper,
         * a desktop event loop). -1 if it could not be made.
         */
        [[nodiscard]] int wake_fd() const noexcept;

        /**
         * @brief Before sleeping on `wake_fd`: false when the model has sent
         * already (drain instead). True means sleep: the next send wakes it.
         */
        [[nodiscard]] bool ready_to_sleep() noexcept;

        /** @brief After sleeping, woken or not: the model no longer wakes the UI. */
        void woke() noexcept;

        [[nodiscard]] CacheStats cache_stats() const noexcept;
        [[nodiscard]] RingStats to_model_stats() const noexcept;
        [[nodiscard]] RingStats to_ui_stats() const noexcept;
        [[nodiscard]] WaitStats wait_stats() const noexcept;
        [[nodiscard]] ModelStats model_stats() const noexcept;

        /**
         * @brief A walk over every value the cache holds, as
         * `ValueCache::entry_from`: from 0, then from the last found plus 1.
         * A value's text is valid until the next `drain`.
         */
        [[nodiscard]] std::optional<BoundEntry> cached_from( std::uint32_t from ) const noexcept;

    private:
        friend class TelegraphLine;
        explicit TelegraphUi( TelegraphLine &line ) noexcept
            : line_( line )
        {}

        SendResult push( const Telegram &telegram, std::span<const std::byte> data ) noexcept;
        DoResult request( const Telegram &telegram, std::span<const std::byte> data ) noexcept;
        DoResult request_packed( const detail::Packed &packed ) noexcept
        {
            return request( packed.telegram, packed.data );
        }
        SendResult send_view( Name rows, RowSpan span ) noexcept;
        [[nodiscard]] std::optional<Received> next_from_model() const noexcept;
        void pop_from_model() noexcept;
        void take_state( const Received &received ) noexcept;
        void signal( std::uint32_t bit ) noexcept;
        // After the UI sent, signalled or drained: wake the model if it sleeps
        // (`sleeping`) or waits for room (`waiting`).
        void wake_model( bool sleeping, bool waiting ) noexcept;

        template<typename T>
        [[nodiscard]] static std::optional<T>
        read( const std::optional<CachedValue> &got ) noexcept;

        TelegraphLine &line_;
};

/**
 * @brief The Telegraph's storage and its two ends: the line between the UI
 * station and the model's. The App owns one, made once at start (about
 * 90 KB at the default knobs, every array inside it, and two `eventfd`s);
 * the model is handed `model()`, the Stage `ui()`, and neither can reach the
 * other's. A `ModelRunner` runs the model's end.
 */
class TelegraphLine final
{
    public:
        TelegraphLine() noexcept;
        TelegraphLine( const TelegraphLine & ) = delete;
        TelegraphLine &operator=( const TelegraphLine & ) = delete;
        TelegraphLine( TelegraphLine && ) = delete;
        TelegraphLine &operator=( TelegraphLine && ) = delete;
        ~TelegraphLine() = default;

        [[nodiscard]] Telegraph &model() noexcept { return model_; }
        [[nodiscard]] TelegraphUi &ui() noexcept { return ui_; }

    private:
        friend class Telegraph;
        friend class TelegraphUi;
        template<typename M>
        friend class ModelRunner;

        static constexpr std::uint32_t low_memory_bit = 1;
        static constexpr std::uint32_t background_bit = 2;
        static constexpr std::uint32_t foreground_bit = 4;
        static constexpr std::uint32_t quit_bit = 8;

        // Storage first: the rings and caches below are made over it.
        std::array<std::byte, telegraph_size::ring * 32> to_model_slots_{};
        std::array<std::byte, telegraph_size::data> to_model_data_{};
        std::array<std::byte, telegraph_size::ring * 32> to_ui_slots_{};
        std::array<std::byte, telegraph_size::data> to_ui_data_{};
        std::array<std::byte, telegraph_size::cache * 32> cache_entries_{};
        std::array<std::byte, ValueCache::index_bytes( telegraph_size::cache )> cache_index_{};
        std::array<std::byte, telegraph_size::cache_text> cache_text_{};
        std::array<std::byte, telegraph_size::pending * 32> pending_entries_{};
        std::array<std::byte, ValueCache::index_bytes( telegraph_size::pending )> pending_index_{};
        std::array<std::byte, telegraph_size::pending_text> pending_text_{};
        std::array<std::array<std::uint32_t, telegraph_size::list_view>, telegraph_size::lists>
            windows_{};
        std::array<std::uint32_t, telegraph_size::list_view> scratch_{};
        std::array<std::optional<ModelList>, telegraph_size::lists> lists_{};
        std::array<Name, telegraph_size::lists> view_names_{};

        TelegramRing to_model_;
        TelegramRing to_ui_;
        ValueCache cache_;
        ValueCache pending_;

        // Signals, written by the UI and taken by the model.
        std::atomic<std::uint32_t> signals_{ 0 };
        std::atomic<std::uint64_t> elapsed_{ 0 };
        std::atomic<bool> visible_{ true };

        // Who sleeps, for the other to wake (see idle() and ready_to_sleep()).
        std::atomic<bool> model_sleeping_{ false }; // until the UI sends or signals
        std::atomic<bool> model_waiting_{ false };  // until the UI drains
        std::atomic<bool> ui_sleeping_{ false };    // until the model sends
        std::atomic<std::uint32_t> waits_{ 0 };     // the model's, read by the UI
        std::atomic<std::uint32_t> full_{ 0 };      // the model's, read by the UI
        std::atomic<std::uint32_t> held_{ 0 };      // pending_count_, published for the UI
        std::atomic<std::uint32_t> held_highest_{ 0 };
        std::atomic<std::uint32_t> lists_count_{ 0 };
        Wake model_wake_;
        Wake ui_wake_;

        std::uint32_t pending_count_ = 0; // the model's
        std::uint32_t tick_ms_ = 0;       // the model's
        bool quitting_ = false;           // the model's
        bool threaded_ = false;           // set by a ModelRunner before its thread starts
        bool has_runner_ = false;         // the runner's: one to a line
        std::uint16_t correlation_ = 0;   // the UI's

        Telegraph model_;
        TelegraphUi ui_;
};

// ---- the templates, now that the line is complete --------------------------------------------

template<typename M>
void Telegraph::start( M &model ) noexcept
{
    if constexpr( requires { model.start( *this ); } ) {
        model.start( *this );
    }
}

template<typename M>
std::uint32_t Telegraph::deliver( M &model ) noexcept
{
    const Signals signals = take_signals();
    if( signals.low_memory ) {
        if constexpr( requires { model.low_memory( *this ); } ) {
            model.low_memory( *this );
        }
    }
    const auto background = [&] {
        if constexpr( requires { model.background( *this ); } ) {
            model.background( *this );
        }
    };
    const auto foreground = [&] {
        if constexpr( requires { model.foreground( *this, signals.elapsed ); } ) {
            model.foreground( *this, signals.elapsed );
        }
    };
    // Both since the last turn: the one that ends where the app is now goes last.
    if( signals.background && signals.foreground ) {
        if( signals.visible ) {
            background();
            foreground();
        } else {
            foreground();
            background();
        }
    } else if( signals.background ) {
        background();
    } else if( signals.foreground ) {
        foreground();
    }
    if( signals.quit ) {
        if( signals.visible && !signals.background ) {
            background(); // quit saves
        }
        mark_quitting();
    }

    flush();
    std::uint32_t handled = 0;
    while( handled < telegraph_size::ring ) {
        const auto received = next_from_ui();
        if( !received ) {
            break;
        }
        if( !answer_view( *received ) ) {
            model.receive( *this, *received );
        }
        pop_from_ui();
        ++handled;
    }
    return handled;
}

template<typename H>
std::uint32_t TelegraphUi::drain( H &handler ) noexcept
{
    std::uint32_t taken = 0;
    while( taken < telegraph_size::ring ) {
        const auto received = next_from_model();
        if( !received ) {
            break;
        }
        if( received->telegram.verb() == Verb::state ) {
            take_state( *received );
        } else if constexpr( requires { handler.event( *received ); } ) {
            if( received->telegram.verb() == Verb::event ) {
                handler.event( *received );
            }
        }
        pop_from_model();
        ++taken;
    }
    if( taken != 0 ) {
        wake_model( false, true ); // room, for a model waiting for it
    }
    return taken;
}

template<typename T>
std::optional<T> TelegraphUi::read( const std::optional<CachedValue> &got ) noexcept
{
    using Uids = std::span<const std::uint32_t>;
    static_assert( !std::is_same_v<T, Uids>, "a uid list is read with find(), 4 bytes a uid" );
    static_assert( !std::is_same_v<T, void>, "a key with no value has nothing to read" );
    if( !got ) {
        return std::nullopt;
    }
    const Telegram &t = got->telegram;
    if constexpr( std::is_same_v<T, bool> ) {
        return t.type() == ValueType::boolean ? std::optional<T>( t.as_boolean() ) : std::nullopt;
    } else if constexpr( std::is_same_v<T, std::int64_t> ) {
        return t.type() == ValueType::integer ? std::optional<T>( t.as_integer() ) : std::nullopt;
    } else if constexpr( std::is_same_v<T, double> ) {
        return t.type() == ValueType::number ? std::optional<T>( t.as_number() ) : std::nullopt;
    } else if constexpr( std::is_same_v<T, std::string_view> ) {
        if( t.type() != ValueType::text ) {
            return std::nullopt;
        }
        const auto *chars = reinterpret_cast<const char *>( got->data.data() );
        return std::string_view( chars, got->data.size() );
    } else {
        constexpr std::size_t n = std::tuple_size_v<T>; // std::array<float, 2..4>
        if( float_count( t.type() ) != n ) {
            return std::nullopt;
        }
        const std::array<float, 4> all = t.as_floats();
        T floats{};
        for( std::size_t i = 0; i < n; ++i ) {
            floats[i] = all[i];
        }
        return floats;
    }
}

} // namespace sq::messaging
