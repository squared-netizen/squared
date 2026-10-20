# namespace detail

## Records

* [UnwindState](UnwindState.md)


## Functions

### base_name

*const char * base_name(const char * path)*

*Defined at [dev/crash_trace.hpp:73](../../../../../dev/crash_trace.hpp#L73)*

### fault_pc

*int fault_pc(void * ucontext)*

*Defined at [dev/crash_trace.hpp:139](../../../../../dev/crash_trace.hpp#L139)*

 The program counter that faulted, from the signal's saved registers; 0 on an architecture this does not know (the unwound frames still follow).

### first_caller

*int first_caller(const UnwindState & state, int fault)*

*Defined at [dev/crash_trace.hpp:125](../../../../../dev/crash_trace.hpp#L125)*

 Where the frames below the fault start: just past frame 0's program counter when the unwinder reached it, else at the first unwound frame.

### signal_name

*const char * signal_name(int signal)*

*Defined at [dev/crash_trace.hpp:53](../../../../../dev/crash_trace.hpp#L53)*

### unwind_step

*_Unwind_Reason_Code unwind_step(_Unwind_Context * unwind, void * argument)*

*Defined at [dev/crash_trace.hpp:112](../../../../../dev/crash_trace.hpp#L112)*

 Not noexcept, on purpose: a noexcept callback stops _Unwind_Backtrace at its first frame (found 2026-10-19 moving this code here; with it the trace lost every frame but #0, with g++ and clang alike).

### write_frame

*void write_frame(int index, int pc, CrashSink sink, void * context)*

*Defined at [dev/crash_trace.hpp:80](../../../../../dev/crash_trace.hpp#L80)*

 One frame: "  #n pc 0xOFFSET module.so", or the bare address when no module claims it.

