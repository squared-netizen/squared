// The built-in commands that read the running system. Linux interfaces, so
// the same code serves Android, Termux and desktop Linux; the C library is
// what differs (bionic has system properties and mallinfo, glibc has
// mallinfo2), so that is what the two #if blocks test, not the platform:
// Termux is the posix platform on bionic.

#include <squared/console/line_reader.hpp>
#include <squared/console/system_commands.hpp>

#include <dlfcn.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>

#if defined( __BIONIC__ )
#include <sys/system_properties.h>
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
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
    // MemAvailable is the kernel's own estimate, from Linux 3.14. Before it
    // (the Android 5.1 tablet's 3.10), free plus page cache is the usual
    // stand-in, and is said to be one.
    long long free_kb = field( meminfo, "MemAvailable:" );
    const bool estimated = free_kb < 0;
    if( estimated ) {
        const long long unused = field( meminfo, "MemFree:" );
        const long long cached = field( meminfo, "Cached:" );
        free_kb = unused >= 0 && cached >= 0 ? unused + cached : -1;
    }
    console.printf(
        Level::info, "memory: resident %s (peak %s); the system has %s%s of %s available",
        megabytes( field( status, "VmRSS:" ), resident ),
        megabytes( field( status, "VmHWM:" ), peak ), estimated ? "about " : "",
        megabytes( free_kb, available ), megabytes( field( meminfo, "MemTotal:" ), total ) );
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
    int directory = ::open( "/proc/self/fd", O_RDONLY | O_DIRECTORY | O_CLOEXEC );
    if( directory < 0 ) {
        // Linux before 3.17 lets a process that is not dumpable (a release
        // Android app) list its own descriptors only from the thread whose id
        // is the process's: the main one. The thread's own directory shows the
        // same, shared, table from any thread.
        std::array<char, 48> path{};
        std::snprintf( path.data(), path.size(), "/proc/self/task/%ld/fd",
                       ::syscall( SYS_gettid ) );
        directory = ::open( path.data(), O_RDONLY | O_DIRECTORY | O_CLOEXEC );
    }
    if( directory < 0 ) {
        console.print( "open files: neither /proc/self/fd nor this thread's is readable",
                       Level::error );
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
    Kind current = Kind::other;
    for_each_line( fd, buffer, [&]( std::string_view line ) {
        std::string_view name;
        if( header_name( line, name ) ) {
            current = kind_of( name, self_path );
            return;
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
    } );
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

/// FNV-1a: a library's path, as a number, to count it once without storing it.
std::uint64_t path_hash( std::string_view text ) noexcept
{
    std::uint64_t hash = 0xcbf29ce484222325ULL;
    for( const char c : text ) {
        hash = ( hash ^ static_cast<unsigned char>( c ) ) * 0x100000001b3ULL;
    }
    return hash;
}

/// One line of /proc/self/maps: its span in bytes and the mapped path ("" for none).
struct Mapping
{
        std::uint64_t bytes;
        std::string_view path;
};

Mapping mapping_of( std::string_view line ) noexcept
{
    Mapping out{ 0, {} };
    std::uint64_t start = 0;
    std::uint64_t end = 0;
    const auto dash = line.find( '-' );
    const auto space = line.find( ' ' );
    if( dash == std::string_view::npos || space == std::string_view::npos || dash > space ) {
        return out;
    }
    std::from_chars( line.data(), line.data() + dash, start, 16 );
    std::from_chars( line.data() + dash + 1, line.data() + space, end, 16 );
    out.bytes = end > start ? end - start : 0;
    const auto slash = line.find( '/' );
    if( slash != std::string_view::npos ) {
        out.path = line.substr( slash );
    }
    return out;
}

[[nodiscard]] bool is_library( std::string_view path ) noexcept
{
    return path.size() > 3 && ( path.substr( path.size() - 3 ) == ".so" ||
                                path.find( ".so." ) != std::string_view::npos );
}

void libs( Console &console, const Args &, void * ) noexcept
{
    // Two passes over the maps, so nothing but numbers is kept: the first adds
    // up each library's spans (up to 256 libraries), the second prints each
    // at its first line, where its path is at hand.
    struct Library
    {
            std::uint64_t hash;
            std::uint64_t bytes;
            bool printed;
    };
    std::array<Library, 256> table{};
    std::size_t count = 0;
    std::uint64_t total = 0;
    std::array<char, 1024> buffer{};
    const auto find = [&]( std::uint64_t hash ) -> Library * {
        for( std::size_t i = 0; i < count; ++i ) {
            if( table[i].hash == hash ) {
                return &table[i];
            }
        }
        return nullptr;
    };
    int fd = ::open( "/proc/self/maps", O_RDONLY | O_CLOEXEC );
    if( fd < 0 ) {
        console.print( "libs: /proc/self/maps is not readable", Level::error );
        return;
    }
    for_each_line( fd, buffer, [&]( std::string_view line ) {
        const Mapping m = mapping_of( line );
        if( !is_library( m.path ) ) {
            return;
        }
        const std::uint64_t hash = path_hash( m.path );
        Library *library = find( hash );
        if( library == nullptr && count < table.size() ) {
            library = &table[count++];
            *library = { hash, 0, false };
        }
        if( library != nullptr ) {
            library->bytes += m.bytes;
            total += m.bytes;
        }
    } );
    ::close( fd );
    console.printf( Level::info, "libs: %zu libraries, %.1f MB of address space%s", count,
                    static_cast<double>( total ) / ( 1024.0 * 1024.0 ),
                    count == table.size() ? " (the first 256)" : "" );
    fd = ::open( "/proc/self/maps", O_RDONLY | O_CLOEXEC );
    if( fd < 0 ) {
        return;
    }
    for_each_line( fd, buffer, [&]( std::string_view line ) {
        const Mapping m = mapping_of( line );
        if( !is_library( m.path ) ) {
            return;
        }
        Library *library = find( path_hash( m.path ) );
        if( library == nullptr || library->printed ) {
            return;
        }
        library->printed = true;
        console.printf( Level::info, "  %7llu KB  %.*s",
                        static_cast<unsigned long long>( library->bytes / 1024 ),
                        static_cast<int>( m.path.size() ), m.path.data() );
    } );
    ::close( fd );
}

struct Builtin
{
        const char *name;
        const char *help;
        Handler handler;
};

constexpr std::array<Builtin, 7> builtins = { {
    { "mem", "mem: resident and peak memory, and the system's available memory", mem },
    { "heap", "heap: the allocator's in-use, free and mapped bytes", heap },
    { "threads", "threads: how many threads the process runs", threads },
    { "fds", "fds: how many files the process has open", fds },
    { "device", "device: maker, model, Android version and ABI; the kernel", device },
    { "smaps", "smaps: memory by kind: proportional (PSS), resident (RSS), private dirty", smaps },
    { "libs", "libs: the shared libraries the process has loaded, and their size", libs },
} };

} // namespace

bool run_program( Console &console, const char *const *argv, std::size_t max_lines ) noexcept
{
    std::array<int, 2> pipe{};
    if( argv == nullptr || argv[0] == nullptr || ::pipe2( pipe.data(), O_CLOEXEC ) != 0 ) {
        return false;
    }
    const pid_t child = ::fork();
    if( child < 0 ) {
        ::close( pipe[0] );
        ::close( pipe[1] );
        return false;
    }
    if( child == 0 ) {
        // In the child only what is safe between fork and exec: dup2, exec, _exit.
        ::dup2( pipe[1], STDOUT_FILENO );
        ::dup2( pipe[1], STDERR_FILENO );
        // execv's argv is char *const[]; the strings are not written to.
        ::execv( argv[0], const_cast<char *const *>( argv ) );
        ::_exit( 127 );
    }
    ::close( pipe[1] );
    std::size_t shown = 0;
    std::size_t total = 0;
    std::array<char, 2048> buffer{};
    for_each_line( pipe[0], buffer, [&]( std::string_view line ) {
        ++total;
        if( shown < max_lines ) {
            console.print( line );
            ++shown;
        }
    } );
    ::close( pipe[0] );
    int status = 0;
    while( ::waitpid( child, &status, 0 ) < 0 && errno == EINTR ) {
    }
    if( total > shown ) {
        console.printf( Level::info, "... and %zu more lines", total - shown );
    }
    return !( WIFEXITED( status ) && WEXITSTATUS( status ) == 127 );
}

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
