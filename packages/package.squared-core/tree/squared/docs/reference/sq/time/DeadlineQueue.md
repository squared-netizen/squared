# class DeadlineQueue

*Defined at [time/deadline_queue.cppm:26](../../../../time/deadline_queue.cppm#L26)*



**brief** A model's timers: values due at times, given back in order once their time has come. It holds at most `Capacity` at once, in an array of its own, so nothing is ever allocated: `DeadlineQueue<Restock, 16> timers;`

 It reads no clock and runs no thread: its owner passes the time in. In a model, `next_due()` is the model's `wake_at()`, and `poll_due` runs in its `due()` (see `ModelRunner`). Entries due at the same time come back in the order they were scheduled. Values are not callbacks: what a due value means is its owner's business.



## Records

Scheduled

Ticket



## Functions

### cancel

*public bool cancel(Ticket ticket)*

*Defined at [time/deadline_queue.cppm:110](../../../../time/deadline_queue.cppm#L110)*



**brief** Cancel the entry `ticket` names, freeing its slot. False if it is not here (due already, or cancelled). 

### cancel

*public bool cancel(Ticket ticket, Visitor && visitor)*

*Defined at [time/deadline_queue.cppm:117](../../../../time/deadline_queue.cppm#L117)*



**brief** Cancel, handing the value to `visitor` first (to release what it holds). 

### capacity

*public static std::size_t capacity()*

*Defined at [time/deadline_queue.cppm:65](../../../../time/deadline_queue.cppm#L65)*

### empty

*public bool empty()*

*Defined at [time/deadline_queue.cppm:66](../../../../time/deadline_queue.cppm#L66)*

### next_due

*public std::optional<TimePoint> next_due()*

*Defined at [time/deadline_queue.cppm:69](../../../../time/deadline_queue.cppm#L69)*



**brief** When the first entry is due; nothing when the queue is empty. 

### poll_due

*public std::size_t poll_due(TimePoint now, std::size_t limit, Visitor && visitor)*

*Defined at [time/deadline_queue.cppm:145](../../../../time/deadline_queue.cppm#L145)*

 NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)

### schedule_after

*public Scheduled schedule_after(TimePoint now, Duration delay, Value value)*

*Defined at [time/deadline_queue.cppm:100](../../../../time/deadline_queue.cppm#L100)*



**brief** `value`, due `delay` after `now` (the owner's one reading of the clock). 

### schedule_at

*public Scheduled schedule_at(TimePoint due, Value value)*

*Defined at [time/deadline_queue.cppm:78](../../../../time/deadline_queue.cppm#L78)*



**brief** `value`, due at `due` (not before the epoch). 

### size

*public std::size_t size()*

*Defined at [time/deadline_queue.cppm:64](../../../../time/deadline_queue.cppm#L64)*

### visit_ordered

*public void visit_ordered(Visitor && visitor)*

*Defined at [time/deadline_queue.cppm:167](../../../../time/deadline_queue.cppm#L167)*

 NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)

## Enums

| enum class ScheduleResult |

--

| scheduled |
| full |
| invalid_time |
| tickets_exhausted |


*Defined at [time/deadline_queue.cppm:41](../../../../time/deadline_queue.cppm#L41)*



**brief** What scheduling did. 

