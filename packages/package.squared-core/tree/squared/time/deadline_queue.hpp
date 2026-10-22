#pragma once

#include <squared/time/duration.hpp>
#include <squared/time/time_point.hpp>

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>

namespace sq::time
{

/** @brief What a `DeadlineQueue` can hold: made empty, and moved without throwing. */
template<typename T>
concept DeadlineValue = std::default_initializable<T> && std::is_nothrow_move_constructible_v<T>;

/**
 * @brief A model's timers: values due at times, given back in order once
 * their time has come. It holds at most `Capacity` at once, in an array of
 * its own, so nothing is ever allocated: `DeadlineQueue<Restock, 16> timers;`
 *
 * It reads no clock and runs no thread: its owner passes the time in. In a
 * model, `next_due()` is the model's `wake_at()`, and `poll_due` runs in its
 * `due()` (see `ModelRunner`). Entries due at the same time come back in the
 * order they were scheduled. Values are not callbacks: what a due value means
 * is its owner's business.
 */
template<DeadlineValue Value, std::size_t Capacity>
class DeadlineQueue final
{
        static_assert( Capacity >= 1, "a DeadlineQueue holds at least one entry" );
        static_assert( std::is_nothrow_move_assignable_v<Value>, "a value moves without throwing" );

    public:
        /** @brief Names a scheduled entry, to cancel it. Zero names none. */
        struct Ticket
        {
                std::uint64_t value = 0;

                [[nodiscard]] explicit operator bool() const noexcept { return value != 0; }
        };

        /** @brief What scheduling did. */
        enum class ScheduleResult : std::uint8_t {
            /** In the queue. */
            scheduled,
            /** Every entry of the storage is in use. */
            full,
            /** Before the epoch, or past the largest time. */
            invalid_time,
            /** 2^64 - 1 entries scheduled over its life: none more. */
            tickets_exhausted,
        };

        /** @brief The result, and the ticket when scheduled. */
        struct Scheduled
        {
                ScheduleResult result = ScheduleResult::full;
                Ticket ticket{};

                [[nodiscard]] explicit operator bool() const noexcept
                {
                    return result == ScheduleResult::scheduled;
                }
        };

        [[nodiscard]] std::size_t size() const noexcept { return size_; }
        [[nodiscard]] static constexpr std::size_t capacity() noexcept { return Capacity; }
        [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

        /** @brief When the first entry is due; nothing when the queue is empty. */
        [[nodiscard]] std::optional<TimePoint> next_due() const noexcept
        {
            if( size_ == 0 ) {
                return std::nullopt;
            }
            return storage_.front().due;
        }

        /** @brief `value`, due at `due` (not before the epoch). */
        Scheduled schedule_at( TimePoint due, Value value ) noexcept
        {
            if( due.count() < 0 ) {
                return { ScheduleResult::invalid_time, {} };
            }
            if( size_ == Capacity ) {
                return { ScheduleResult::full, {} };
            }
            if( next_sequence_ == std::numeric_limits<std::uint64_t>::max() ) {
                return { ScheduleResult::tickets_exhausted, {} };
            }
            const std::uint64_t sequence = ++next_sequence_;
            Entry &slot = storage_[size_];
            slot.due = due;
            slot.sequence = sequence;
            slot.value = std::move( value );
            ++size_;
            std::push_heap( storage_.begin(), end(), Later{} );
            return { ScheduleResult::scheduled, Ticket{ sequence } };
        }

        /** @brief `value`, due `delay` after `now` (the owner's one reading of the clock). */
        Scheduled schedule_after( TimePoint now, Duration delay, Value value ) noexcept
        {
            constexpr auto largest = std::numeric_limits<Duration::rep>::max();
            if( delay.count() < 0 || now.count() < 0 || delay.count() > largest - now.count() ) {
                return { ScheduleResult::invalid_time, {} };
            }
            return schedule_at( now + delay, std::move( value ) );
        }

        /** @brief Cancel the entry `ticket` names, freeing its slot. False if it is not here (due already, or cancelled). */
        [[nodiscard]] bool cancel( Ticket ticket ) noexcept
        {
            return cancel( ticket, []( Value & ) noexcept {} );
        }

        /** @brief Cancel, handing the value to `visitor` first (to release what it holds). */
        template<typename Visitor>
        bool cancel( Ticket ticket, Visitor &&visitor )
        {
            if( !ticket ) {
                return false;
            }
            for( std::size_t i = 0; i < size_; ++i ) {
                if( storage_[i].sequence == ticket.value ) {
                    Value cancelled = std::move( storage_[i].value );
                    --size_;
                    if( i != size_ ) {
                        storage_[i] = std::move( storage_[size_] );
                    }
                    storage_[size_].value = Value{};
                    std::make_heap( storage_.begin(), end(), Later{} );
                    std::forward<Visitor>( visitor )( cancelled );
                    return true;
                }
            }
            return false;
        }

        /**
         * @brief Hand each entry due at `now` or before to `visitor`, in
         * order, at most `limit` of them. The number handed over.
         */
        // Called once per due entry: forwarding would let the first call move it away.
        template<typename Visitor>
        // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
        std::size_t poll_due( TimePoint now, std::size_t limit, Visitor &&visitor )
        {
            std::size_t delivered = 0;
            while( size_ != 0 && delivered < limit && storage_.front().due <= now ) {
                std::pop_heap( storage_.begin(), end(), Later{} );
                --size_;
                Value value = std::move( storage_[size_].value );
                storage_[size_].value = Value{};
                visitor( std::move( value ) );
                ++delivered;
            }
            return delivered;
        }

        /**
         * @brief Every entry, in the order they will come due:
         * `visitor(due, ticket, value)`. Allocates nothing: a selection walk,
         * O(n²), for the few timers a Console command shows.
         */
        // Called once per entry: forwarding would let the first call move it away.
        template<typename Visitor>
        // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
        void visit_ordered( Visitor &&visitor ) const
        {
            const Entry *previous = nullptr;
            for( std::size_t visited = 0; visited < size_; ++visited ) {
                const Entry *next = nullptr;
                for( std::size_t i = 0; i < size_; ++i ) {
                    const Entry &entry = storage_[i];
                    const bool after = previous == nullptr || Later{}( entry, *previous );
                    if( after && ( next == nullptr || Later{}( *next, entry ) ) ) {
                        next = &entry;
                    }
                }
                visitor( next->due, Ticket{ next->sequence }, next->value );
                previous = next;
            }
        }

    private:
        struct Entry
        {
                TimePoint due{};
                std::uint64_t sequence = 0;
                Value value{};
        };

        // Due later, or due at the same time and scheduled later: the heap's
        // front is the entry due first.
        struct Later
        {
                bool operator()( const Entry &left, const Entry &right ) const noexcept
                {
                    if( left.due != right.due ) {
                        return left.due > right.due;
                    }
                    return left.sequence > right.sequence;
                }
        };

        [[nodiscard]] auto end() noexcept
        {
            return storage_.begin() + static_cast<std::ptrdiff_t>( size_ );
        }

        std::array<Entry, Capacity> storage_{};
        std::size_t size_ = 0;
        std::uint64_t next_sequence_ = 0;
};

} // namespace sq::time
