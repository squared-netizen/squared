# class Wake

*Defined at [messaging/src/wake.hpp:14](../../../../messaging/src/wake.hpp#L14)*



**brief** One thread sleeps on it, another wakes it: an `eventfd`, so the sleeper can wait on other file descriptors in the same `poll` (a pty, a child's pipes), and Android's looper can watch it.

 Signalled only when the sleeper says it is asleep (a flag beside it, the Telegraph's), so a busy thread costs no system call.



## Functions

### Wake

*public void Wake()*

### Wake

*public void Wake(const Wake & )*

*Defined at [messaging/src/wake.hpp:19](../../../../messaging/src/wake.hpp#L19)*

### operator=

*public Wake & operator=(const Wake & )*

*Defined at [messaging/src/wake.hpp:20](../../../../messaging/src/wake.hpp#L20)*

### Wake

*public void Wake(Wake && )*

*Defined at [messaging/src/wake.hpp:21](../../../../messaging/src/wake.hpp#L21)*

### operator=

*public Wake & operator=(Wake && )*

*Defined at [messaging/src/wake.hpp:22](../../../../messaging/src/wake.hpp#L22)*

### fd

*public int fd()*

*Defined at [messaging/src/wake.hpp:25](../../../../messaging/src/wake.hpp#L25)*



**brief** The descriptor to poll; -1 if it could not be made (then waits only time out). 

### ~Wake

*public void ~Wake()*

### signal

*public void signal()*



**brief** Wake the sleeper (or its next sleep). 

### clear

*public void clear()*



**brief** Forget any signal: after waking, before looking for work. 

### wait

*public void wait(int timeout_ms)*



**brief** Sleep until signalled or `timeout_ms` passes (-1: no timeout). Returns at once when already signalled.



