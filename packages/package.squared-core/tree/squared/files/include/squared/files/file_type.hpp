#pragma once

namespace sq::files {

/**
 * @brief Where a path is rooted.
 *
 * The same relative path means a different file under each type, and the
 * platform decides what each root is. Android internal is the read-only asset
 * bundle inside the APK, which is not a POSIX directory; on a desktop or in
 * Termux it is a directory beside the executable. Code above the file system
 * names a type and a relative path and never learns which it got.
 */
enum class FileType {
    /** @brief Read-only files shipped with the application. */
    Internal,

    /** @brief Private writable storage owned by this application. */
    Local,

    /** @brief Shared writable storage outside the application's own space. */
    External,

    /** @brief The path exactly as given, resolved by the platform. */
    Absolute
};

}  // namespace sq::files
