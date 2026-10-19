// The built-in commands that read the running system. Linux interfaces, so
// the same code serves Android, Termux and desktop Linux; the C library is
// what differs (bionic has system properties and mallinfo, glibc has
// mallinfo2), so that is what the two #if blocks test, not the platform:
// Termux is the posix platform on bionic.

#include <squared/console/system_commands.hpp>

#include <dlfcn.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <unistd.h>

#if defined( __BIONIC__ )
#include <sys/system_properties.h>
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <span>
#include <string_view>

namespace sq::console
{

namespace
{

/// A whole small file (a /proc one), read into `buffer`; empty if unreadable.
std::string_view read_file( const char *path, std::span<char> buffer ) noexcept
{
    const int fd = ::open( path, O_RDONLY | O_CLOEXEC );
    if( fd < 0 ) {
        return {};
    }
    std::size_t used = 0;
    while( used < buffer.size() ) {
        const ssize_t got = ::read( fd, buffer.data() + used, buffer.size() - used );
        if( got <= 0 ) {
            break;
        }
        used += static_cast<std::size_t>( got );
    }
    ::close( fd );
    return { buffer.data(), used };
}

/// The number after `name` on its line of a /proc "Name:   1234 kB" file; -1 when absent.
long long field( std::string_view text, std::string_view name ) noexcept
{
    std::size_t at = 0;
    while( at < text.size() ) {
        const std::size_t end = std::min( text.find( '\n', at ), text.size() );
        const std::string_view line = text.substr( at, end - at );
        if( line.starts_with( name ) ) {
            std::size_t digits = name.size();
            while( digits < line.size() && ( line[digits] == ' ' || line[digits] == '\t' ) ) {
                ++digits;
            }
            long long value = -1;
            const auto result =
                std::from_chars( line.data() + digits, line.data() + line.size(), value );
            return result.ec == std::errc{} ? value : -1;
        }
        at = end + 1;
    }
    return -1;
}

/// Kilobytes as "123.4 MB", or "?" when unknown, into `out`. Integers only.
const char *megabytes( long long kilobytes, std::span<char, 24> out ) noexcept
{
    if( kilobytes < 0 ) {
        return "?";
    }
    const long long tenths = ( kilobytes * 10 + 512 ) / 1024;
    std::snprintf( out.data(), out.size(), "%lld.%lld MB", tenths / 10, tenths % 10 );
    return out.data();
}

void mem( Console &console, const Args &, void * ) noexcept
{
    std::array<char, 4096> status_buffer{};
    std::array<char, 4096> meminfo_buffer{};
    const std::string_view status = read_file( "/proc/self/status", status_buffer );
    const std::string_view meminfo = read_file( "/proc/meminfo", meminfo_buffer );
    std::array<char, 24> resident{};
    std::array<char, 24> peak{};
    std::array<char, 24> available{};
    std::array<char, 24> total{};
    console.printf( Level::info, "memory: resident %s (peak %s); the system has %s of %s available",
                    megabytes( field( status, "VmRSS:" ), resident ),
                    megabytes( field( status, "VmHWM:" ), peak ),
                    megabytes( field( meminfo, "MemAvailable:" ), available ),
                    megabytes( field( meminfo, "MemTotal:" ), total ) );
}

void heap( Console &console, const Args &, void * ) noexcept
{
#if defined( __GLIBC__ ) && ( __GLIBC__ > 2 || ( __GLIBC__ == 2 && __GLIBC_MINOR__ >= 33 ) )
    const struct mallinfo2 info = ::mallinfo2();
#else
    const struct mallinfo info = ::mallinfo();
#endif
    std::array<char, 24> used{};
    std::array<char, 24> free{};
    std::array<char, 24> mapped{};
    console.printf( Level::info, "heap: %s in use, %s free in the arena, %s mapped",
                    megabytes( static_cast<long long>( info.uordblks / 1024 ), used ),
                    megabytes( static_cast<long long>( info.fordblks / 1024 ), free ),
                    megabytes( static_cast<long long>( info.hblkhd / 1024 ), mapped ) );
}

void threads( Console &console, const Args &, void * ) noexcept
{
    std::array<char, 4096> buffer{};
    const long long count = field( read_file( "/proc/self/status", buffer ), "Threads:" );
    console.printf( Level::info, "threads: %lld", count );
}

void fds( Console &console, const Args &, void * ) noexcept
{
    const int directory = ::open( "/proc/self/fd", O_RDONLY | O_DIRECTORY | O_CLOEXEC );
    if( directory < 0 ) {
        console.print( "open files: /proc/self/fd is not readable", Level::error );
        return;
    }
    // getdents64 straight into the stack: opendir() would allocate its DIR.
    // A record: 8-byte inode, 8-byte offset, 2-byte length, type, name.
    std::array<char, 2048> buffer{};
    long long entries = 0;
    for( ;; ) {
        const long got = ::syscall( SYS_getdents64, directory, buffer.data(), buffer.size() );
        if( got <= 0 ) {
            break;
        }
        for( long at = 0; at < got; ) {
            std::uint16_t length = 0;
            std::memcpy( &length, buffer.data() + at + 16, sizeof( length ) );
            const char *name = buffer.data() + at + 19;
            if( name[0] != '.' ) {
                ++entries;
            }
            at += length != 0 ? length : got;
        }
    }
    ::close( directory );
    console.printf( Level::info, "open files: %lld", entries - 1 ); // not the one reading them
}

void device( Console &console, const Args &, void * ) noexcept
{
#if defined( __BIONIC__ )
    const auto property = []( const char *name, std::span<char, PROP_VALUE_MAX> out ) {
        return __system_property_get( name, out.data() ) > 0 ? out.data() : "?";
    };
    std::array<char, PROP_VALUE_MAX> maker{};
    std::array<char, PROP_VALUE_MAX> model{};
    std::array<char, PROP_VALUE_MAX> release{};
    std::array<char, PROP_VALUE_MAX> sdk{};
    std::array<char, PROP_VALUE_MAX> abi{};
    console.printf(
        Level::info, "device: %s %s, Android %s (API %s), %s",
        property( "ro.product.manufacturer", maker ), property( "ro.product.model", model ),
        property( "ro.build.version.release", release ), property( "ro.build.version.sdk", sdk ),
        property( "ro.product.cpu.abi", abi ) );
#endif
    utsname name{};
    if( ::uname( &name ) == 0 ) {
        console.printf( Level::info, "kernel: %s %s, %s", name.sysname, name.release,
                        name.machine );
    } else {
        console.print( "kernel: uname failed", Level::error );
    }
}

// --- smaps: memory by kind ---------------------------------------------------------

/// What a mapping is, judged by its name in /proc/self/smaps. The first that
/// matches wins; the order is the order printed when two weigh the same.
enum class Kind : std::uint8_t {
    code,   // this program's own library
    heap,   // the C allocator's arenas
    gpu,    // the GPU driver: its device mappings, its libraries, graphics buffers
    java,   // the Java runtime every Android process carries from the zygote
    stacks, // thread stacks
    libs,   // every other shared library
    files,  // every other mapped file: fonts, APKs, resources
    other,  // anonymous memory with no better name
    count
};

constexpr std::array<const char *, static_cast<std::size_t>( Kind::count )> kind_names = {
    "this program", "native heap", "GPU driver",  "Java runtime",
    "stacks",       "other .so",   "other files", "other anon"
};

bool contains( std::string_view text, std::string_view part ) noexcept
{
    return text.find( part ) != std::string_view::npos;
}

Kind kind_of( std::string_view name, std::string_view self ) noexcept
{
    if( !self.empty() && name == self ) {
        return Kind::code;
    }
    if( name.starts_with( "[anon:scudo:" ) || name.starts_with( "[anon:libc_malloc" ) ||
        name.starts_with( "[anon:jemalloc" ) || name.starts_with( "[anon:GWP-ASan" ) ||
        name == "[heap]" ) {
        return Kind::heap;
    }
    if( contains( name, "mali" ) || contains( name, "kgsl" ) || contains( name, "adreno" ) ||
        contains( name, "dmabuf" ) || contains( name, "/egl/" ) || name.starts_with( "/dev/dri" ) ||
        name.starts_with( "/dev/ion" ) || contains( name, "gralloc" ) ) {
        return Kind::gpu;
    }
    if( name.starts_with( "[anon:dalvik-" ) || name.ends_with( ".art" ) ||
        name.ends_with( ".oat" ) || name.ends_with( ".vdex" ) || name.ends_with( ".odex" ) ||
        name.ends_with( ".jar" ) || contains( name, "/apex/com.android.art/" ) ) {
        return Kind::java;
    }
    if( name == "[stack]" || name.starts_with( "[anon:stack_and_tls" ) ||
        name.starts_with( "[anon:thread signal stack" ) ) {
        return Kind::stacks;
    }
    if( name.ends_with( ".so" ) || contains( name, ".so." ) || contains( name, ".so " ) ) {
        return Kind::libs;
    }
    if( name.starts_with( "/" ) ) {
        return Kind::files;
    }
    return Kind::other;
}

struct Usage
{
        long long pss = 0;   // kB
        long long rss = 0;   // kB
        long long dirty = 0; // kB, Private_Dirty
        long long swap = 0;  // kB, SwapPss
};

/// Whether a /proc/self/smaps line is a mapping's header ("start-end perms
/// offset dev inode name"); if so, its name (empty for anonymous memory).
bool header_name( std::string_view line, std::string_view &name ) noexcept
{
    const std::size_t dash = line.find( '-' );
    const std::size_t space = line.find( ' ' );
    if( line.empty() || dash == std::string_view::npos || space == std::string_view::npos ||
        dash > space || std::isxdigit( static_cast<unsigned char>( line[0] ) ) == 0 ) {
        return false;
    }
    std::size_t at = 0;
    for( int field = 0; field < 5; ++field ) {
        at = line.find( ' ', at );
        if( at == std::string_view::npos ) {
            name = {};
            return true;
        }
        while( at < line.size() && line[at] == ' ' ) {
            ++at;
        }
    }
    name = line.substr( at );
    if( name.ends_with( " (deleted)" ) ) {
        name.remove_suffix( 10 );
    }
    return true;
}

void smaps( Console &console, const Args &, void * ) noexcept
{
    // This program's library, so its own mappings are told apart from the rest.
    Dl_info self{};
    const std::string_view self_path =
        ::dladdr( reinterpret_cast<const void *>( &add_system_commands ), &self ) != 0 &&
                self.dli_fname != nullptr
            ? std::string_view( self.dli_fname )
            : std::string_view();

    const int fd = ::open( "/proc/self/smaps", O_RDONLY | O_CLOEXEC );
    if( fd < 0 ) {
        console.print( "smaps: /proc/self/smaps is not readable", Level::error );
        return;
    }
    std::array<Usage, static_cast<std::size_t>( Kind::count )> usage{};
    Usage total{};
    // Streamed through 4 KB on the stack: the file is a few hundred KB.
    std::array<char, 4096> buffer{};
    std::size_t held = 0;
    Kind current = Kind::other;
    bool reading = true;
    while( reading ) {
        const ssize_t got = ::read( fd, buffer.data() + held, buffer.size() - held );
        reading = got > 0;
        held += reading ? static_cast<std::size_t>( got ) : 0;
        std::size_t start = 0;
        for( ;; ) {
            const auto end = std::string_view( buffer.data() + start, held - start ).find( '\n' );
            if( end == std::string_view::npos ) {
                break;
            }
            const std::string_view line( buffer.data() + start, end );
            start += end + 1;
            std::string_view name;
            if( header_name( line, name ) ) {
                current = kind_of( name, self_path );
                continue;
            }
            Usage &kind = usage[static_cast<std::size_t>( current )];
            if( line.starts_with( "Rss:" ) ) {
                const long long value = field( line, "Rss:" );
                kind.rss += value;
                total.rss += value;
            } else if( line.starts_with( "Pss:" ) ) {
                const long long value = field( line, "Pss:" );
                kind.pss += value;
                total.pss += value;
            } else if( line.starts_with( "Private_Dirty:" ) ) {
                const long long value = field( line, "Private_Dirty:" );
                kind.dirty += value;
                total.dirty += value;
            } else if( line.starts_with( "SwapPss:" ) ) {
                const long long value = field( line, "SwapPss:" );
                kind.swap += value;
                total.swap += value;
            }
        }
        // Keep the unfinished line; a line longer than the buffer is dropped.
        if( start == 0 && held == buffer.size() ) {
            held = 0;
        } else {
            std::memmove( buffer.data(), buffer.data() + start, held - start );
            held -= start;
        }
    }
    ::close( fd );

    std::array<char, 24> pss{};
    std::array<char, 24> rss{};
    std::array<char, 24> dirty{};
    console.printf( Level::info, "memory by kind: PSS %s (RSS %s), private dirty %s",
                    megabytes( total.pss, pss ), megabytes( total.rss, rss ),
                    megabytes( total.dirty, dirty ) );
    // Largest share first: a selection by repeated passes, no sort, no copy.
    std::array<bool, static_cast<std::size_t>( Kind::count )> shown{};
    for( std::size_t row = 0; row < usage.size(); ++row ) {
        std::size_t best = usage.size();
        for( std::size_t k = 0; k < usage.size(); ++k ) {
            if( !shown[k] && ( best == usage.size() || usage[k].pss > usage[best].pss ) ) {
                best = k;
            }
        }
        shown[best] = true;
        if( usage[best].pss == 0 && usage[best].rss == 0 ) {
            continue;
        }
        console.printf( Level::info, "  %-12s pss %s, rss %s, dirty %s", kind_names[best],
                        megabytes( usage[best].pss, pss ), megabytes( usage[best].rss, rss ),
                        megabytes( usage[best].dirty, dirty ) );
    }
    if( total.swap > 0 ) {
        console.printf( Level::info, "  swapped out (pss): %s", megabytes( total.swap, pss ) );
    }
}

struct Builtin
{
        const char *name;
        const char *help;
        Handler handler;
};

constexpr std::array<Builtin, 6> builtins = { {
    { "mem", "mem: resident and peak memory, and the system's available memory", mem },
    { "heap", "heap: the allocator's in-use, free and mapped bytes", heap },
    { "threads", "threads: how many threads the process runs", threads },
    { "fds", "fds: how many files the process has open", fds },
    { "device", "device: maker, model, Android version and ABI; the kernel", device },
    { "smaps", "smaps: memory by kind: proportional (PSS), resident (RSS), private dirty", smaps },
} };

} // namespace

std::size_t add_system_commands( Console &console ) noexcept
{
    std::size_t refused = 0;
    for( const Builtin &builtin : builtins ) {
        if( console.add( builtin.name, builtin.help, builtin.handler ) !=
            Commands::AddResult::added ) {
            ++refused;
        }
    }
    return refused;
}

} // namespace sq::console
