# struct Ticket

*Defined at [time/src/deadline_queue.hpp:34](../../../../../time/src/deadline_queue.hpp#L34)*



**brief** Opaque cancellation identity for one scheduled entry.

 A zero-valued ticket is empty and never matches a scheduled entry.



## Members

public int value



## Functions

### operator bool

*public bool operator bool()*

*Defined at [time/src/deadline_queue.hpp:43](../../../../../time/src/deadline_queue.hpp#L43)*



**brief** Check whether this ticket references a scheduled entry.

**return** true when the ticket is non-zero and usable.



