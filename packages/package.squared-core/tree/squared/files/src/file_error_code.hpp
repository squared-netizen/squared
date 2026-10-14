#pragma once

#include <cstdint>

namespace sq::files
{

/** @brief Stable error categories produced by file system operations. */
enum class FileErrorCode : std::uint8_t {
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

} // namespace sq::files
