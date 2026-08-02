#include <squared/time/deadline_queue.hpp>
#include <squared/time/timepiece.hpp>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

namespace {

using namespace std::chrono_literals;

void require(bool condition, const char* message)
{
    if (condition) return;
    std::cerr << "time test failed: " << message << '\n';
    std::exit(1);
}

}  // namespace

int main()
{
    squared::time::Timepiece clock;
    require(clock.now() == 0ns, "new clock begins at zero");
    clock.advance(1s);
    require(clock.now() == 1s, "clock advances once");

    clock.pause();
    clock.advance(4s);
    require(clock.now() == 1s, "paused clock does not advance");
    clock.resume();
    require(clock.set_time_scale(0.5), "valid scale accepted");
    clock.advance(2s);
    require(clock.now() == 2s, "scaled clock advances");
    require(
        !clock.set_time_scale(
            std::numeric_limits<double>::quiet_NaN()
        ),
        "non-finite scale rejected"
    );
    require(
        !clock.set_time_scale(-1.0),
        "negative scale rejected"
    );

    squared::time::ManualTimepiece manual;
    manual.reset(10s);
    require(manual.now() == 10s, "manual clock resets");

    using Queue = squared::time::DeadlineQueue<int>;
    Queue queue(4);
    Queue::Ticket cancelled;
    require(
        queue.schedule_at(3s, 30) ==
            Queue::ScheduleResult::Scheduled,
        "later entry schedules"
    );
    require(
        queue.schedule_at(1s, 10, &cancelled) ==
            Queue::ScheduleResult::Scheduled,
        "first equal entry schedules"
    );
    require(
        queue.schedule_at(1s, 11) ==
            Queue::ScheduleResult::Scheduled,
        "second equal entry schedules"
    );
    int cancelled_value = 0;
    require(
        queue.cancel(
            cancelled,
            [&cancelled_value](const int& value) {
                cancelled_value = value;
            }
        ),
        "ticket cancels"
    );
    require(cancelled_value == 10, "cancel exposes removed value");
    require(!queue.cancel(cancelled), "ticket cancels once");
    require(
        queue.schedule_at(2s, 20) ==
            Queue::ScheduleResult::Scheduled,
        "fourth entry schedules"
    );
    require(
        queue.schedule_at(4s, 40) ==
            Queue::ScheduleResult::Scheduled,
        "cancellation reclaims queue capacity"
    );
    require(
        queue.schedule_at(5s, 50) ==
            Queue::ScheduleResult::QueueFull,
        "queue capacity is enforced"
    );

    std::vector<int> delivered;
    const auto first_count = queue.poll_due(
        3s,
        1,
        [&delivered](int value) {
            delivered.push_back(value);
        }
    );
    require(first_count == 1, "delivery limit is enforced");
    require(
        delivered == std::vector<int>{11},
        "cancelled equal-time entry is skipped"
    );
    const auto second_count = queue.poll_due(
        3s,
        8,
        [&delivered](int value) {
            delivered.push_back(value);
        }
    );
    require(second_count == 2, "remaining due entries deliver");
    require(
        delivered == std::vector<int>({11, 20, 30}),
        "due and insertion ordering is deterministic"
    );
    require(!queue.empty(), "future entry remains queued");
    require(
        queue.poll_due(
            4s,
            1,
            [&delivered](int value) {
                delivered.push_back(value);
            }
        ) == 1,
        "future entry delivers when due"
    );
    require(
        delivered == std::vector<int>({11, 20, 30, 40}),
        "future delivery preserves value"
    );
    require(queue.empty(), "queue drains");

    Queue overflow(1);
    manual.reset(squared::time::TimePoint(
        std::numeric_limits<
            squared::time::Duration::rep
        >::max()
    ));
    require(
        overflow.schedule_after(manual, 1ns, 1) ==
            Queue::ScheduleResult::InvalidTime,
        "deadline overflow is rejected"
    );

    std::cout << "Squared time domains and deadline queue: OK\n";
    return 0;
}
