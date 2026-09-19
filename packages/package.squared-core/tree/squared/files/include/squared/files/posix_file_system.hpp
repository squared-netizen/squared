#pragma once

#include <squared/files/file_error.hpp>
#include <squared/files/file_read_result.hpp>
#include <squared/files/file_system.hpp>
#include <squared/files/file_type.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sq::files {

/** @brief Directories each FileType resolves to under a POSIX file system. */
struct PosixFileSystemRoots final {
    /** @brief Read-only bundled assets; typically an `assets` directory. */
    std::string internal_root;

    /** @brief Private writable storage for this application. */
    std::string local_root;

    /** @brief Shared writable storage. */
    std::string external_root;
};

/**
 * @brief FileSystem over ordinary directories.
 *
 * This is the backend for Termux and desktop Linux, and it is the one used on
 * Android for Local and External storage. Android Internal storage lives
 * inside the APK and needs the asset backend instead.
 *
 * Internal is refused for writes on every platform, so a build that works
 * against a directory in Termux behaves the same on a device where the same
 * files are read-only inside an APK.
 */
class PosixFileSystem final : public FileSystem {
public:
    /**
     * @brief Bind each root to a directory.
     * @param roots Directories for Internal, Local and External. An empty root
     * makes every path under that type fail with NotSupported rather than
     * resolving somewhere unintended.
     */
    explicit PosixFileSystem(PosixFileSystemRoots roots);

    [[nodiscard]] bool exists(
        FileType type,
        std::string_view path
    ) const noexcept override;

    [[nodiscard]] bool is_directory(
        FileType type,
        std::string_view path
    ) const noexcept override;

    [[nodiscard]] std::uint64_t length(
        FileType type,
        std::string_view path
    ) const noexcept override;

    [[nodiscard]] FileReadResult read(
        FileType type,
        std::string_view path
    ) const override;

    [[nodiscard]] FileError write(
        FileType type,
        std::string_view path,
        std::span<const std::byte> bytes,
        bool append
    ) override;

    [[nodiscard]] FileError list(
        FileType type,
        std::string_view path,
        std::vector<std::string>& names
    ) const override;

    [[nodiscard]] FileError make_directories(
        FileType type,
        std::string_view path
    ) override;

    [[nodiscard]] FileError remove(
        FileType type,
        std::string_view path
    ) override;

    /**
     * @brief Return the platform path a type and relative path resolve to.
     * @param type Root to resolve against.
     * @param path Relative path, or the whole path for FileType::Absolute.
     * @return The resolved path, or an empty string when the root is unset or
     * the relative path escapes it.
     * @note Exposed for diagnostics and tests. Code above the file system
     * should not need it.
     */
    [[nodiscard]] std::string resolve_platform_path(
        FileType type,
        std::string_view path
    ) const;

private:
    PosixFileSystemRoots roots_;
};

}  // namespace sq::files
