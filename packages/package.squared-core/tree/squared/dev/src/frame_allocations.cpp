#include <squared/dev/dev_log.hpp>
#include <squared/dev/frame_allocations.hpp>

#include <dlfcn.h>
#include <pthread.h>
#include <unwind.h>

#include <array>
#include <atomic>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <string_view>

namespace sq::dev
{
namespace
{

// Which thread is being counted, and whether counting is on. Every allocation
// in the process reads these two, so they are atomics; only the counted thread
// writes the tallies below, so those are plain.
std::atomic<bool> g_armed{ false };
std::atomic<pthread_t> g_thread{};

FrameAllocations g_frame{};

// Reporting state, touched only by the thread that calls frame_end().
constexpr std::uint64_t kSummaryFrames = 120;
std::uint64_t g_frames = 0;
bool g_reported_first = false;
std::uint64_t g_window_frames = 0;
std::uint64_t g_window_allocating = 0;
FrameAllocations g_window{};
std::uint64_t g_window_worst = 0;

// Call sites. An allocation's site is the four innermost frames of the stack
// that called operator new, starting at its direct caller: enough to name
// both the container that grew and the code that grew it. Sites live in a
// fixed table, because recording them must not allocate either.
constexpr std::size_t kSiteFrames = 4;
constexpr std::size_t kSites = 64;
constexpr std::size_t kSkipLimit = 16;

struct Site
{
        std::array<std::uintptr_t, kSiteFrames> pcs{};
        std::uint64_t window_count = 0; ///< Allocations in the current window.
        std::uint64_t first_frame = 0;  ///< The frame it was first seen in.
        bool used = false;
        bool named = false; ///< Its "new site" line has been sent.
};

std::array<Site, kSites> g_sites{};
std::uint64_t g_unnamed = 0; ///< Allocations from sites that did not fit.
bool g_reported_full = false;
bool g_capturing = false; ///< Guards against an unwinder that allocates.

struct Capture
{
        std::uintptr_t caller;
        std::array<std::uintptr_t, kSiteFrames> pcs{};
        std::size_t found = 0;
        std::size_t skipped = 0;
};

// Walks outwards from here, skips this file's own frames (they end where
// operator new returns to its caller), then keeps the next kSiteFrames.
_Unwind_Reason_Code capture_step( _Unwind_Context *context, void *argument ) noexcept
{
    auto *capture = static_cast<Capture *>( argument );
    const std::uintptr_t pc = _Unwind_GetIP( context );
    if( capture->found == 0 && pc != capture->caller ) {
        return ++capture->skipped < kSkipLimit ? _URC_NO_REASON : _URC_END_OF_STACK;
    }
    capture->pcs[capture->found++] = pc;
    return capture->found < kSiteFrames ? _URC_NO_REASON : _URC_END_OF_STACK;
}

void record_site( std::uintptr_t caller ) noexcept
{
    if( g_capturing ) {
        return;
    }
    g_capturing = true;
    Capture capture{ caller };
    _Unwind_Backtrace( capture_step, &capture );
    if( capture.found == 0 ) {
        // The unwinder could not see the caller's frame: keep what is known.
        capture.pcs[0] = caller;
    }
    // Linear search: a frame allocates a handful of times, from a handful of
    // sites, and only in a development build.
    for( Site &site : g_sites ) {
        if( site.used && site.pcs == capture.pcs ) {
            ++site.window_count;
            g_capturing = false;
            return;
        }
        if( !site.used ) {
            site.used = true;
            site.pcs = capture.pcs;
            site.window_count = 1;
            site.first_frame = g_frames + 1;
            g_capturing = false;
            return;
        }
    }
    ++g_unnamed;
    g_capturing = false;
}

/** @brief Count one allocation, and its site, if it comes from the counted thread. */
inline void count( std::size_t bytes, std::uintptr_t caller ) noexcept
{
    if( g_armed.load( std::memory_order_relaxed ) &&
        pthread_equal( g_thread.load( std::memory_order_relaxed ), pthread_self() ) != 0 ) {
        ++g_frame.count;
        g_frame.bytes += bytes;
        record_site( caller );
    }
}

// The caller of the function this is expanded in, as an integer. A macro and
// not a function: __builtin_return_address( 0 ) names the frame it is written
// in, so it has to be written in each operator new. #undef at the end of file.
#define SQ_DEV_CALLER reinterpret_cast<std::uintptr_t>( __builtin_return_address( 0 ) )

/** @brief A short line built in place: integers through to_chars, no allocation. */
struct Report
{
        char text[256];
        std::size_t length = 0;

        void add( std::string_view part ) noexcept
        {
            for( const char c : part ) {
                if( length == sizeof( text ) ) {
                    return;
                }
                text[length++] = c;
            }
        }
        void add( std::uint64_t value, int base = 10 ) noexcept
        {
            char digits[24];
            const auto result = std::to_chars( digits, digits + sizeof( digits ), value, base );
            add( std::string_view( digits, static_cast<std::size_t>( result.ptr - digits ) ) );
        }
        /// A program counter as `library 0xoffset`, the form addr2line takes.
        void add_frame( std::uintptr_t pc ) noexcept
        {
            Dl_info info{};
            // NOLINTNEXTLINE(performance-no-int-to-ptr): a program counter is an address held as an integer
            if( ::dladdr( reinterpret_cast<const void *>( pc ), &info ) == 0 ||
                info.dli_fbase == nullptr || info.dli_fname == nullptr ) {
                add( "? 0x" );
                add( pc, 16 );
                return;
            }
            const std::string_view path( info.dli_fname );
            const std::size_t slash = path.rfind( '/' );
            add( slash == std::string_view::npos ? path : path.substr( slash + 1 ) );
            add( " 0x" );
            add( pc - reinterpret_cast<std::uintptr_t>( info.dli_fbase ), 16 );
        }
        void send() const noexcept { log_line( std::string_view( text, length ) ); }
};

/** @brief "new site" lines for sites first seen since the last call. */
void report_new_sites() noexcept
{
    for( std::size_t index = 0; index < kSites; ++index ) {
        Site &site = g_sites[index];
        if( !site.used || site.named ) {
            continue;
        }
        site.named = true;
        Report line;
        line.add( "alloc: site " );
        line.add( index + 1 );
        line.add( " (frame " );
        line.add( site.first_frame );
        line.add( "):" );
        for( std::size_t frame = 0; frame < kSiteFrames && site.pcs[frame] != 0; ++frame ) {
            line.add( frame == 0 ? " " : " < " );
            line.add_frame( site.pcs[frame] );
        }
        line.send();
    }
    if( g_unnamed != 0 && !g_reported_full ) {
        g_reported_full = true;
        Report line;
        line.add( "alloc: the site table is full (" );
        line.add( kSites );
        line.add( "); later sites are counted but not named" );
        line.send();
    }
}

/** @brief The window's busiest sites, most allocations first: "site:count". */
void report_window_sites() noexcept
{
    Report line;
    line.add( "alloc: by site:" );
    std::uint64_t ceiling = UINT64_MAX;
    // At most eight entries, each found by a pass for the next-largest count:
    // no sort and no copy of the table.
    for( int shown = 0; shown < 8; ++shown ) {
        std::size_t best = kSites;
        for( std::size_t index = 0; index < kSites; ++index ) {
            const Site &site = g_sites[index];
            if( site.used && site.window_count != 0 && site.window_count < ceiling &&
                ( best == kSites || site.window_count > g_sites[best].window_count ) ) {
                best = index;
            }
        }
        if( best == kSites ) {
            break;
        }
        ceiling = g_sites[best].window_count;
        // Every site with this count, in table order.
        for( std::size_t index = 0; index < kSites; ++index ) {
            if( g_sites[index].used && g_sites[index].window_count == ceiling ) {
                line.add( " " );
                line.add( index + 1 );
                line.add( ":" );
                line.add( ceiling );
            }
        }
    }
    if( g_unnamed != 0 ) {
        line.add( " unnamed:" );
        line.add( g_unnamed );
    }
    line.send();
}

void report( const FrameAllocations &frame ) noexcept
{
    ++g_window_frames;
    if( frame.count != 0 ) {
        ++g_window_allocating;
        g_window.count += frame.count;
        g_window.bytes += frame.bytes;
        if( frame.count > g_window_worst ) {
            g_window_worst = frame.count;
        }
        if( !g_reported_first ) {
            g_reported_first = true;
            Report line;
            line.add( "alloc: frame " );
            line.add( g_frames );
            line.add( ": " );
            line.add( frame.count );
            line.add( " allocations, " );
            line.add( frame.bytes );
            line.add( " bytes (the first frame that allocated)" );
            line.send();
        }
    }
    if( g_window_frames < kSummaryFrames ) {
        return;
    }
    if( g_window_allocating != 0 ) {
        Report line;
        line.add( "alloc: " );
        line.add( g_window_allocating );
        line.add( " of the last " );
        line.add( g_window_frames );
        line.add( " frames allocated: " );
        line.add( g_window.count );
        line.add( " allocations, " );
        line.add( g_window.bytes );
        line.add( " bytes; most in one frame: " );
        line.add( g_window_worst );
        line.send();
        report_window_sites();
    }
    for( Site &site : g_sites ) {
        site.window_count = 0;
    }
    g_unnamed = 0;
    g_window_frames = 0;
    g_window_allocating = 0;
    g_window = {};
    g_window_worst = 0;
}

/** @brief What operator new does when malloc fails: the new handler, then bad_alloc. */
[[noreturn]] void out_of_memory()
{
#if defined( __cpp_exceptions )
    throw std::bad_alloc();
#else
    std::abort();
#endif
}

// The replacements below allocate with malloc and posix_memalign and release
// with free: the one place in squared that does, because a replacement
// operator new is where the heap is reached at all. Rules and checks that
// forbid it elsewhere are suppressed here, and only here.
// NOLINTBEGIN(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)

void *allocate( std::size_t bytes, std::uintptr_t caller )
{
    count( bytes, caller );
    const std::size_t asked = bytes == 0 ? 1 : bytes;
    for( ;; ) {
        if( void *memory = std::malloc( asked ) ) {
            return memory;
        }
        const std::new_handler handler = std::get_new_handler();
        if( handler == nullptr ) {
            out_of_memory();
        }
        handler();
    }
}

void *allocate( std::size_t bytes, std::size_t alignment, std::uintptr_t caller )
{
    count( bytes, caller );
    const std::size_t asked = bytes == 0 ? 1 : bytes;
    const std::size_t align = alignment < sizeof( void * ) ? sizeof( void * ) : alignment;
    for( ;; ) {
        void *memory = nullptr;
        if( ::posix_memalign( &memory, align, asked ) == 0 ) {
            return memory;
        }
        const std::new_handler handler = std::get_new_handler();
        if( handler == nullptr ) {
            out_of_memory();
        }
        handler();
    }
}

void *allocate_nothrow( std::size_t bytes, std::uintptr_t caller ) noexcept
{
    count( bytes, caller );
    return std::malloc( bytes == 0 ? 1 : bytes );
}

void *allocate_nothrow( std::size_t bytes, std::size_t alignment, std::uintptr_t caller ) noexcept
{
    count( bytes, caller );
    void *memory = nullptr;
    const std::size_t align = alignment < sizeof( void * ) ? sizeof( void * ) : alignment;
    return ::posix_memalign( &memory, align, bytes == 0 ? 1 : bytes ) == 0 ? memory : nullptr;
}

void release( void *memory ) noexcept
{
    std::free( memory );
}

// NOLINTEND(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)

} // namespace

void frame_begin() noexcept
{
    g_frame = {};
    g_thread.store( pthread_self(), std::memory_order_relaxed );
    g_armed.store( true, std::memory_order_release );
}

FrameAllocations frame_end() noexcept
{
    if( !g_armed.load( std::memory_order_acquire ) ||
        pthread_equal( g_thread.load( std::memory_order_relaxed ), pthread_self() ) == 0 ) {
        return {};
    }
    g_armed.store( false, std::memory_order_release );
    const FrameAllocations frame = g_frame;
    ++g_frames;
    report_new_sites();
    report( frame );
    return frame;
}

} // namespace sq::dev

// Replacement allocation functions ([new.delete]). Default visibility, because
// the build hides symbols by default and these have to reach the whole
// process: a shared libc++ resolves its own calls to operator new to these.

[[gnu::visibility( "default" )]] void *operator new( std::size_t bytes )
{
    return sq::dev::allocate( bytes, SQ_DEV_CALLER );
}
[[gnu::visibility( "default" )]] void *operator new[]( std::size_t bytes )
{
    return sq::dev::allocate( bytes, SQ_DEV_CALLER );
}
[[gnu::visibility( "default" )]] void *operator new( std::size_t bytes, std::align_val_t alignment )
{
    return sq::dev::allocate( bytes, static_cast<std::size_t>( alignment ), SQ_DEV_CALLER );
}
[[gnu::visibility( "default" )]] void *operator new[]( std::size_t bytes,
                                                       std::align_val_t alignment )
{
    return sq::dev::allocate( bytes, static_cast<std::size_t>( alignment ), SQ_DEV_CALLER );
}
[[gnu::visibility( "default" )]] void *operator new( std::size_t bytes,
                                                     const std::nothrow_t & ) noexcept
{
    return sq::dev::allocate_nothrow( bytes, SQ_DEV_CALLER );
}
[[gnu::visibility( "default" )]] void *operator new[]( std::size_t bytes,
                                                       const std::nothrow_t & ) noexcept
{
    return sq::dev::allocate_nothrow( bytes, SQ_DEV_CALLER );
}
[[gnu::visibility( "default" )]] void *operator new( std::size_t bytes, std::align_val_t alignment,
                                                     const std::nothrow_t & ) noexcept
{
    return sq::dev::allocate_nothrow( bytes, static_cast<std::size_t>( alignment ), SQ_DEV_CALLER );
}
[[gnu::visibility( "default" )]] void *
operator new[]( std::size_t bytes, std::align_val_t alignment, const std::nothrow_t & ) noexcept
{
    return sq::dev::allocate_nothrow( bytes, static_cast<std::size_t>( alignment ), SQ_DEV_CALLER );
}

[[gnu::visibility( "default" )]] void operator delete( void *memory ) noexcept
{
    sq::dev::release( memory );
}
[[gnu::visibility( "default" )]] void operator delete[]( void *memory ) noexcept
{
    sq::dev::release( memory );
}
[[gnu::visibility( "default" )]] void operator delete( void *memory, std::size_t ) noexcept
{
    sq::dev::release( memory );
}
[[gnu::visibility( "default" )]] void operator delete[]( void *memory, std::size_t ) noexcept
{
    sq::dev::release( memory );
}
[[gnu::visibility( "default" )]] void operator delete( void *memory, std::align_val_t ) noexcept
{
    sq::dev::release( memory );
}
[[gnu::visibility( "default" )]] void operator delete[]( void *memory, std::align_val_t ) noexcept
{
    sq::dev::release( memory );
}
[[gnu::visibility( "default" )]] void operator delete( void *memory, std::size_t,
                                                       std::align_val_t ) noexcept
{
    sq::dev::release( memory );
}
[[gnu::visibility( "default" )]] void operator delete[]( void *memory, std::size_t,
                                                         std::align_val_t ) noexcept
{
    sq::dev::release( memory );
}
[[gnu::visibility( "default" )]] void operator delete( void *memory,
                                                       const std::nothrow_t & ) noexcept
{
    sq::dev::release( memory );
}
[[gnu::visibility( "default" )]] void operator delete[]( void *memory,
                                                         const std::nothrow_t & ) noexcept
{
    sq::dev::release( memory );
}
[[gnu::visibility( "default" )]] void operator delete( void *memory, std::align_val_t,
                                                       const std::nothrow_t & ) noexcept
{
    sq::dev::release( memory );
}
[[gnu::visibility( "default" )]] void operator delete[]( void *memory, std::align_val_t,
                                                         const std::nothrow_t & ) noexcept
{
    sq::dev::release( memory );
}

#undef SQ_DEV_CALLER
