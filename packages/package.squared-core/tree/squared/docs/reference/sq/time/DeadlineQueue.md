# class DeadlineQueue

*Defined at [time/src/deadline_queue.hpp:26](../../../../time/src/deadline_queue.hpp#L26)*



**brief** Bounded deterministic queue used by higher-level schedulers.

 Entries are ordered by due time and then insertion sequence. The queue does not read a clock or run a thread; its owner captures time once and passes it to poll_due(). Stored values are not callbacks, keeping policy in the higher-level subsystem.



## Records

Entry

Ticket



## Functions

### DeadlineQueue<Value>

*public void DeadlineQueue<Value>(int capacity)*

*Defined at [time/src/deadline_queue.hpp:62](../../../../time/src/deadline_queue.hpp#L62)*



**brief** Construct a queue with fixed logical capacity.

**capacity** Maximum number of simultaneously scheduled entries.

### size

*public int size()*

*Defined at [time/src/deadline_queue.hpp:72](../../../../time/src/deadline_queue.hpp#L72)*



**brief** Count currently scheduled entries.

**return** Number of scheduled entries, at most the configured capacity.

### capacity

*public int capacity()*

*Defined at [time/src/deadline_queue.hpp:78](../../../../time/src/deadline_queue.hpp#L78)*



**brief** Read the fixed logical capacity.

**return** Maximum number of simultaneously scheduled entries.

### empty

*public bool empty()*

*Defined at [time/src/deadline_queue.hpp:84](../../../../time/src/deadline_queue.hpp#L84)*



**brief** Check whether the queue holds no entries.

**return** true when no entries are scheduled.

### schedule_at

*public ScheduleResult schedule_at(int due, Value value, Ticket * ticket)*

*Defined at [time/src/deadline_queue.hpp:95](../../../../time/src/deadline_queue.hpp#L95)*



**brief** Schedule an entry at an exact domain time.

**due** Nanoseconds since the domain epoch; must be non-negative.

**value** Storage delivered later; moved through the queue.

**ticket** Optional out-parameter filled with the cancellation ticket on success; left unchanged on failure.

**return** ScheduleResult::Scheduled on success, or QueueFull, InvalidTime, or TicketExhausted.

### schedule_after

*public ScheduleResult schedule_after(const Clock & clock, int delay, Value value, Ticket * ticket)*

*Defined at [time/src/deadline_queue.hpp:126](../../../../time/src/deadline_queue.hpp#L126)*



**brief** Schedule an entry a fixed delay after the clock's current time.

**clock** Domain clock read exactly once to compute the deadline.

**delay** Non-negative delay in nanoseconds.

**value** Storage delivered later; moved through the queue.

**ticket** Optional out-parameter filled with the cancellation ticket on success; left unchanged on failure.

**return** ScheduleResult::Scheduled on success, or QueueFull, InvalidTime, or TicketExhausted.

### cancel

*public bool cancel(Ticket ticket)*

*Defined at [time/src/deadline_queue.hpp:145](../../../../time/src/deadline_queue.hpp#L145)*



**brief** Cancel a scheduled entry and reclaim its capacity.

**ticket** Ticket returned when the entry was scheduled.

**return** `true` when the entry was found and removed.

### cancel

*public bool cancel(Ticket ticket, Visitor && visitor)*

*Defined at [time/src/deadline_queue.hpp:167](../../../../time/src/deadline_queue.hpp#L167)*



**brief** Cancel a scheduled entry and release its stored value.

**ticket** Ticket returned when the entry was scheduled.

**visitor** Called with the cancelled value before it is destroyed.

**return** `true` when the entry was found and removed.

### poll_due

*public int poll_due(int captured_now, int limit, Visitor && visitor)*

*Defined at [time/src/deadline_queue.hpp:197](../../../../time/src/deadline_queue.hpp#L197)*

 NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)

### visit_ordered

*public void visit_ordered(Visitor && visitor)*

*Defined at [time/src/deadline_queue.hpp:231](../../../../time/src/deadline_queue.hpp#L231)*

 NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)



## Enums

| enum class ScheduleResult |

--

| Scheduled |
| QueueFull |
| InvalidTime |
| TicketExhausted |


*Defined at [time/src/deadline_queue.hpp:51](../../../../time/src/deadline_queue.hpp#L51)*



**brief** Result of attempting to schedule or restore an entry. 



