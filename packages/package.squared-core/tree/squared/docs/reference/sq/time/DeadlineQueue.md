# class DeadlineQueue

*Defined at [time/deadline_queue.cppm:26](../../../../time/deadline_queue.cppm#L26)*

## Records

Scheduled

Ticket



## Functions

### size

*public std::size_t size()*

*Defined at [time/deadline_queue.cppm:64](../../../../time/deadline_queue.cppm#L64)*

### capacity

*public static std::size_t capacity()*

*Defined at [time/deadline_queue.cppm:65](../../../../time/deadline_queue.cppm#L65)*

### empty

*public bool empty()*

*Defined at [time/deadline_queue.cppm:66](../../../../time/deadline_queue.cppm#L66)*

### next_due

*public std::optional<TimePoint> next_due()*

*Defined at [time/deadline_queue.cppm:69](../../../../time/deadline_queue.cppm#L69)*

### schedule_at

*public Scheduled schedule_at(TimePoint due, Value value)*

*Defined at [time/deadline_queue.cppm:78](../../../../time/deadline_queue.cppm#L78)*

### schedule_after

*public Scheduled schedule_after(TimePoint now, Duration delay, Value value)*

*Defined at [time/deadline_queue.cppm:100](../../../../time/deadline_queue.cppm#L100)*

### cancel

*public bool cancel(Ticket ticket)*

*Defined at [time/deadline_queue.cppm:110](../../../../time/deadline_queue.cppm#L110)*

### cancel

*public bool cancel(Ticket ticket, Visitor && visitor)*

*Defined at [time/deadline_queue.cppm:117](../../../../time/deadline_queue.cppm#L117)*

### poll_due

*public std::size_t poll_due(TimePoint now, std::size_t limit, Visitor && visitor)*

*Defined at [time/deadline_queue.cppm:145](../../../../time/deadline_queue.cppm#L145)*

### visit_ordered

*public void visit_ordered(Visitor && visitor)*

*Defined at [time/deadline_queue.cppm:167](../../../../time/deadline_queue.cppm#L167)*



## Enums

| enum class ScheduleResult |

--

| scheduled |
| full |
| invalid_time |
| tickets_exhausted |


*Defined at [time/deadline_queue.cppm:41](../../../../time/deadline_queue.cppm#L41)*



