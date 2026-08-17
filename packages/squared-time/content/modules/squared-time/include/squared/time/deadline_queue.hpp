#pragma once

#include <squared/time/timepiece.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace squared::time {

/**
 * @brief Bounded deterministic queue used by higher-level schedulers.
 *
 * Entries are ordered by due time and then insertion sequence. The queue does
 * not read a clock or run a thread; its owner captures time once and passes it
 * to poll_due(). Stored values are not callbacks, keeping policy in the
 * higher-level subsystem.
 */
template<typename Value>
class DeadlineQueue {
public:
    /**
     * @brief Opaque cancellation identity for one scheduled entry.
     *
     * A zero-valued ticket is empty and never matches a scheduled entry.
     */
    struct Ticket {
        /** @brief Non-zero monotonic sequence; zero means empty/unset. */
        std::uint64_t value{0};

        /**
         * @brief Check whether this ticket references a scheduled entry.
         * @return true when the ticket is non-zero and usable.
         */
        [[nodiscard]]
        explicit operator bool() const noexcept
        {
            return value != 0;
        }
    };

    /** @brief Result of attempting to schedule or restore an entry. */
    enum class ScheduleResult {
        Scheduled,
        QueueFull,
        InvalidTime,
        TicketExhausted
    };

    /**
     * @brief Construct a queue with fixed logical capacity.
     * @param capacity Maximum number of simultaneously scheduled entries.
     */
    explicit DeadlineQueue(std::size_t capacity)
        : capacity_(capacity)
    {
        entries_.reserve(capacity);
    }

    /**
     * @brief Count currently scheduled entries.
     * @return Number of scheduled entries, at most the configured capacity.
     */
    [[nodiscard]] std::size_t size() const noexcept
    {
        return entries_.size();
    }

    /**
     * @brief Read the fixed logical capacity.
     * @return Maximum number of simultaneously scheduled entries.
     */
    [[nodiscard]] std::size_t capacity() const noexcept
    {
        return capacity_;
    }

    /**
     * @brief Check whether the queue holds no entries.
     * @return true when no entries are scheduled.
     */
    [[nodiscard]] bool empty() const noexcept
    {
        return entries_.empty();
    }

    /**
     * @brief Schedule an entry at an exact domain time.
     * @param due Nanoseconds since the domain epoch; must be non-negative.
     * @param value Storage delivered later; moved through the queue.
     * @param ticket Optional out-parameter filled with the cancellation
     * ticket on success; left unchanged on failure.
     * @return ScheduleResult::Scheduled on success, or QueueFull,
     * InvalidTime, or TicketExhausted.
     */
    ScheduleResult schedule_at(
        TimePoint due,
        Value value,
        Ticket* ticket = nullptr
    )
    {
        if (due.count() < 0) return ScheduleResult::InvalidTime;
        if (entries_.size() >= capacity_) {
            return ScheduleResult::QueueFull;
        }
        if (next_sequence_ ==
            std::numeric_limits<std::uint64_t>::max()) {
            return ScheduleResult::TicketExhausted;
        }

        const std::uint64_t sequence = ++next_sequence_;
        entries_.push_back(
            Entry{due, sequence, std::move(value)}
        );
        std::push_heap(
            entries_.begin(),
            entries_.end(),
            Later{}
        );
        if (ticket) ticket->value = sequence;
        return ScheduleResult::Scheduled;
    }

    /**
     * @brief Schedule an entry a fixed delay after the clock's current time.
     * @param clock Domain clock read exactly once to compute the deadline.
     * @param delay Non-negative delay in nanoseconds.
     * @param value Storage delivered later; moved through the queue.
     * @param ticket Optional out-parameter filled with the cancellation
     * ticket on success; left unchanged on failure.
     * @return ScheduleResult::Scheduled on success, or QueueFull, InvalidTime,
     * or TicketExhausted.
     */
    ScheduleResult schedule_after(
        const Clock& clock,
        Duration delay,
        Value value,
        Ticket* ticket = nullptr
    )
    {
        if (delay.count() < 0) return ScheduleResult::InvalidTime;
        const TimePoint current = clock.now();
        const auto maximum =
            std::numeric_limits<Duration::rep>::max();
        if (current.count() < 0 ||
            delay.count() > maximum - current.count()) {
            return ScheduleResult::InvalidTime;
        }
        return schedule_at(
            current + delay,
            std::move(value),
            ticket
        );
    }

    /**
     * @brief Cancel a scheduled entry and reclaim its capacity.
     * @param ticket Ticket returned when the entry was scheduled.
     * @return `true` when the entry was found and removed.
     */
    [[nodiscard]] bool cancel(Ticket ticket) noexcept
    {
        if (!ticket) return false;
        for (auto iterator = entries_.begin();
             iterator != entries_.end();
             ++iterator) {
            if (iterator->sequence == ticket.value) {
                entries_.erase(iterator);
                std::make_heap(
                    entries_.begin(),
                    entries_.end(),
                    Later{}
                );
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Cancel a scheduled entry and release its stored value.
     * @param ticket Ticket returned when the entry was scheduled.
     * @param visitor Called with the cancelled value before it is destroyed.
     * @return `true` when the entry was found and removed.
     */
    template<typename Visitor>
    bool cancel(Ticket ticket, Visitor&& visitor)
    {
        if (!ticket) return false;
        for (auto iterator = entries_.begin();
             iterator != entries_.end();
             ++iterator) {
            if (iterator->sequence == ticket.value) {
                Value cancelled = std::move(iterator->value);
                entries_.erase(iterator);
                std::make_heap(
                    entries_.begin(),
                    entries_.end(),
                    Later{}
                );
                visitor(cancelled);
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Deliver at most limit due values using one captured timestamp.
     *
     * Cancellation removes entries immediately and reclaims their capacity.
     *
     * @param captured_now The owner's single per-update clock reading.
     * @param limit Maximum number of values delivered by this call.
     * @param visitor Called with each due value in delivery order.
     * @return The number of values delivered.
     */
    template<typename Visitor>
    std::size_t poll_due(
        TimePoint captured_now,
        std::size_t limit,
        Visitor&& visitor
    )
    {
        std::size_t delivered = 0;
        while (!entries_.empty()) {
            const Entry& next = entries_.front();
            if (next.due > captured_now) break;

            std::pop_heap(
                entries_.begin(),
                entries_.end(),
                Later{}
            );
            Entry entry = std::move(entries_.back());
            entries_.pop_back();
            if (delivered >= limit) {
                entries_.push_back(std::move(entry));
                std::push_heap(
                    entries_.begin(),
                    entries_.end(),
                    Later{}
                );
                break;
            }
            visitor(std::move(entry.value));
            ++delivered;
        }
        return delivered;
    }

    /**
     * @brief Visit entries in delivery order without mutating the queue.
     *
     * Inspection allocates only a bounded array of entry pointers. References
     * passed to the visitor remain valid only for the duration of the call.
     *
     * @param visitor Called with (due time, ticket, value) for every entry.
     */
    template<typename Visitor>
    void visit_ordered(Visitor&& visitor) const
    {
        std::vector<const Entry*> ordered;
        ordered.reserve(entries_.size());
        for (const auto& entry : entries_) {
            ordered.push_back(&entry);
        }
        std::sort(
            ordered.begin(),
            ordered.end(),
            [](const Entry* left, const Entry* right) {
                if (left->due != right->due) {
                    return left->due < right->due;
                }
                return left->sequence < right->sequence;
            }
        );
        for (const Entry* entry : ordered) {
            visitor(
                entry->due,
                Ticket{entry->sequence},
                entry->value
            );
        }
    }

private:
    struct Entry {
        TimePoint due;
        std::uint64_t sequence;
        Value value;
    };

    struct Later {
        bool operator()(
            const Entry& left,
            const Entry& right
        ) const noexcept
        {
            if (left.due != right.due) {
                return left.due > right.due;
            }
            return left.sequence > right.sequence;
        }
    };

    std::vector<Entry> entries_;
    std::size_t capacity_;
    std::uint64_t next_sequence_{0};
};

}  // namespace squared::time
