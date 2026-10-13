#include <squared/dev/dev_log.hpp>

#include <arpa/inet.h>
#include <dlfcn.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unwind.h>

#include <atomic>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace sq::dev {
namespace {

// Resident cost, all static storage and all present only in a build that links
// this module: the alternate stack dominates.
constexpr std::size_t kLineMax = 1000;
constexpr std::size_t kAltStackBytes = 16 * 1024;
constexpr int kSignals[] = {SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL, SIGTRAP};
constexpr std::size_t kSignalCount = sizeof(kSignals) / sizeof(kSignals[0]);

std::atomic<int> g_fd{-1};
sockaddr_in g_address{};
alignas(16) char g_alt_stack[kAltStackBytes];
struct sigaction g_previous[kSignalCount];
bool g_handlers_installed = false;

// A line under construction. Plain character appends, so the crash handler can
// use it: no allocation, no stdio, no locale.
struct Line {
    char text[kLineMax + 1];
    std::size_t length = 0;

    void add(const char* s) noexcept {
        while (*s != '\0' && length < kLineMax) text[length++] = *s++;
    }
    void add_decimal(std::uint64_t v) noexcept {
        char digits[20];
        std::size_t n = 0;
        do { digits[n++] = static_cast<char>('0' + v % 10); v /= 10; } while (v != 0);
        while (n != 0 && length < kLineMax) text[length++] = digits[--n];
    }
    void add_hex(std::uint64_t v) noexcept {
        add("0x");
        char digits[16];
        std::size_t n = 0;
        do { digits[n++] = "0123456789abcdef"[v & 0xF]; v >>= 4; } while (v != 0);
        while (n != 0 && length < kLineMax) text[length++] = digits[--n];
    }
};

void send_bytes(const char* data, std::size_t length) noexcept {
    const int fd = g_fd.load(std::memory_order_relaxed);
    if (fd < 0) return;
    // A full socket buffer or an absent listener drops the datagram; that is
    // the right outcome for a diagnostic.
    (void)::sendto(fd, data, length, 0,
                   reinterpret_cast<const sockaddr*>(&g_address), sizeof(g_address));
}

void send_line(const Line& line) noexcept { send_bytes(line.text, line.length); }

const char* signal_name(int sig) noexcept {
    switch (sig) {
        case SIGSEGV: return "SIGSEGV";
        case SIGABRT: return "SIGABRT";
        case SIGBUS:  return "SIGBUS";
        case SIGFPE:  return "SIGFPE";
        case SIGILL:  return "SIGILL";
        case SIGTRAP: return "SIGTRAP";
        default:      return "signal";
    }
}

const char* base_name(const char* path) noexcept {
    const char* slash = std::strrchr(path, '/');
    return slash != nullptr ? slash + 1 : path;
}

// One backtrace frame as "  #n pc 0xOFFSET module.so": the offset is relative
// to where the module is loaded, which is what `addr2line -e module.so` wants.
void send_frame(int index, std::uintptr_t pc) noexcept {
    Line line;
    line.add("  #");
    line.add_decimal(static_cast<std::uint64_t>(index));
    line.add(" pc ");
    Dl_info info{};
    if (pc != 0 && ::dladdr(reinterpret_cast<void*>(pc), &info) != 0 &&
        info.dli_fbase != nullptr && info.dli_fname != nullptr) {
        line.add_hex(pc - reinterpret_cast<std::uintptr_t>(info.dli_fbase));
        line.add(" ");
        line.add(base_name(info.dli_fname));
    } else {
        line.add_hex(pc);
    }
    send_line(line);
}

struct UnwindState {
    int index;
};

_Unwind_Reason_Code unwind_step(_Unwind_Context* context, void* argument) noexcept {
    auto* state = static_cast<UnwindState*>(argument);
    const std::uintptr_t pc = _Unwind_GetIP(context);
    if (pc == 0 || state->index >= 24) return _URC_END_OF_STACK;
    send_frame(state->index++, pc);
    return _URC_NO_REASON;
}

// The program counter that faulted, read from the signal's saved registers.
// This is the single most useful address in a crash, and it is read directly
// because unwinding out of a signal handler does not always reach it. The
// registers differ by architecture; an unknown architecture reports 0 and the
// unwound frames still follow.
std::uintptr_t fault_pc(void* ucontext) noexcept {
    if (ucontext == nullptr) return 0;
    const auto* uc = static_cast<const ucontext_t*>(ucontext);
#if defined(__aarch64__)
    return static_cast<std::uintptr_t>(uc->uc_mcontext.pc);
#elif defined(__x86_64__)
    return static_cast<std::uintptr_t>(uc->uc_mcontext.gregs[REG_RIP]);
#elif defined(__arm__)
    return static_cast<std::uintptr_t>(uc->uc_mcontext.arm_pc);
#elif defined(__i386__)
    return static_cast<std::uintptr_t>(uc->uc_mcontext.gregs[REG_EIP]);
#else
    return 0;
#endif
}

void on_fatal_signal(int sig, siginfo_t* info, void* ucontext) {
    Line head;
    head.add("CRASH ");
    head.add(signal_name(sig));
    head.add(" (");
    head.add_decimal(static_cast<std::uint64_t>(sig));
    head.add(") code ");
    head.add_decimal(static_cast<std::uint64_t>(info != nullptr ? info->si_code : 0));
    head.add(" addr ");
    head.add_hex(info != nullptr ? reinterpret_cast<std::uintptr_t>(info->si_addr) : 0);
    send_line(head);

    send_frame(0, fault_pc(ucontext));

    UnwindState state{1};
    _Unwind_Backtrace(unwind_step, &state);

    Line end;
    end.add("END");
    send_line(end);

    // Hand the signal back to the system so it still produces its own report
    // and the process still dies the way it would have.
    ::signal(sig, SIG_DFL);
    ::raise(sig);
}

void install_handlers() noexcept {
    stack_t stack{};
    stack.ss_sp = g_alt_stack;
    stack.ss_size = sizeof(g_alt_stack);
    stack.ss_flags = 0;
    (void)::sigaltstack(&stack, nullptr);

    struct sigaction action{};
    action.sa_sigaction = on_fatal_signal;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&action.sa_mask);
    for (std::size_t i = 0; i < kSignalCount; ++i) {
        (void)::sigaction(kSignals[i], &action, &g_previous[i]);
    }
    g_handlers_installed = true;
}

void restore_handlers() noexcept {
    if (!g_handlers_installed) return;
    for (std::size_t i = 0; i < kSignalCount; ++i) {
        (void)::sigaction(kSignals[i], &g_previous[i], nullptr);
    }
    g_handlers_installed = false;
}

}  // namespace

LogSinkStatus open_log_sink(const LogSinkOptions& options) noexcept {
    if (g_fd.load() >= 0) return LogSinkStatus::AlreadyOpen;

    const int fd = ::socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    if (fd < 0) return LogSinkStatus::SocketFailed;

    std::memset(&g_address, 0, sizeof(g_address));
    g_address.sin_family = AF_INET;
    g_address.sin_port = htons(options.port);
    g_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    g_fd.store(fd);
    if (options.catch_crashes) install_handlers();
    return LogSinkStatus::Opened;
}

void log_line(std::string_view line) noexcept {
    if (g_fd.load(std::memory_order_relaxed) < 0) return;
    send_bytes(line.data(), line.size() < kLineMax ? line.size() : kLineMax);
}

void close_log_sink() noexcept {
    restore_handlers();
    const int fd = g_fd.exchange(-1);
    if (fd >= 0) ::close(fd);
}

}  // namespace sq::dev
