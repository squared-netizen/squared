#pragma once

#include <squared/dev/src/crash_line.hpp>

#include <dlfcn.h>
#include <unwind.h>

#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace sq::dev
{

/**
 * @brief Where a crash trace's lines go: called once a line, without the
 * newline, from inside a signal handler. It must be async-signal-safe: a
 * `write()` to a descriptor, a `sendto()`.
 */
using CrashSink = void ( * )( void *context, const char *text, std::size_t length ) noexcept;

/**
 * @brief Write the trace of a fatal signal to `sink`, a line at a time:
 *
 * ```text
 * CRASH SIGSEGV (11) code 1 addr 0x0
 *   #0 pc 0x4a5f0 libdemo.so
 *   #1 pc 0x51c2c libdemo.so
 *   ...
 * END
 * ```
 *
 * Frame 0 is the faulting program counter, read from the saved registers
 * (unwinding out of a handler does not always reach it); up to 23 unwound
 * frames follow. The unwinder starts inside the handler, so its first frames
 * are the handler, any signal chain and the kernel's return trampoline, and
 * then frame 0 again; those are dropped when frame 0 is found among them, and
 * kept when it isn't. Each frame is an offset from its library's load address,
 * which is what `addr2line -e lib.so` takes. Header-only, so the in-app
 * Console's crash file uses it without linking this module.
 *
 * Call it only from a signal handler, for a process that is about to die: it
 * allocates nothing and takes no lock, but `dladdr` and the unwinder are not
 * on the async-signal-safe list. A diagnostic that might fail beats none.
 */
void write_crash_trace( int signal, const siginfo_t *info, void *ucontext, CrashSink sink,
                        void *context ) noexcept;

namespace detail
{

[[nodiscard]] inline const char *signal_name( int signal ) noexcept
{
    switch( signal ) {
        case SIGSEGV:
            return "SIGSEGV";
        case SIGABRT:
            return "SIGABRT";
        case SIGBUS:
            return "SIGBUS";
        case SIGFPE:
            return "SIGFPE";
        case SIGILL:
            return "SIGILL";
        case SIGTRAP:
            return "SIGTRAP";
        default:
            return "signal";
    }
}

[[nodiscard]] inline const char *base_name( const char *path ) noexcept
{
    const char *slash = std::strrchr( path, '/' );
    return slash != nullptr ? slash + 1 : path;
}

/// One frame: "  #n pc 0xOFFSET module.so", or the bare address when no module claims it.
inline void write_frame( int index, std::uintptr_t pc, CrashSink sink, void *context ) noexcept
{
    CrashLine line;
    line.add( "  #" );
    line.add_decimal( static_cast<std::uint64_t>( index ) );
    line.add( " pc " );
    Dl_info info{};
    // A program counter is an address held as an integer.
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    if( pc != 0 && ::dladdr( reinterpret_cast<void *>( pc ), &info ) != 0 &&
        info.dli_fbase != nullptr && info.dli_fname != nullptr ) {
        line.add_hex( pc - reinterpret_cast<std::uintptr_t>( info.dli_fbase ) );
        line.add( " " );
        line.add( base_name( info.dli_fname ) );
    } else {
        line.add_hex( pc );
    }
    sink( context, line.text, line.length );
}

/// The unwound program counters, collected before any is written so the
/// handler's own frames can be dropped (crash_trace.hpp, frame 0).
struct UnwindState
{
        static constexpr int capacity = 32;
        std::uintptr_t pcs[capacity];
        int count;
};

// Not noexcept, on purpose: a noexcept callback stops _Unwind_Backtrace at
// its first frame (found 2026-10-19 moving this code here; with it the trace
// lost every frame but #0, with g++ and clang alike).
inline _Unwind_Reason_Code unwind_step( _Unwind_Context *unwind, void *argument )
{
    auto *state = static_cast<UnwindState *>( argument );
    const std::uintptr_t pc = _Unwind_GetIP( unwind );
    if( pc == 0 || state->count >= UnwindState::capacity ) {
        return _URC_END_OF_STACK;
    }
    state->pcs[state->count++] = pc;
    return _URC_NO_REASON;
}

/// Where the frames below the fault start: just past frame 0's program
/// counter when the unwinder reached it, else at the first unwound frame.
[[nodiscard]] inline int first_caller( const UnwindState &state, std::uintptr_t fault ) noexcept
{
    if( fault != 0 ) {
        for( int i = 0; i < state.count; ++i ) {
            if( state.pcs[i] == fault ) {
                return i + 1;
            }
        }
    }
    return 0;
}

/// The program counter that faulted, from the signal's saved registers; 0 on
/// an architecture this does not know (the unwound frames still follow).
[[nodiscard]] inline std::uintptr_t fault_pc( void *ucontext ) noexcept
{
    if( ucontext == nullptr ) {
        return 0;
    }
    const auto *uc = static_cast<const ucontext_t *>( ucontext );
#if defined( __aarch64__ )
    return static_cast<std::uintptr_t>( uc->uc_mcontext.pc );
#elif defined( __x86_64__ )
    return static_cast<std::uintptr_t>( uc->uc_mcontext.gregs[REG_RIP] );
#elif defined( __arm__ )
    return static_cast<std::uintptr_t>( uc->uc_mcontext.arm_pc );
#elif defined( __i386__ )
    return static_cast<std::uintptr_t>( uc->uc_mcontext.gregs[REG_EIP] );
#else
    return 0;
#endif
}

} // namespace detail

inline void write_crash_trace( int signal, const siginfo_t *info, void *ucontext, CrashSink sink,
                               void *context ) noexcept
{
    CrashLine head;
    head.add( "CRASH " );
    head.add( detail::signal_name( signal ) );
    head.add( " (" );
    head.add_decimal( static_cast<std::uint64_t>( signal ) );
    head.add( ") code " );
    head.add_decimal( static_cast<std::uint64_t>( info != nullptr ? info->si_code : 0 ) );
    head.add( " addr " );
    head.add_hex( info != nullptr ? reinterpret_cast<std::uintptr_t>( info->si_addr ) : 0 );
    sink( context, head.text, head.length );

    const std::uintptr_t fault = detail::fault_pc( ucontext );
    detail::write_frame( 0, fault, sink, context );

    detail::UnwindState state{ .pcs = {}, .count = 0 };
    _Unwind_Backtrace( detail::unwind_step, &state );
    int index = 1;
    for( int i = detail::first_caller( state, fault ); i < state.count && index < 24; ++i ) {
        detail::write_frame( index++, state.pcs[i], sink, context );
    }

    sink( context, "END", 3 );
}

} // namespace sq::dev
