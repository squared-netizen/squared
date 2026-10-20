#include <squared/console/file_commands.hpp>
#include <squared/console/line_reader.hpp>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string_view>

namespace sq::console
{

namespace
{

struct Files
{
        const char *home;
        Bridge *bridge;
};

/// Files for the three commands; one set, as there is one Console.
Files files{ nullptr, nullptr };

using Path = std::array<char, 512>;

/// The installed APK: the first mapping of a file ending in ".apk".
bool apk_path( Path &out ) noexcept
{
    const int fd = ::open( "/proc/self/maps", O_RDONLY | O_CLOEXEC );
    if( fd < 0 ) {
        return false;
    }
    bool found = false;
    std::array<char, 1024> buffer{};
    for_each_line( fd, buffer, [&]( std::string_view line ) {
        const auto slash = line.find( '/' );
        if( found || slash == std::string_view::npos ) {
            return;
        }
        const std::string_view path = line.substr( slash );
        if( path.size() > 4 && path.substr( path.size() - 4 ) == ".apk" &&
            path.size() < out.size() ) {
            std::copy( path.begin(), path.end(), out.begin() );
            out[path.size()] = '\0';
            found = true;
        }
    } );
    ::close( fd );
    return found;
}

/// `argument` as a path: `apk`, absolute, or under the home directory.
bool resolve( std::string_view argument, Path &out ) noexcept
{
    if( argument == "apk" ) {
        return apk_path( out );
    }
    int length = 0;
    if( !argument.empty() && argument.front() == '/' ) {
        length = std::snprintf( out.data(), out.size(), "%.*s", static_cast<int>( argument.size() ),
                                argument.data() );
    } else {
        length = std::snprintf( out.data(), out.size(), "%s/%.*s",
                                files.home != nullptr ? files.home : ".",
                                static_cast<int>( argument.size() ), argument.data() );
    }
    return length > 0 && static_cast<std::size_t>( length ) < out.size();
}

std::string_view base_name( std::string_view path ) noexcept
{
    while( path.size() > 1 && path.back() == '/' ) {
        path.remove_suffix( 1 );
    }
    const auto slash = path.rfind( '/' );
    return slash == std::string_view::npos ? path : path.substr( slash + 1 );
}

char type_of( mode_t mode ) noexcept
{
    if( S_ISDIR( mode ) ) {
        return 'd';
    }
    if( S_ISLNK( mode ) ) {
        return 'l';
    }
    return S_ISREG( mode ) ? 'f' : '?';
}

void ls( Console &console, const Args &args, void * ) noexcept
{
    Path path{};
    const std::string_view argument = args.size() > 1 ? args.text( 1 ) : std::string_view{ "." };
    if( !resolve( argument, path ) ) {
        console.print( "ls: the path is too long", Level::error );
        return;
    }
    const int directory = ::open( path.data(), O_RDONLY | O_DIRECTORY | O_CLOEXEC );
    if( directory < 0 ) {
        console.printf( Level::error, "ls: %s: %s", path.data(), std::strerror( errno ) );
        return;
    }
    // getdents64 straight into the stack: readdir() would allocate its DIR.
    std::array<char, 4096> buffer{};
    std::size_t entries = 0;
    console.printf( Level::info, "ls: %s", path.data() );
    for( ;; ) {
        const long got = ::syscall( SYS_getdents64, directory, buffer.data(), buffer.size() );
        if( got <= 0 ) {
            break;
        }
        for( long at = 0; at < got; ) {
            // struct linux_dirent64: u64 ino, s64 off, u16 reclen, u8 type, name.
            std::uint16_t length = 0;
            std::memcpy( &length, buffer.data() + at + 16, sizeof( length ) );
            const char *name = buffer.data() + at + 19;
            at += length;
            if( length == 0 ) {
                break;
            }
            if( std::strcmp( name, "." ) == 0 || std::strcmp( name, ".." ) == 0 ) {
                continue;
            }
            struct stat status = {};
            const bool known = ::fstatat( directory, name, &status, AT_SYMLINK_NOFOLLOW ) == 0;
            const char type = known ? type_of( status.st_mode ) : '?';
            if( type == 'f' ) {
                console.printf( Level::info, "  f %10lld  %s",
                                static_cast<long long>( status.st_size ), name );
            } else {
                console.printf( Level::info, "  %c %10s  %s%s", type, "-", name,
                                type == 'd' ? "/" : "" );
            }
            ++entries;
        }
    }
    ::close( directory );
    console.printf( Level::info, "ls: %zu %s", entries, entries == 1 ? "entry" : "entries" );
}

void pull( Console &console, const Args &args, void * ) noexcept
{
    if( args.size() < 2 ) {
        console.print( "pull: pull <path> (files/crash.txt, cache/x, apk, /proc/self/maps...)",
                       Level::error );
        return;
    }
    if( files.bridge == nullptr || !files.bridge->connected() ) {
        console.print( "pull: needs Termux connected: make console in the project", Level::error );
        return;
    }
    Path path{};
    if( !resolve( args.text( 1 ), path ) ) {
        console.print( args.text( 1 ) == "apk"
                           ? "pull: the APK was not found in this process's maps"
                           : "pull: the path is too long",
                       Level::error );
        return;
    }
    const int fd = ::open( path.data(), O_RDONLY | O_CLOEXEC );
    struct stat status = {};
    if( fd < 0 || ::fstat( fd, &status ) != 0 || !S_ISREG( status.st_mode ) ) {
        console.printf( Level::error, "pull: %s: %s", path.data(),
                        fd < 0 ? std::strerror( errno ) : "not a regular file" );
        if( fd >= 0 ) {
            ::close( fd );
        }
        return;
    }
    // A /proc file says size 0 but has content: sent as far as it reads.
    const auto size =
        status.st_size == 0 ? Bridge::unknown_size : static_cast<std::uint64_t>( status.st_size );
    const std::string_view name = base_name( { path.data() } );
    if( size == Bridge::unknown_size ) {
        console.printf( Level::info, "pull: %s, to Termux", path.data() );
    } else {
        console.printf( Level::info, "pull: %s, %llu bytes, to Termux", path.data(),
                        static_cast<unsigned long long>( size ) );
    }
    if( !files.bridge->send_file( fd, name, size ) ) {
        ::close( fd );
        console.print( "pull: another file is on its way; try again when it has arrived",
                       Level::error );
    }
}

void push_help( Console &console, const Args &, void * ) noexcept
{
    console.print( "push: typed in make console (Termux): push <file> [name]; it lands in the "
                   "app's files/" );
}

} // namespace

std::size_t add_file_commands( Console &console, const char *home, Bridge *bridge ) noexcept
{
    files = { home, bridge };
    using Added = Commands::AddResult;
    std::size_t refused = 0;
    refused += console.add( "ls", "ls [path]: a directory's entries (relative to the app's data)",
                            ls ) != Added::added
                   ? 1
                   : 0;
    refused += console.add( "pull", "pull <path>: a file to Termux (make console saves it)",
                            pull ) != Added::added
                   ? 1
                   : 0;
    refused += console.add( "push", "push: from make console: push <file> [name]", push_help ) !=
                       Added::added
                   ? 1
                   : 0;
    return refused;
}

} // namespace sq::console
