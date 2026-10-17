// The built-in commands that read the running system. Linux interfaces, so
// the same code serves Android, Termux and desktop Linux; the C library is
// what differs (bionic has system properties and mallinfo, glibc has
// mallinfo2), so that is what the two #if blocks test, not the platform:
// Termux is the posix platform on bionic.

#include <squared/console/system_commands.hpp>

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
    struct utsname name
    {};
    if( ::uname( &name ) == 0 ) {
        console.printf( Level::info, "kernel: %s %s, %s", name.sysname, name.release,
                        name.machine );
    } else {
        console.print( "kernel: uname failed", Level::error );
    }
}

struct Builtin
{
        const char *name;
        const char *help;
        Handler handler;
};

constexpr std::array<Builtin, 5> builtins = { {
    { "mem", "mem: resident and peak memory, and the system's available memory", mem },
    { "heap", "heap: the allocator's in-use, free and mapped bytes", heap },
    { "threads", "threads: how many threads the process runs", threads },
    { "fds", "fds: how many files the process has open", fds },
    { "device", "device: maker, model, Android version and ABI; the kernel", device },
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
