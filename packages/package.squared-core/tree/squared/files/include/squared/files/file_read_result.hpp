#pragma once

#include <squared/files/file_error.hpp>

#include <cstddef>
#include <vector>

namespace sq::files {

/** @brief Bytes read from one file, or the failure that prevented it. */
struct FileReadResult {
    /** @brief File contents; valid only when error is empty. */
    std::vector<std::byte> bytes;

    /** @brief Read failure, or a no-error FileError on success. */
    FileError error;

    /** @brief Return whether the read succeeded. */
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return !error;
    }
};

}  // namespace sq::files
