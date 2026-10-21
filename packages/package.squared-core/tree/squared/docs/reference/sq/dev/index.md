# namespace dev

## Namespaces

* [detail](detail/index.md)


## Records

* [CrashLine](CrashLine.md)
* [FrameAllocations](FrameAllocations.md)
* [LogSinkOptions](LogSinkOptions.md)


## Functions

### close_log_sink

*void close_log_sink()*

 Close the sink and restore the signal handlers it replaced.

### frame_begin

*void frame_begin()*



**brief** Start counting heap allocations made by the calling thread.

 Linking this module replaces the global operator new and operator delete with versions that count, then use malloc and free. Only allocations on the thread that called frame_begin(), and only until frame_end(), are counted; a loader or audio thread allocating meanwhile is not. Calling it twice without frame_end() restarts the count.

### frame_end

*FrameAllocations frame_end()*



**brief** Stop counting and report what the frame allocated.

 When a log sink is open (open_log_sink()), allocating frames are reported through it: the first one at once, then at most one summary line every 120 frames, followed by the window's busiest call sites. Each call site is named once, the first time it allocates, as the four innermost frames above operator new, in the `library 0xoffset` form addr2line takes. A frame that allocated nothing is never reported.



**return** This frame's count; zero when frame_begin() was not called on this thread.

### log_line

*void log_line(std::string_view line, LogLevel level)*



**brief** Send one line to the sink, if one is open, and to the listener, if one is set and this is its thread.

**line** Text without a trailing newline; longer than 1000 bytes is cut for the sink (the listener gets it whole).

**level** Passed to the listener; the sink sends the text alone. Never blocks and never fails visibly: a missing `nc` drops the line.

### open_log_sink

*LogSinkStatus open_log_sink(const LogSinkOptions & options)*



**brief** Send log lines, and crash reports, to a UDP port on this device.

 A development aid for working on a phone with no computer attached. Run a listener in the same device's terminal, for example `nc -klu 5555`, and each line arrives as it is written. The sink uses the loopback address only: no network, no Wi-Fi, and no data leaves the device.

 Nothing here allocates after open_log_sink() returns, and nothing is buffered, so the last line written before a crash is still delivered.



**options** Port and crash-handler switch.

**return** Opened on success; AlreadyOpen if called twice; SocketFailed if the OS refused the socket.

### report_allocations

*void report_allocations()*



**brief** Report every frame counted so far through log_line(): one summary line (frames counted, how many allocated, allocations, bytes, the most in one frame); what operator new holds now on every thread (KB and blocks, by the allocator's measure, and the peak); then every site found, as the `site` lines name them.

 What the Console's `alloc` command runs. Call it on the thread that counts frames. Allocates nothing.

### set_log_listener

*void set_log_listener(LogListener listener, void * context)*



**brief** Hand every later log_line() made on the calling thread to `listener` as well, with `context`; nullptr stops it. One listener at a time: setting one replaces the last. Lines written on other threads reach the sink only, so the listener never needs a lock. The crash handler's lines never reach it.

 The in-app Console sets one, so the dev log shows on screen.

### write_crash_trace

*void write_crash_trace(int signal, const siginfo_t * info, void * ucontext, CrashSink sink, void * context)*

*Defined at [dev/crash_trace.hpp:160](../../../../dev/crash_trace.hpp#L160)*



**brief** Write the trace of a fatal signal to `sink`, a line at a time:

 ```text CRASH SIGSEGV (11) code 1 addr 0x0   #0 pc 0x4a5f0 libdemo.so   #1 pc 0x51c2c libdemo.so   ... END ```

 Frame 0 is the faulting program counter, read from the saved registers (unwinding out of a handler does not always reach it); up to 23 unwound frames follow. The unwinder starts inside the handler, so its first frames are the handler, any signal chain and the kernel's return trampoline, and then frame 0 again; those are dropped when frame 0 is found among them, and kept when it isn't. Each frame is an offset from its library's load address, which is what `addr2line -e lib.so` takes. Header-only, so the in-app Console's crash file uses it without linking this module.

 Call it only from a signal handler, for a process that is about to die: it allocates nothing and takes no lock, but `dladdr` and the unwinder are not on the async-signal-safe list. A diagnostic that might fail beats none.

## Enums

| enum class LogLevel |

--

| Info |
| Warning |
| Error |


*Defined at [dev/dev_log.hpp:18](../../../../dev/dev_log.hpp#L18)*

 How serious a line is. The UDP sink sends the text alone; a listener gets both.

| enum class LogSinkStatus |

--

| Opened |
| AlreadyOpen |
| SocketFailed |


*Defined at [dev/dev_log.hpp:10](../../../../dev/dev_log.hpp#L10)*

 Outcome of open_log_sink().

