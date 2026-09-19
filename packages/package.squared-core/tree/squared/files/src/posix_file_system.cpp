#include <squared/files/posix_file_system.hpp>

#include <squared/files/file_error.hpp>
#include <squared/files/file_error_code.hpp>
#include <squared/files/file_read_result.hpp>
#include <squared/files/file_type.hpp>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

namespace sq::files {

namespace {

FileError make_error(FileErrorCode code, std::string message, std::string path)
{
    return FileError{
        .code = code,
        .message = std::move(message),
        .path = std::move(path)
    };
}

/** @brief Translate errno into a stable category. */
FileErrorCode code_from_errno(int value) noexcept
{
    switch (value) {
    case ENOENT: return FileErrorCode::NotFound;
    case ENOTDIR: return FileErrorCode::NotADirectory;
    case EISDIR: return FileErrorCode::IsADirectory;
    case EACCES:
    case EPERM: return FileErrorCode::PermissionDenied;
    case EROFS: return FileErrorCode::ReadOnly;
    case EEXIST: return FileErrorCode::AlreadyExists;
    case ENAMETOOLONG:
    case EINVAL: return FileErrorCode::InvalidPath;
    case EFBIG:
    case EOVERFLOW: return FileErrorCode::TooLarge;
    default: return FileErrorCode::IoFailure;
    }
}

FileError from_errno(int value, std::string path)
{
    return make_error(
        code_from_errno(value),
        std::strerror(value),
        std::move(path)
    );
}

/**
 * @brief Reject a relative path that would escape its root.
 *
 * A leading '/' or any '..' segment is refused rather than normalised. The
 * caller asked for something under a root; silently resolving it elsewhere
 * would be worse than failing.
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

PosixFileSystem::PosixFileSystem(PosixFileSystemRoots roots)
    : roots_(std::move(roots))
{
}

std::string PosixFileSystem::resolve_platform_path(
    FileType type,
    std::string_view path
) const
{
    if (type == FileType::Absolute) return std::string{path};
    if (escapes_root(path)) return {};

    const std::string* root = nullptr;
    switch (type) {
    case FileType::Internal: root = &roots_.internal_root; break;
    case FileType::Local: root = &roots_.local_root; break;
    case FileType::External: root = &roots_.external_root; break;
    case FileType::Absolute: break;
    }
    if (root == nullptr || root->empty()) return {};
    if (path.empty()) return *root;

    std::string resolved;
    resolved.reserve(root->size() + path.size() + 1);
    resolved.assign(*root);
    if (resolved.back() != '/') resolved.push_back('/');
    resolved.append(path);
    return resolved;
}

bool PosixFileSystem::exists(FileType type, std::string_view path) const noexcept
{
    const std::string resolved = resolve_platform_path(type, path);
    if (resolved.empty()) return false;
    struct stat status {};
    return ::stat(resolved.c_str(), &status) == 0;
}

bool PosixFileSystem::is_directory(
    FileType type,
    std::string_view path
) const noexcept
{
    const std::string resolved = resolve_platform_path(type, path);
    if (resolved.empty()) return false;
    struct stat status {};
    if (::stat(resolved.c_str(), &status) != 0) return false;
    return S_ISDIR(status.st_mode);
}

std::uint64_t PosixFileSystem::length(
    FileType type,
    std::string_view path
) const noexcept
{
    const std::string resolved = resolve_platform_path(type, path);
    if (resolved.empty()) return 0U;
    struct stat status {};
    if (::stat(resolved.c_str(), &status) != 0) return 0U;
    if (S_ISDIR(status.st_mode)) return 0U;
    return static_cast<std::uint64_t>(status.st_size);
}

FileReadResult PosixFileSystem::read(
    FileType type,
    std::string_view path
) const
{
    const std::string resolved = resolve_platform_path(type, path);
    if (resolved.empty()) {
        return FileReadResult{
            .bytes = {},
            .error = make_error(
                FileErrorCode::NotSupported,
                "no root configured for this file type, or path escapes it",
                std::string{path}
            )
        };
    }

    std::FILE* stream = std::fopen(resolved.c_str(), "rb");
    if (stream == nullptr) {
        return FileReadResult{.bytes = {}, .error = from_errno(errno, resolved)};
    }

    FileReadResult result;
    result.bytes.resize(static_cast<std::size_t>(length(type, path)));
    const std::size_t read_count = result.bytes.empty()
        ? 0U
        : std::fread(result.bytes.data(), 1, result.bytes.size(), stream);
    const bool failed = std::ferror(stream) != 0;
    static_cast<void>(std::fclose(stream));

    if (failed) {
        return FileReadResult{
            .bytes = {},
            .error = make_error(
                FileErrorCode::IoFailure, "read failed", resolved
            )
        };
    }

    // A file can shrink between the size query and the read.
    result.bytes.resize(read_count);
    return result;
}

FileError PosixFileSystem::write(
    FileType type,
    std::string_view path,
    std::span<const std::byte> bytes,
    bool append
)
{
    if (type == FileType::Internal) {
        return make_error(
            FileErrorCode::ReadOnly,
            "internal storage is read-only on every platform",
            std::string{path}
        );
    }

    const std::string resolved = resolve_platform_path(type, path);
    if (resolved.empty()) {
        return make_error(
            FileErrorCode::NotSupported,
            "no root configured for this file type, or path escapes it",
            std::string{path}
        );
    }

    std::FILE* stream = std::fopen(resolved.c_str(), append ? "ab" : "wb");
    if (stream == nullptr) return from_errno(errno, resolved);

    bool failed = false;
    if (!bytes.empty()) {
        failed = std::fwrite(bytes.data(), 1, bytes.size(), stream)
            != bytes.size();
    }
    if (std::fclose(stream) != 0) failed = true;

    if (failed) {
        return make_error(FileErrorCode::IoFailure, "write failed", resolved);
    }
    return {};
}

FileError PosixFileSystem::list(
    FileType type,
    std::string_view path,
    std::vector<std::string>& names
) const
{
    names.clear();

    const std::string resolved = resolve_platform_path(type, path);
    if (resolved.empty()) {
        return make_error(
            FileErrorCode::NotSupported,
            "no root configured for this file type, or path escapes it",
            std::string{path}
        );
    }

    DIR* directory = ::opendir(resolved.c_str());
    if (directory == nullptr) return from_errno(errno, resolved);

    while (const dirent* entry = ::readdir(directory)) {
        const std::string_view name{static_cast<const char*>(entry->d_name)};
        if (name == "." || name == "..") continue;
        names.emplace_back(name);
    }
    static_cast<void>(::closedir(directory));
    return {};
}

FileError PosixFileSystem::make_directories(
    FileType type,
    std::string_view path
)
{
    if (type == FileType::Internal) {
        return make_error(
            FileErrorCode::ReadOnly,
            "internal storage is read-only on every platform",
            std::string{path}
        );
    }

    const std::string resolved = resolve_platform_path(type, path);
    if (resolved.empty()) {
        return make_error(
            FileErrorCode::NotSupported,
            "no root configured for this file type, or path escapes it",
            std::string{path}
        );
    }

    std::string partial;
    partial.reserve(resolved.size());
    for (std::size_t index = 0; index <= resolved.size(); ++index) {
        const bool boundary = index == resolved.size() || resolved[index] == '/';
        if (!boundary) {
            partial.push_back(resolved[index]);
            continue;
        }
        if (partial.empty() || partial == "/") {
            partial.push_back('/');
            continue;
        }
        if (::mkdir(partial.c_str(), 0755) != 0 && errno != EEXIST) {
            return from_errno(errno, partial);
        }
        if (index < resolved.size()) partial.push_back('/');
    }
    return {};
}

FileError PosixFileSystem::remove(FileType type, std::string_view path)
{
    if (type == FileType::Internal) {
        return make_error(
            FileErrorCode::ReadOnly,
            "internal storage is read-only on every platform",
            std::string{path}
        );
    }

    const std::string resolved = resolve_platform_path(type, path);
    if (resolved.empty()) {
        return make_error(
            FileErrorCode::NotSupported,
            "no root configured for this file type, or path escapes it",
            std::string{path}
        );
    }

    if (is_directory(type, path)) {
        if (::rmdir(resolved.c_str()) != 0) return from_errno(errno, resolved);
        return {};
    }
    if (::unlink(resolved.c_str()) != 0) return from_errno(errno, resolved);
    return {};
}

}  // namespace sq::files
