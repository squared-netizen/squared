# struct UnwindState

*Defined at [dev/crash_trace.hpp:102](../../../../../dev/crash_trace.hpp#L102)*

 The unwound program counters, collected before any is written so the handler's own frames can be dropped (crash_trace.hpp, frame 0).



## Members

public std::uintptr_t[32] pcs

public int count

public static const int capacity



