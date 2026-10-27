#pragma once

#include <squared/files/file_handle.hpp>
#include <squared/files/src/file_error.hpp>
#include <squared/files/src/file_read_result.hpp>
#include <squared/files/src/file_type.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sq::files
{

/**
 * @brief The framework's porting layer for storage.
 *
 * This is one of the few interfaces squared declares on purpose. A second
 * concrete implementation is guaranteed by the architecture rather than
 * speculative: Android internal storage is an asset bundle inside the APK and
 * is not reachable through POSIX, while Termux and desktop Linux are ordinary
 * directories. Everything above this class names a FileType and a relative
 * path and never learns which backend answered.
 *
 * Implementations are not required to be thread-safe.
 */
class FileSystem
{
    public:
        FileSystem() = default;
        virtual ~FileSystem() = default;

        FileSystem( const FileSystem & ) = delete;
        FileSystem &operator=( const FileSystem & ) = delete;
        FileSystem( FileSystem && ) = delete;
        FileSystem &operator=( FileSystem && ) = delete;

        /** @brief Return a handle under the read-only bundled assets. */
        [[nodiscard]] FileHandle internal( std::string_view path );

        /** @brief Return a handle under this application's private storage. */
        [[nodiscard]] FileHandle local( std::string_view path );

        /** @brief Return a handle under shared storage. */
        [[nodiscard]] FileHandle external( std::string_view path );

        /** @brief Return a handle for a path used exactly as given. */
        [[nodiscard]] FileHandle absolute( std::string_view path );

        /** @brief Return a handle under an explicitly chosen root. */
        [[nodiscard]] FileHandle resolve( FileType type, std::string_view path );

        /** @brief Report whether anything exists at a path. */
        [[nodiscard]] virtual bool exists( FileType type,
                                           std::string_view path ) const noexcept = 0;

        /** @brief Report whether a path names a directory. */
        [[nodiscard]] virtual bool is_directory( FileType type,
                                                 std::string_view path ) const noexcept = 0;

        /** @brief Return a file's size, or zero when it is missing or a directory. */
        [[nodiscard]] virtual std::uint64_t length( FileType type,
                                                    std::string_view path ) const noexcept = 0;

        /** @brief Read a whole file. */
        [[nodiscard]] virtual FileReadResult read( FileType type, std::string_view path ) const = 0;

        /** @brief Write a whole file, truncating or appending. */
        [[nodiscard]] virtual FileError write( FileType type, std::string_view path,
                                               std::span<const std::byte> bytes, bool append ) = 0;

        /**
         * @brief Write a whole file so that it holds the old bytes or the new,
         * never a part: written beside it (`<path>.new`), flushed to storage,
         * and renamed over it. What a save that the process may die during
         * needs. The directory must exist.
         */
        [[nodiscard]] virtual FileError write_atomically( FileType type, std::string_view path,
                                                          std::span<const std::byte> bytes ) = 0;

        /** @brief Fill names with one entry name per directory entry. */
        [[nodiscard]] virtual FileError list( FileType type, std::string_view path,
                                              std::vector<std::string> &names ) const = 0;

        /** @brief Create a directory and any missing parents. */
        [[nodiscard]] virtual FileError make_directories( FileType type,
                                                          std::string_view path ) = 0;

        /** @brief Remove a file, or an empty directory. */
        [[nodiscard]] virtual FileError remove( FileType type, std::string_view path ) = 0;

        /**
         * @brief The platform's own path a type and relative path name; empty
         * when there is none (bundled files inside an APK, embedded files, a
         * root not set, or a path that escapes it).
         *
         * For what outlives this file system: the App keeps where it saves,
         * and writes there through `FileType::Absolute` from the model's
         * thread after the activity that made this file system is gone.
         */
        [[nodiscard]] virtual std::string resolve_platform_path( FileType type,
                                                                 std::string_view path ) const = 0;
};

} // namespace sq::files
