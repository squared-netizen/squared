# class DeadlineQueue

*Defined at [time/deadline_queue.hpp:35](../../../../time/deadline_queue.hpp#L35)*



**brief** A model's timers: values due at times, given back in order once their time has come. It holds at most `Capacity` at once, in an array of its own, so nothing is ever allocated: `DeadlineQueue<Restock, 16> timers;`

 It reads no clock and runs no thread: its owner passes the time in. In a model, `next_due()` is the model's `wake_at()`, and `poll_due` runs in its `due()` (see `ModelRunner`). Entries due at the same time come back in the order they were scheduled. Values are not callbacks: what a due value means is its owner's business.



## Records

Scheduled

Ticket



## Functions

### size

*public std::size_t size()*

*Defined at [time/deadline_queue.hpp:73](../../../../time/deadline_queue.hpp#L73)*

### capacity

*public static std::size_t capacity()*

*Defined at [time/deadline_queue.hpp:74](../../../../time/deadline_queue.hpp#L74)*

### empty

*public bool empty()*

*Defined at [time/deadline_queue.hpp:75](../../../../time/deadline_queue.hpp#L75)*

### next_due

*public std::optional<TimePoint> next_due()*

*Defined at [time/deadline_queue.hpp:78](../../../../time/deadline_queue.hpp#L78)*



**brief** When the first entry is due; nothing when the queue is empty. 

### schedule_at

*public Scheduled schedule_at(TimePoint due, Value value)*

*Defined at [time/deadline_queue.hpp:87](../../../../time/deadline_queue.hpp#L87)*



**brief** `value`, due at `due` (not before the epoch). 

### schedule_after

*public Scheduled schedule_after(TimePoint now, Duration delay, Value value)*

*Defined at [time/deadline_queue.hpp:109](../../../../time/deadline_queue.hpp#L109)*



**brief** `value`, due `delay` after `now` (the owner's one reading of the clock). 

### cancel

*public bool cancel(Ticket ticket)*

*Defined at [time/deadline_queue.hpp:119](../../../../time/deadline_queue.hpp#L119)*



**brief** Cancel the entry `ticket` names, freeing its slot. False if it is not here (due already, or cancelled). 

### cancel

*public bool cancel(Ticket ticket, Visitor && visitor)*

*Defined at [time/deadline_queue.hpp:126](../../../../time/deadline_queue.hpp#L126)*



**brief** Cancel, handing the value to `visitor` first (to release what it holds). 

### poll_due

*public std::size_t poll_due(TimePoint now, std::size_t limit, Visitor && visitor)*

*Defined at [time/deadline_queue.hpp:154](../../../../time/deadline_queue.hpp#L154)*

 NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)

### visit_ordered

*public void visit_ordered(Visitor && visitor)*

*Defined at [time/deadline_queue.hpp:176](../../../../time/deadline_queue.hpp#L176)*

 NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)



## Enums

| enum class ScheduleResult |

--

| scheduled |
| full |
| invalid_time |
| tickets_exhausted |


*Defined at [time/deadline_queue.hpp:50](../../../../time/deadline_queue.hpp#L50)*



**brief** What scheduling did. 



