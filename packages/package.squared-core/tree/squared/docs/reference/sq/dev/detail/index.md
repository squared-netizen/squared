# namespace detail

## Records

* [UnwindState](UnwindState.md)


## Functions

### base_name

*const char * base_name(const char * path)*

*Defined at [dev/crash_trace.cppm:72](../../../../../dev/crash_trace.cppm#L72)*

### fault_pc

*std::uintptr_t fault_pc(void * ucontext)*

*Defined at [dev/crash_trace.cppm:138](../../../../../dev/crash_trace.cppm#L138)*

 The program counter that faulted, from the signal's saved registers; 0 on an architecture this does not know (the unwound frames still follow).

### first_caller

*int first_caller(const UnwindState & state, std::uintptr_t fault)*

*Defined at [dev/crash_trace.cppm:124](../../../../../dev/crash_trace.cppm#L124)*

 Where the frames below the fault start: just past frame 0's program counter when the unwinder reached it, else at the first unwound frame.

### signal_name

*const char * signal_name(int signal)*

*Defined at [dev/crash_trace.cppm:52](../../../../../dev/crash_trace.cppm#L52)*

### unwind_step

*_Unwind_Reason_Code unwind_step(_Unwind_Context * unwind, void * argument)*

*Defined at [dev/crash_trace.cppm:111](../../../../../dev/crash_trace.cppm#L111)*

 Not noexcept, on purpose: a noexcept callback stops _Unwind_Backtrace at its first frame (found 2026-10-19 moving this code here; with it the trace lost every frame but #0, with g++ and clang alike).

### write_frame

*void write_frame(int index, std::uintptr_t pc, CrashSink sink, void * context)*

*Defined at [dev/crash_trace.cppm:79](../../../../../dev/crash_trace.cppm#L79)*

 One frame: "  #n pc 0xOFFSET module.so", or the bare address when no module claims it.

