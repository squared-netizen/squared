#pragma once

namespace sq::files {

/** @brief Stable error categories produced by file system operations. */
enum class FileErrorCode {
    None,
    NotFound,
    NotADirectory,
    IsADirectory,
    PermissionDenied,
    ReadOnly,
    TooLarge,
    AlreadyExists,
    InvalidPath,
    NotSupported,
    IoFailure
};

}  // namespace sq::files
