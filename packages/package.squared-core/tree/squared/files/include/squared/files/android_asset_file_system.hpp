#pragma once

#include <squared/files/file_error.hpp>
#include <squared/files/file_read_result.hpp>
#include <squared/files/file_system.hpp>
#include <squared/files/file_type.hpp>
#include <squared/files/posix_file_system.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

struct AAssetManager;

namespace sq::files {

/** @brief Directories the writable file types resolve to on Android. */
struct AndroidStorageRoots final {
    /**
     * @brief Private writable storage; `ANativeActivity::internalDataPath`.
     *
     * Android calls this internal storage and libGDX calls it Local, which is
     * the same place under two names. No permission is needed, and it is
     * removed when the application is uninstalled.
     */
    std::string local_root;

    /**
     * @brief App-scoped shared storage; `ANativeActivity::externalDataPath`.
     *
     * May be empty: external storage is not always mounted, and the platform
     * reports null when it is not. An empty root makes every path under that
     * type fail with NotSupported rather than resolving somewhere unintended.
     */
    std::string external_root;
};

/**
 * @brief FileSystem over an APK's assets and the app's private storage.
 *
 * The second implementation the porting layer was shaped for. `Internal`
 * reads through AAssetManager, because assets inside an APK are zip entries
 * rather than files and no POSIX call can reach them. Everything writable is
 * an ordinary directory and is delegated to PosixFileSystem.
 *
 * The asset manager is referenced, not owned, and comes from
 * `ANativeActivity::assetManager`. It must outlive this file system.
 *
 * @note Names on Android are confusing and worth stating once. Android's
 * "internal storage" is this class's Local; this class's Internal is the read
 * only asset bundle, which Android does not call storage at all.
 */
class AndroidAssetFileSystem final : public FileSystem {
public:
    /**
     * @brief Bind to an asset manager and the writable roots.
     * @param assets Asset manager from the activity; must outlive this.
     * @param roots Directories for Local and External.
     */
    AndroidAssetFileSystem(
        AAssetManager& assets,
        AndroidStorageRoots roots
    );

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

private:
    AAssetManager* assets_;
    PosixFileSystem storage_;
};

}  // namespace sq::files
