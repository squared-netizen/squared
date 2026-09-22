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

/**
 * @brief Path of the build-time index inside the asset bundle.
 *
 * AAssetDir lists files only; subdirectories are invisible to it, so a nested
 * asset tree cannot be walked through the NDK. libGDX sidesteps this by
 * calling the Java AssetManager. squared has no Java, so packaging writes
 * every asset path into this file instead, and directory questions are
 * answered from it.
 *
 * It lives under a framework-owned directory so it cannot collide with an
 * asset the application ships, and so the application's own top level stays
 * the application's. See docs/developer/asset-index.md.
 */
constexpr const char* k_index_path = ".squared/index";

/** @brief The framework-owned directory holding the index, hidden from lists. */
constexpr const char* k_framework_directory = ".squared";

/**
 * @brief Read the index, one asset path per line.
 * @return The paths, or an empty vector when there is no index.
 *
 * Read per call rather than cached, on purpose: a large game's index is tens
 * of kilobytes that would otherwise stay resident for a question asked once
 * at startup. Anything that lists hot should cache its own answer.
 */
std::vector<std::string> read_index(AAssetManager* assets)
{
    std::vector<std::string> paths;
    AAsset* asset = AAssetManager_open(assets, k_index_path,
                                       AASSET_MODE_STREAMING);
    if (asset == nullptr) return paths;

    std::string text;
    char buffer[4096];
    while (true) {
        const int count = AAsset_read(asset, buffer, sizeof(buffer));
        if (count <= 0) break;
        text.append(buffer, static_cast<std::size_t>(count));
    }
    AAsset_close(asset);

    std::size_t start = 0;
    while (start < text.size()) {
        std::size_t end = text.find('\n', start);
        if (end == std::string::npos) end = text.size();
        std::string line = text.substr(start, end - start);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) paths.push_back(std::move(line));
        start = end + 1;
    }
    return paths;
}

/** @brief The directory prefix a path is tested against, with a trailing '/'. */
std::string directory_prefix(std::string_view path)
{
    std::string prefix{path};
    while (!prefix.empty() && prefix.back() == '/') prefix.pop_back();
    if (!prefix.empty()) prefix.push_back('/');
    return prefix;
}

/** @brief Whether a path names the root of the bundle. */
bool prefix_is_root(std::string_view path) noexcept
{
    while (!path.empty() && path.back() == '/') path.remove_suffix(1);
    return path.empty();
}

/** @brief Whether any indexed asset lives under this directory. */
bool index_has_directory(
    const std::vector<std::string>& index,
    std::string_view path
)
{
    const std::string prefix = directory_prefix(path);
    if (prefix.empty()) return !index.empty();   // the root
    for (const std::string& entry : index) {
        if (entry.size() > prefix.size()
            && entry.compare(0, prefix.size(), prefix) == 0) {
            return true;
        }
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

    return is_directory(type, path);
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

    // A directory is not an asset, so the question goes to the index. That is
    // what makes a directory holding only subdirectories - skins/ holding
    // default/ - report correctly. AAssetDir cannot see it at all.
    const std::vector<std::string> index = read_index(assets_);
    if (!index.empty()) return index_has_directory(index, path);

    // No index: an APK built without one. Fall back to AAssetDir, which gets
    // directories of files right and directories of directories wrong.
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

    const std::vector<std::string> index = read_index(assets_);
    if (!index.empty()) {
        if (!index_has_directory(index, path)) {
            return make_error(
                FileErrorCode::NotFound, "no such asset directory", path
            );
        }

        // Immediate children only: the next path segment after the prefix,
        // whether that is a file or the first component of a deeper path.
        const std::string prefix = directory_prefix(path);
        for (const std::string& entry : index) {
            if (entry.compare(0, prefix.size(), prefix) != 0) continue;
            const std::string_view rest =
                std::string_view{entry}.substr(prefix.size());
            if (rest.empty()) continue;
            const std::string child{rest.substr(0, rest.find('/'))};
            // The framework's own directory is not an application asset.
            if (prefix.empty() && child == k_framework_directory) continue;
            bool seen = false;
            for (const std::string& existing : names) {
                if (existing == child) { seen = true; break; }
            }
            if (!seen) names.push_back(child);
        }
        return {};
    }

    // No index: list files through AAssetDir, which cannot see
    // subdirectories. Degraded rather than broken, and exactly what this
    // backend did before the index existed.
    const std::string name{path};
    AAssetDir* directory = AAssetManager_openDir(assets_, name.c_str());
    if (directory == nullptr) {
        return make_error(
            FileErrorCode::NotFound, "no such asset directory", path
        );
    }
    while (const char* entry = AAssetDir_getNextFileName(directory)) {
        if (prefix_is_root(path)
            && std::string_view{entry} == k_framework_directory) {
            continue;
        }
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
