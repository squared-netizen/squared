#include <squared/dev/dev_log.hpp>
#include <squared/dev/frame_allocations.hpp>

#include <pthread.h>

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

/** @brief Count one allocation if it comes from the counted thread. */
inline void count( std::size_t bytes ) noexcept
{
    if( g_armed.load( std::memory_order_relaxed ) &&
        pthread_equal( g_thread.load( std::memory_order_relaxed ), pthread_self() ) != 0 ) {
        ++g_frame.count;
        g_frame.bytes += bytes;
    }
}

/** @brief A short line built in place: integers through to_chars, no allocation. */
struct Report
{
        char text[160];
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
        void add( std::uint64_t value ) noexcept
        {
            char digits[20];
            const auto result = std::to_chars( digits, digits + sizeof( digits ), value );
            add( std::string_view( digits, static_cast<std::size_t>( result.ptr - digits ) ) );
        }
        void send() const noexcept { log_line( std::string_view( text, length ) ); }
};

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
            line.add( " allocated " );
            line.add( frame.count );
            line.add( " times, " );
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
        line.add( " times, " );
        line.add( g_window.bytes );
        line.add( " bytes; most in one frame: " );
        line.add( g_window_worst );
        line.send();
    }
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

void *allocate( std::size_t bytes )
{
    count( bytes );
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

void *allocate( std::size_t bytes, std::size_t alignment )
{
    count( bytes );
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

void *allocate_nothrow( std::size_t bytes ) noexcept
{
    count( bytes );
    return std::malloc( bytes == 0 ? 1 : bytes );
}

void *allocate_nothrow( std::size_t bytes, std::size_t alignment ) noexcept
{
    count( bytes );
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
    report( frame );
    return frame;
}

} // namespace sq::dev

// Replacement allocation functions ([new.delete]). Default visibility, because
// the build hides symbols by default and these have to reach the whole
// process: a shared libc++ resolves its own calls to operator new to these.

[[gnu::visibility( "default" )]] void *operator new( std::size_t bytes )
{
    return sq::dev::allocate( bytes );
}
[[gnu::visibility( "default" )]] void *operator new[]( std::size_t bytes )
{
    return sq::dev::allocate( bytes );
}
[[gnu::visibility( "default" )]] void *operator new( std::size_t bytes, std::align_val_t alignment )
{
    return sq::dev::allocate( bytes, static_cast<std::size_t>( alignment ) );
}
[[gnu::visibility( "default" )]] void *operator new[]( std::size_t bytes,
                                                       std::align_val_t alignment )
{
    return sq::dev::allocate( bytes, static_cast<std::size_t>( alignment ) );
}
[[gnu::visibility( "default" )]] void *operator new( std::size_t bytes,
                                                     const std::nothrow_t & ) noexcept
{
    return sq::dev::allocate_nothrow( bytes );
}
[[gnu::visibility( "default" )]] void *operator new[]( std::size_t bytes,
                                                       const std::nothrow_t & ) noexcept
{
    return sq::dev::allocate_nothrow( bytes );
}
[[gnu::visibility( "default" )]] void *operator new( std::size_t bytes, std::align_val_t alignment,
                                                     const std::nothrow_t & ) noexcept
{
    return sq::dev::allocate_nothrow( bytes, static_cast<std::size_t>( alignment ) );
}
[[gnu::visibility( "default" )]] void *
operator new[]( std::size_t bytes, std::align_val_t alignment, const std::nothrow_t & ) noexcept
{
    return sq::dev::allocate_nothrow( bytes, static_cast<std::size_t>( alignment ) );
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
