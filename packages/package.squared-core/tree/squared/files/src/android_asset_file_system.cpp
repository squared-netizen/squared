#include <squared/files/android_asset_file_system.hpp>

#include <squared/files/file_error.hpp>
#include <squared/files/file_error_code.hpp>
#include <squared/files/file_read_result.hpp>
#include <squared/files/file_type.hpp>
#include <squared/files/posix_file_system.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <android/asset_manager.h>

namespace sq::files {

namespace {

FileError make_error(
    FileErrorCode code,
    std::string message,
    std::string_view path
)
{
    return FileError{
        .code = code,
        .message = std::move(message),
        .path = std::string{path}
    };
}

FileError read_only(std::string_view path)
{
    return make_error(
        FileErrorCode::ReadOnly,
        "internal storage is the APK asset bundle and cannot be written",
        path
    );
}

/**
 * @brief Reject a path that would escape the asset root.
 *
 * AAssetManager does not normalise, and a leading '/' or a '..' segment would
 * be passed through to whatever the platform makes of it. Refusing is the same
 * rule PosixFileSystem applies, for the same reason.
 */
bool escapes_root(std::string_view path) noexcept
{
    if (!path.empty() && path.front() == '/') return true;

    std::size_t start = 0;
    while (start <= path.size()) {
        const std::size_t end = path.find('/', start);
        const std::string_view segment = path.substr(
            start,
            end == std::string_view::npos ? std::string_view::npos : end - start
        );
        if (segment == "..") return true;
        if (end == std::string_view::npos) break;
        start = end + 1;
    }
    return false;
}

}  // namespace

AndroidAssetFileSystem::AndroidAssetFileSystem(
    AAssetManager& assets,
    AndroidStorageRoots roots
)
    : assets_(&assets)
    , storage_(PosixFileSystemRoots{
          // Internal is served from the asset manager, never from a directory.
          // Leaving this empty means a delegated Internal call fails rather
          // than silently resolving somewhere on the filesystem.
          .internal_root = {},
          .local_root = std::move(roots.local_root),
          .external_root = std::move(roots.external_root)
      })
{
}

bool AndroidAssetFileSystem::exists(
    FileType type,
    std::string_view path
) const noexcept
{
    if (type != FileType::Internal) return storage_.exists(type, path);
    if (escapes_root(path)) return false;

    const std::string name{path};
    AAsset* asset =
        AAssetManager_open(assets_, name.c_str(), AASSET_MODE_UNKNOWN);
    if (asset != nullptr) {
        AAsset_close(asset);
        return true;
    }

    // A directory is not openable as an asset, so an unopenable path may still
    // be a directory that exists. openDir succeeds for any path, so the test
    // is whether it has at least one entry.
    AAssetDir* directory = AAssetManager_openDir(assets_, name.c_str());
    if (directory == nullptr) return false;
    const bool populated = AAssetDir_getNextFileName(directory) != nullptr;
    AAssetDir_close(directory);
    return populated;
}

bool AndroidAssetFileSystem::is_directory(
    FileType type,
    std::string_view path
) const noexcept
{
    if (type != FileType::Internal) return storage_.is_directory(type, path);
    if (escapes_root(path)) return false;

    const std::string name{path};
    AAsset* asset =
        AAssetManager_open(assets_, name.c_str(), AASSET_MODE_UNKNOWN);
    if (asset != nullptr) {
        AAsset_close(asset);
        return false;   // it is a file
    }

    AAssetDir* directory = AAssetManager_openDir(assets_, name.c_str());
    if (directory == nullptr) return false;
    const bool populated = AAssetDir_getNextFileName(directory) != nullptr;
    AAssetDir_close(directory);
    return populated;
}

std::uint64_t AndroidAssetFileSystem::length(
    FileType type,
    std::string_view path
) const noexcept
{
    if (type != FileType::Internal) return storage_.length(type, path);
    if (escapes_root(path)) return 0U;

    const std::string name{path};
    AAsset* asset =
        AAssetManager_open(assets_, name.c_str(), AASSET_MODE_UNKNOWN);
    if (asset == nullptr) return 0U;

    const off64_t size = AAsset_getLength64(asset);
    AAsset_close(asset);
    return size > 0 ? static_cast<std::uint64_t>(size) : 0U;
}

FileReadResult AndroidAssetFileSystem::read(
    FileType type,
    std::string_view path
) const
{
    if (type != FileType::Internal) return storage_.read(type, path);

    if (escapes_root(path)) {
        return FileReadResult{
            .bytes = {},
            .error = make_error(
                FileErrorCode::NotSupported,
                "asset path escapes the bundle root",
                path
            )
        };
    }

    const std::string name{path};
    // STREAMING rather than BUFFER: the asset is read once into a vector the
    // caller owns, so asking the platform to mmap or fully buffer it as well
    // would hold two copies of a texture atlas at the same moment.
    AAsset* asset =
        AAssetManager_open(assets_, name.c_str(), AASSET_MODE_STREAMING);
    if (asset == nullptr) {
        return FileReadResult{
            .bytes = {},
            .error = make_error(
                FileErrorCode::NotFound, "no such asset", path
            )
        };
    }

    const off64_t size = AAsset_getLength64(asset);
    if (size < 0) {
        AAsset_close(asset);
        return FileReadResult{
            .bytes = {},
            .error = make_error(
                FileErrorCode::IoFailure, "asset reports a negative size", path
            )
        };
    }

    FileReadResult result;
    result.bytes.resize(static_cast<std::size_t>(size));

    std::size_t filled = 0;
    while (filled < result.bytes.size()) {
        // AAsset_read returns a short count on a compressed asset without it
        // being an error, so one call is not enough. Looping is what makes a
        // compressed skin atlas read correctly.
        const int read_count = AAsset_read(
            asset,
            result.bytes.data() + filled,
            result.bytes.size() - filled
        );
        if (read_count < 0) {
            AAsset_close(asset);
            return FileReadResult{
                .bytes = {},
                .error = make_error(
                    FileErrorCode::IoFailure, "asset read failed", path
                )
            };
        }
        if (read_count == 0) break;   // end of asset
        filled += static_cast<std::size_t>(read_count);
    }

    AAsset_close(asset);
    result.bytes.resize(filled);
    return result;
}

FileError AndroidAssetFileSystem::write(
    FileType type,
    std::string_view path,
    std::span<const std::byte> bytes,
    bool append
)
{
    if (type == FileType::Internal) return read_only(path);
    return storage_.write(type, path, bytes, append);
}

FileError AndroidAssetFileSystem::list(
    FileType type,
    std::string_view path,
    std::vector<std::string>& names
) const
{
    if (type != FileType::Internal) return storage_.list(type, path, names);

    names.clear();
    if (escapes_root(path)) {
        return make_error(
            FileErrorCode::NotSupported,
            "asset path escapes the bundle root",
            path
        );
    }

    const std::string name{path};
    AAssetDir* directory = AAssetManager_openDir(assets_, name.c_str());
    if (directory == nullptr) {
        return make_error(
            FileErrorCode::NotFound, "no such asset directory", path
        );
    }

    // AAssetDir lists files only; subdirectories are invisible to it. That is
    // a platform limitation rather than a choice here, and it is why an asset
    // tree is best kept flat enough to address by path.
    while (const char* entry = AAssetDir_getNextFileName(directory)) {
        names.emplace_back(entry);
    }
    AAssetDir_close(directory);
    return {};
}

FileError AndroidAssetFileSystem::make_directories(
    FileType type,
    std::string_view path
)
{
    if (type == FileType::Internal) return read_only(path);
    return storage_.make_directories(type, path);
}

FileError AndroidAssetFileSystem::remove(
    FileType type,
    std::string_view path
)
{
    if (type == FileType::Internal) return read_only(path);
    return storage_.remove(type, path);
}

}  // namespace sq::files
