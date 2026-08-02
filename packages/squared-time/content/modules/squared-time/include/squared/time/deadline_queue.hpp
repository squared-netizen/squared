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
    struct Ticket {
        std::uint64_t value{0};

        [[nodiscard]]
        explicit operator bool() const noexcept
        {
            return value != 0;
        }
    };

    enum class ScheduleResult {
        Scheduled,
        QueueFull,
        InvalidTime,
        TicketExhausted
    };

    explicit DeadlineQueue(std::size_t capacity)
        : capacity_(capacity)
    {
        entries_.reserve(capacity);
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return entries_.size();
    }

    [[nodiscard]] std::size_t capacity() const noexcept
    {
        return capacity_;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return entries_.empty();
    }

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
