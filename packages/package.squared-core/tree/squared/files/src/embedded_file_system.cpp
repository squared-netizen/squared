#include <squared/files/embedded_file_system.hpp>
#include <squared/files/src/file_error_code.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sq::files
{

namespace
{

FileError error_of( FileErrorCode code, std::string message, std::string_view path )
{
    return FileError{ .code = code, .message = std::move( message ), .path = std::string( path ) };
}

/// The part of `file` after the directory `directory`; empty when it is not under it.
[[nodiscard]] std::string_view under( std::string_view file, std::string_view directory ) noexcept
{
    if( directory.empty() ) {
        return file;
    }
    if( file.size() <= directory.size() + 1 || !file.starts_with( directory ) ) {
        return {};
    }
    if( file[directory.size()] != '/' ) {
        return {};
    }
    return file.substr( directory.size() + 1 );
}

} // namespace

const EmbeddedFile *EmbeddedFileSystem::find( FileType type, std::string_view path ) const noexcept
{
    if( type != FileType::Internal ) {
        return nullptr;
    }
    for( const EmbeddedFile &file : files_ ) {
        if( file.path == path ) {
            return &file;
        }
    }
    return nullptr;
}

bool EmbeddedFileSystem::exists( FileType type, std::string_view path ) const noexcept
{
    return find( type, path ) != nullptr || is_directory( type, path );
}

bool EmbeddedFileSystem::is_directory( FileType type, std::string_view path ) const noexcept
{
    if( type != FileType::Internal ) {
        return false;
    }
    for( const EmbeddedFile &file : files_ ) {
        if( !under( file.path, path ).empty() ) {
            return true;
        }
    }
    return false;
}

std::uint64_t EmbeddedFileSystem::length( FileType type, std::string_view path ) const noexcept
{
    const EmbeddedFile *file = find( type, path );
    return file != nullptr ? file->bytes.size() : 0;
}

FileReadResult EmbeddedFileSystem::read( FileType type, std::string_view path ) const
{
    FileReadResult result;
    if( type != FileType::Internal ) {
        result.error = error_of( FileErrorCode::NotSupported,
                                 "an embedded file system holds bundled files only", path );
        return result;
    }
    const EmbeddedFile *file = find( type, path );
    if( file == nullptr ) {
        if( is_directory( type, path ) ) {
            result.error = error_of( FileErrorCode::IsADirectory, "a directory", path );
        } else {
            result.error = error_of( FileErrorCode::NotFound, "no embedded file", path );
        }
        return result;
    }
    result.bytes.resize( file->bytes.size() );
    if( !file->bytes.empty() ) {
        std::memcpy( result.bytes.data(), file->bytes.data(), file->bytes.size() );
    }
    return result;
}

FileError EmbeddedFileSystem::write( FileType /*type*/, std::string_view path,
                                     std::span<const std::byte> /*bytes*/, bool /*append*/ )
{
    return error_of( FileErrorCode::ReadOnly, "embedded files cannot be written", path );
}

FileError EmbeddedFileSystem::list( FileType type, std::string_view path,
                                    std::vector<std::string> &names ) const
{
    names.clear();
    if( type != FileType::Internal ) {
        return error_of( FileErrorCode::NotSupported,
                         "an embedded file system holds bundled files only", path );
    }
    if( find( type, path ) != nullptr ) {
        return error_of( FileErrorCode::NotADirectory, "a file, not a directory", path );
    }
    for( const EmbeddedFile &file : files_ ) {
        const std::string_view rest = under( file.path, path );
        if( !rest.empty() ) {
            names.emplace_back( rest.substr( 0, rest.find( '/' ) ) );
        }
    }
    if( names.empty() && !path.empty() ) {
        return error_of( FileErrorCode::NotFound, "no embedded directory", path );
    }
    std::sort( names.begin(), names.end() );
    names.erase( std::unique( names.begin(), names.end() ), names.end() );
    return {};
}

FileError EmbeddedFileSystem::make_directories( FileType /*type*/, std::string_view path )
{
    return error_of( FileErrorCode::ReadOnly, "embedded files cannot be written", path );
}

FileError EmbeddedFileSystem::remove( FileType /*type*/, std::string_view path )
{
    return error_of( FileErrorCode::ReadOnly, "embedded files cannot be written", path );
}

} // namespace sq::files
