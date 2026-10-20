#include <squared/files/posix_file_system.hpp>
#include <squared/files/src/file_error.hpp>
#include <squared/files/src/file_error_code.hpp>
#include <squared/files/src/file_read_result.hpp>
#include <squared/files/src/file_type.hpp>

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sq::files
{

namespace
{

/** @brief A file descriptor, closed on every path out of the scope. */
struct Descriptor
{
        int fd = -1;
        Descriptor( const Descriptor & ) = delete;
        Descriptor &operator=( const Descriptor & ) = delete;
        Descriptor( Descriptor && ) = delete;
        Descriptor &operator=( Descriptor && ) = delete;
        explicit Descriptor( int value ) noexcept
            : fd( value )
        {}
        ~Descriptor()
        {
            if( fd >= 0 ) {
                static_cast<void>( ::close( fd ) );
            }
        }
};

FileError make_error( FileErrorCode code, std::string message, std::string path )
{
    return FileError{ .code = code, .message = std::move( message ), .path = std::move( path ) };
}

/** @brief Translate errno into a stable category. */
FileErrorCode code_from_errno( int value ) noexcept
{
    switch( value ) {
        case ENOENT:
            return FileErrorCode::NotFound;
        case ENOTDIR:
            return FileErrorCode::NotADirectory;
        case EISDIR:
            return FileErrorCode::IsADirectory;
        case EACCES:
        case EPERM:
            return FileErrorCode::PermissionDenied;
        case EROFS:
            return FileErrorCode::ReadOnly;
        case EEXIST:
            return FileErrorCode::AlreadyExists;
        case ENAMETOOLONG:
        case EINVAL:
            return FileErrorCode::InvalidPath;
        case EFBIG:
        case EOVERFLOW:
            return FileErrorCode::TooLarge;
        default:
            return FileErrorCode::IoFailure;
    }
}

/** @brief The error errno describes now, for a path. */
FileError from_errno( std::string path )
{
    const int value = errno;
    return make_error( code_from_errno( value ), std::strerror( value ), std::move( path ) );
}

/**
 * @brief Reject a relative path that would escape its root.
 *
 * A leading '/' or any '..' segment is refused rather than normalised.
 */
bool escapes_root( std::string_view path ) noexcept
{
    if( path.starts_with( '/' ) ) {
        return true;
    }
    for( std::size_t start = 0; start <= path.size(); ) {
        const std::size_t end = std::min( path.find( '/', start ), path.size() );
        if( path.substr( start, end - start ) == ".." ) {
            return true;
        }
        start = end + 1;
    }
    return false;
}

/** @brief A resolved path, or why there is none. */
struct Target
{
        std::string path;
        FileError error;
};

/** @brief stat a path; false when it does not exist or cannot be read. */
bool stat_of( const std::string &path, struct stat &status ) noexcept
{
    return !path.empty() && ::stat( path.c_str(), &status ) == 0;
}

} // namespace

PosixFileSystem::PosixFileSystem( PosixFileSystemRoots roots )
    : roots_( std::move( roots ) )
{}

std::string PosixFileSystem::resolve_platform_path( FileType type, std::string_view path ) const
{
    if( type == FileType::Absolute ) {
        return std::string{ path };
    }
    const std::string &root = type == FileType::Internal ? roots_.internal_root
                              : type == FileType::Local  ? roots_.local_root
                                                         : roots_.external_root;
    if( root.empty() || escapes_root( path ) ) {
        return {};
    }
    if( path.empty() ) {
        return root;
    }
    std::string resolved;
    resolved.reserve( root.size() + path.size() + 1 ); // one allocation, not three
    resolved.assign( root );
    if( resolved.back() != '/' ) {
        resolved.push_back( '/' );
    }
    resolved.append( path );
    return resolved; // returning append()'s reference would copy
}

namespace
{

/** @brief Resolve for an operation; writes refuse Internal on every platform. */
Target target( const PosixFileSystem &files, FileType type, std::string_view path, bool writing )
{
    if( writing && type == FileType::Internal ) {
        return { {},
                 make_error( FileErrorCode::ReadOnly,
                             "internal storage is read-only on every platform",
                             std::string{ path } ) };
    }
    std::string resolved = files.resolve_platform_path( type, path );
    if( resolved.empty() ) {
        return { {},
                 make_error( FileErrorCode::NotSupported,
                             "no root configured for this file type, or path escapes it",
                             std::string{ path } ) };
    }
    return { std::move( resolved ), {} };
}

} // namespace

bool PosixFileSystem::exists( FileType type, std::string_view path ) const noexcept
{
    struct stat status{};
    return stat_of( resolve_platform_path( type, path ), status );
}

bool PosixFileSystem::is_directory( FileType type, std::string_view path ) const noexcept
{
    struct stat status{};
    return stat_of( resolve_platform_path( type, path ), status ) && S_ISDIR( status.st_mode );
}

std::uint64_t PosixFileSystem::length( FileType type, std::string_view path ) const noexcept
{
    struct stat status{};
    if( !stat_of( resolve_platform_path( type, path ), status ) || S_ISDIR( status.st_mode ) ) {
        return 0U;
    }
    return static_cast<std::uint64_t>( status.st_size );
}

FileReadResult PosixFileSystem::read( FileType type, std::string_view path ) const
{
    Target to = target( *this, type, path, false );
    if( to.error ) {
        return { .bytes = {}, .error = std::move( to.error ) };
    }
    const Descriptor file{ ::open( to.path.c_str(), O_RDONLY | O_CLOEXEC ) };
    struct stat status{};
    if( file.fd < 0 || ::fstat( file.fd, &status ) != 0 ) {
        return { .bytes = {}, .error = from_errno( std::move( to.path ) ) };
    }
    if( S_ISDIR( status.st_mode ) ) {
        return { .bytes = {},
                 .error = make_error( FileErrorCode::IsADirectory, std::strerror( EISDIR ),
                                      std::move( to.path ) ) };
    }
    FileReadResult result;
    result.bytes.resize( static_cast<std::size_t>( status.st_size ) );
    std::size_t used = 0;
    while( used < result.bytes.size() ) {
        const ssize_t got =
            ::read( file.fd, result.bytes.data() + used, result.bytes.size() - used );
        if( got < 0 && errno == EINTR ) {
            continue;
        }
        if( got < 0 ) {
            return { .bytes = {}, .error = from_errno( std::move( to.path ) ) };
        }
        if( got == 0 ) {
            break; // the file shrank since fstat
        }
        used += static_cast<std::size_t>( got );
    }
    result.bytes.resize( used );
    return result;
}

FileError PosixFileSystem::write( FileType type, std::string_view path,
                                  std::span<const std::byte> bytes, bool append )
{
    Target to = target( *this, type, path, true );
    if( to.error ) {
        return std::move( to.error );
    }
    const int mode = O_WRONLY | O_CREAT | O_CLOEXEC | ( append ? O_APPEND : O_TRUNC );
    Descriptor file{ ::open( to.path.c_str(), mode, 0644 ) };
    if( file.fd < 0 ) {
        return from_errno( std::move( to.path ) );
    }
    for( std::size_t used = 0; used < bytes.size(); ) {
        const ssize_t put = ::write( file.fd, bytes.data() + used, bytes.size() - used );
        if( put < 0 && errno == EINTR ) {
            continue;
        }
        if( put <= 0 ) {
            return from_errno( std::move( to.path ) );
        }
        used += static_cast<std::size_t>( put );
    }
    // close reports a failed write back, on file systems that delay them
    const int fd = std::exchange( file.fd, -1 );
    if( ::close( fd ) != 0 ) {
        return from_errno( std::move( to.path ) );
    }
    return {};
}

FileError PosixFileSystem::list( FileType type, std::string_view path,
                                 std::vector<std::string> &names ) const
{
    names.clear();
    Target to = target( *this, type, path, false );
    if( to.error ) {
        return std::move( to.error );
    }
    DIR *directory = ::opendir( to.path.c_str() );
    if( directory == nullptr ) {
        return from_errno( std::move( to.path ) );
    }
    // The bundle's metadata directory is hidden at Internal's root, as the
    // Android asset backend hides it, so both list the same names.
    const bool bundle_root = type == FileType::Internal && path.empty();
    while( const dirent *entry = ::readdir( directory ) ) {
        const std::string_view name{ static_cast<const char *>( entry->d_name ) };
        if( name != "." && name != ".." && !( bundle_root && name == "SQ-INF" ) ) {
            names.emplace_back( name );
        }
    }
    static_cast<void>( ::closedir( directory ) );
    return {};
}

FileError PosixFileSystem::make_directories( FileType type, std::string_view path )
{
    Target to = target( *this, type, path, true );
    if( to.error ) {
        return std::move( to.error );
    }
    // Each '/' in turn ends the path for one mkdir; the last one is the end.
    std::string &whole = to.path;
    for( std::size_t at = 1; at <= whole.size(); ++at ) {
        if( at < whole.size() && whole[at] != '/' ) {
            continue;
        }
        const char kept = whole[at];
        whole[at] = '\0';
        const bool failed = ::mkdir( whole.c_str(), 0755 ) != 0 && errno != EEXIST;
        whole[at] = kept;
        if( failed ) {
            return from_errno( whole.substr( 0, at ) );
        }
    }
    // EEXIST was accepted for every part; the whole must be a directory.
    struct stat status{};
    if( !stat_of( whole, status ) || !S_ISDIR( status.st_mode ) ) {
        return make_error( FileErrorCode::NotADirectory, std::strerror( ENOTDIR ),
                           std::move( whole ) );
    }
    return {};
}

FileError PosixFileSystem::remove( FileType type, std::string_view path )
{
    Target to = target( *this, type, path, true );
    if( to.error ) {
        return std::move( to.error );
    }
    // C's remove() unlinks a file or removes an empty directory.
    if( std::remove( to.path.c_str() ) != 0 ) {
        return from_errno( std::move( to.path ) );
    }
    return {};
}

} // namespace sq::files
