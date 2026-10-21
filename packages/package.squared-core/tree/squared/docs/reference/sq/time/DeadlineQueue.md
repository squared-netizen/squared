# class DeadlineQueue

*Defined at [time/deadline_queue.hpp:35](../../../../time/deadline_queue.hpp#L35)*



**brief** A model's timers: values due at times, given back in order once their time has come. Over storage its owner gives (`Entry`s, as many as it may hold at once), so nothing is allocated after it is made.

 It reads no clock and runs no thread: its owner passes the time in. In a model, `next_due()` is the model's `wake_at()`, and `poll_due` runs in its `due()` (see `ModelRunner`). Entries due at the same time come back in the order they were scheduled. Values are not callbacks: what a due value means is its owner's business.



## Records

Entry

Scheduled

Ticket



## Functions

### DeadlineQueue<Value>

*public void DeadlineQueue<Value>(std::span<Entry> storage)*

*Defined at [time/deadline_queue.hpp:81](../../../../time/deadline_queue.hpp#L81)*



**brief** A queue over `storage`: it holds at most `storage.size()` entries at once. 

### DeadlineQueue<Value>

*public void DeadlineQueue<Value>(const DeadlineQueue<Value> & )*

*Defined at [time/deadline_queue.hpp:85](../../../../time/deadline_queue.hpp#L85)*

### operator=

*public DeadlineQueue<Value> & operator=(const DeadlineQueue<Value> & )*

*Defined at [time/deadline_queue.hpp:86](../../../../time/deadline_queue.hpp#L86)*

### DeadlineQueue<Value>

*public void DeadlineQueue<Value>(DeadlineQueue<Value> && )*

*Defined at [time/deadline_queue.hpp:87](../../../../time/deadline_queue.hpp#L87)*

### operator=

*public DeadlineQueue<Value> & operator=(DeadlineQueue<Value> && )*

*Defined at [time/deadline_queue.hpp:88](../../../../time/deadline_queue.hpp#L88)*

### ~DeadlineQueue<Value>

*public void ~DeadlineQueue<Value>()*

*Defined at [time/deadline_queue.hpp:89](../../../../time/deadline_queue.hpp#L89)*

### size

*public std::size_t size()*

*Defined at [time/deadline_queue.hpp:91](../../../../time/deadline_queue.hpp#L91)*

### capacity

*public std::size_t capacity()*

*Defined at [time/deadline_queue.hpp:92](../../../../time/deadline_queue.hpp#L92)*

### empty

*public bool empty()*

*Defined at [time/deadline_queue.hpp:93](../../../../time/deadline_queue.hpp#L93)*

### next_due

*public std::optional<TimePoint> next_due()*

*Defined at [time/deadline_queue.hpp:96](../../../../time/deadline_queue.hpp#L96)*



**brief** When the first entry is due; nothing when the queue is empty. 

### schedule_at

*public Scheduled schedule_at(TimePoint due, Value value)*

*Defined at [time/deadline_queue.hpp:105](../../../../time/deadline_queue.hpp#L105)*



**brief** `value`, due at `due` (not before the epoch). 

### schedule_after

*public Scheduled schedule_after(TimePoint now, Duration delay, Value value)*

*Defined at [time/deadline_queue.hpp:127](../../../../time/deadline_queue.hpp#L127)*



**brief** `value`, due `delay` after `now` (the owner's one reading of the clock). 

### cancel

*public bool cancel(Ticket ticket)*

*Defined at [time/deadline_queue.hpp:137](../../../../time/deadline_queue.hpp#L137)*



**brief** Cancel the entry `ticket` names, freeing its slot. False if it is not here (due already, or cancelled). 

### cancel

*public bool cancel(Ticket ticket, Visitor && visitor)*

*Defined at [time/deadline_queue.hpp:144](../../../../time/deadline_queue.hpp#L144)*



**brief** Cancel, handing the value to `visitor` first (to release what it holds). 

### poll_due

*public std::size_t poll_due(TimePoint now, std::size_t limit, Visitor && visitor)*

*Defined at [time/deadline_queue.hpp:172](../../../../time/deadline_queue.hpp#L172)*

 NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)

### visit_ordered

*public void visit_ordered(Visitor && visitor)*

*Defined at [time/deadline_queue.hpp:194](../../../../time/deadline_queue.hpp#L194)*

 NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)



## Enums

| enum class ScheduleResult |

--

| scheduled |
| full |
| invalid_time |
| tickets_exhausted |


*Defined at [time/deadline_queue.hpp:57](../../../../time/deadline_queue.hpp#L57)*



**brief** What scheduling did. 



