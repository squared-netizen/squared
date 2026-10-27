#pragma once

#include <squared/messaging/key.hpp>
#include <squared/messaging/src/pack.hpp>
#include <squared/messaging/src/verb.hpp>
#include <squared/messaging/telegram_ring.hpp>
#include <squared/messaging/telegraph.hpp>
#include <squared/time/time_point.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sq
{

/**
 * @brief What a model keeps when the app goes to the background: its
 * `save( sq::Save &save )`, if it has one, writes the values it wants back.
 *
 * ```cpp
 * void save( sq::Save &save ) noexcept
 * {
 *     save.state( settings_sound, sound_ );
 *     save.state( settings_volume, volume_ );
 * }
 * ```
 *
 * Each is kept as a `set` line (`set settings.volume 5.0`), and on the next
 * start, before the model's `start`, each comes back to its `receive` as the
 * UI's `set` would: a key the model takes `set` on is what it keeps, and
 * nothing new is written to read them back.
 *
 * Over the model thread's stack (`capacity` bytes); it allocates nothing.
 */
class Save final
{
    public:
        /** @brief The bytes a save holds: when a line does not fit, the file is not written. */
        static constexpr std::size_t capacity = 8192;

        explicit Save( std::span<char> out ) noexcept
            : out_( out )
        {}

        /** @brief Keep `value` for the key `key`, which takes `set`. */
        template<typename K, typename V>
            requires messaging::sendable<K, messaging::Verb::set, V> &&
                     ( !messaging::detail::key_type<K>::takes_uid )
        void state( const K &key, const V &value ) noexcept
        {
            using T = typename messaging::detail::key_type<K>::value_type;
            keep( messaging::detail::pack<T>( messaging::Verb::set, { key.name(), 0 }, value ),
                  key.text() );
        }

        /** @brief Keep `value` for item `uid` of the key `key`, which takes `set` and a uid. */
        template<typename K, typename V>
            requires messaging::sendable<K, messaging::Verb::set, V> &&
                     messaging::detail::key_type<K>::takes_uid
        void state( const K &key, std::uint32_t uid, const V &value ) noexcept
        {
            using T = typename messaging::detail::key_type<K>::value_type;
            keep( messaging::detail::pack<T>( messaging::Verb::set, { key.name(), uid }, value ),
                  key.text() );
        }

        /** @brief The lines written, each ended by '\n'. */
        [[nodiscard]] std::string_view text() const noexcept { return { out_.data(), used_ }; }

        /** @brief How many values are kept. */
        [[nodiscard]] std::size_t lines() const noexcept { return lines_; }

        /** @brief A value did not fit: the save is not written, and the last one stays. */
        [[nodiscard]] bool full() const noexcept { return full_; }

    private:
        void keep( const messaging::detail::Packed &packed, std::string_view name ) noexcept;

        std::span<char> out_;
        std::size_t used_ = 0;
        std::size_t lines_ = 0;
        bool full_ = false;
};

/** @brief A model that keeps what it has: it has `save( sq::Save & )`. */
template<typename M>
concept SavingModel = requires( M &model, Save &save ) { model.save( save ); };

namespace detail
{

/** @brief Where a model's save is kept, and what the last one held: handed to the model. */
struct ModelFile final
{
        /// The platform's path of the file; empty: nowhere to keep it, nothing is written.
        std::string path;
        /// The last save's lines, given back before the model's `start`, then freed.
        std::vector<std::byte> saved;
        /// Set when a save could not be written: the App says so when the app comes back.
        std::atomic<bool> *failed = nullptr;
};

/** @brief Each `set` line of `saved` to `deliver`, as a telegram from the UI would come. */
void give_back( std::span<const std::byte> saved,
                void ( *deliver )( void *context, const messaging::Received &received ) noexcept,
                void *context ) noexcept;

/** @brief Write a save's lines to `file`'s path, whole or not at all; false when it cannot. */
[[nodiscard]] bool write_save( const ModelFile &file, std::string_view text ) noexcept;

/**
 * @brief The model `M` as the App runs one that saves: the model's own hooks
 * forwarded, the last save given back before its `start`, and `save` called
 * after each `background` (a quit's too), on the model's thread.
 */
template<SavingModel M>
class Saving final
{
    public:
        template<typename... Args>
        explicit Saving( ModelFile file, Args &&...args ) noexcept
            : file_( std::move( file ) )
            , model_( std::forward<Args>( args )... )
        {}

        void start( messaging::Telegraph &telegraph ) noexcept
        {
            struct Giving final
            {
                    Saving *self;
                    messaging::Telegraph *telegraph;
            };
            Giving giving{ this, &telegraph };
            const auto deliver = []( void *context, const messaging::Received &received ) noexcept {
                auto &to = *static_cast<Giving *>( context );
                to.self->model_.receive( *to.telegraph, received );
            };
            give_back( file_.saved, deliver, &giving );
            std::vector<std::byte>().swap( file_.saved );
            if constexpr( requires { model_.start( telegraph ); } ) {
                model_.start( telegraph );
            }
        }

        void receive( messaging::Telegraph &telegraph,
                      const messaging::Received &received ) noexcept
        {
            model_.receive( telegraph, received );
        }

        void background( messaging::Telegraph &telegraph ) noexcept
        {
            if constexpr( requires { model_.background( telegraph ); } ) {
                model_.background( telegraph );
            }
            if( file_.path.empty() ) {
                return;
            }
            std::array<char, Save::capacity> out{};
            Save save( out );
            model_.save( save );
            if( save.full() || !write_save( file_, save.text() ) ) {
                if( file_.failed != nullptr ) {
                    file_.failed->store( true, std::memory_order_relaxed );
                }
            }
        }

        void tick( messaging::Telegraph &telegraph, double seconds ) noexcept
            requires requires( M &model, messaging::Telegraph &line ) { model.tick( line, 0.0 ); }
        {
            model_.tick( telegraph, seconds );
        }

        void low_memory( messaging::Telegraph &telegraph ) noexcept
            requires requires( M &model, messaging::Telegraph &line ) { model.low_memory( line ); }
        {
            model_.low_memory( telegraph );
        }

        void foreground( messaging::Telegraph &telegraph, std::uint64_t elapsed ) noexcept
            requires requires( M &model, messaging::Telegraph &line ) {
                model.foreground( line, std::uint64_t{} );
            }
        {
            model_.foreground( telegraph, elapsed );
        }

        [[nodiscard]] std::optional<time::TimePoint> wake_at() noexcept
            requires requires( M &model ) { model.wake_at(); }
        {
            return model_.wake_at();
        }

        void due( messaging::Telegraph &telegraph, time::TimePoint now ) noexcept
            requires requires( M &model, messaging::Telegraph &line, time::TimePoint at ) {
                model.due( line, at );
            }
        {
            model_.due( telegraph, now );
        }

    private:
        ModelFile file_;
        M model_;
};

} // namespace detail

} // namespace sq
