#pragma once

#include <squared/files/file_error.hpp>
#include <squared/files/file_read_result.hpp>
#include <squared/files/file_text_result.hpp>
#include <squared/files/file_type.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sq::files {

class FileSystem;

/**
 * @brief A named location, modelled on libGDX's FileHandle.
 *
 * A handle is a value: a file system, a root type, and a relative path. It is
 * 48 bytes, it holds no descriptor, and constructing one touches nothing, so
 * building handles in a loop costs only the path string. Nothing is opened
 * until an operation is called, and nothing stays open after one returns.
 *
 * The file system is referenced, not owned, and must outlive every handle
 * resolved from it. Handles are freely copyable and comparable.
 *
 * @note Every failing operation returns its failure. None of them throw, and
 * none of them assert on a missing file &mdash; a missing file is an ordinary
 * result here, not a programmer error.
 */
class FileHandle final {
public:
    /** @brief Construct an invalid handle that answers no operation. */
    FileHandle() noexcept = default;

    /**
     * @brief Bind a handle to a file system, a root, and a relative path.
     * @param system File system that outlives this handle.
     * @param type Root the path is relative to.
     * @param path Relative path using '/' separators; empty means the root.
     * @note Prefer FileSystem::internal() and its siblings, which supply the
     * first two arguments.
     */
    FileHandle(
        FileSystem& system,
        FileType type,
        std::string path
    ) noexcept;

    /** @brief Report whether this handle is bound to a file system. */
    [[nodiscard]] bool valid() const noexcept { return system_ != nullptr; }

    /** @brief Return the root this handle's path is relative to. */
    [[nodiscard]] FileType type() const noexcept { return type_; }

    /** @brief Return the relative path, with '/' separators. */
    [[nodiscard]] const std::string& path() const noexcept { return path_; }

    /** @brief Return the final path segment, extension included. */
    [[nodiscard]] std::string_view name() const noexcept;

    /** @brief Return the extension without its dot, or empty when there is none. */
    [[nodiscard]] std::string_view extension() const noexcept;

    /** @brief Return the final path segment without its extension. */
    [[nodiscard]] std::string_view name_without_extension() const noexcept;

    /**
     * @brief Return a handle to an entry inside this directory.
     * @param name Relative path to append; '/' separators are accepted.
     * @return A handle under the same file system and root. No check is made
     * that this handle is a directory or that the child exists.
     */
    [[nodiscard]] FileHandle child(std::string_view name) const;

    /**
     * @brief Return a handle to an entry beside this one.
     * @param name Relative path to append to this handle's parent.
     * @return A handle under the same file system and root.
     */
    [[nodiscard]] FileHandle sibling(std::string_view name) const;

    /**
     * @brief Return a handle to the containing directory.
     * @return The parent handle, or a handle to the root when this is already
     * a top-level entry.
     */
    [[nodiscard]] FileHandle parent() const;

    /** @brief Report whether anything exists at this path. */
    [[nodiscard]] bool exists() const noexcept;

    /** @brief Report whether this path names a directory. */
    [[nodiscard]] bool is_directory() const noexcept;

    /**
     * @brief Return the file's size in bytes.
     * @return Size in bytes, or zero when the file is missing, unreadable, or
     * a directory. Use exists() to tell an empty file from a missing one.
     */
    [[nodiscard]] std::uint64_t length() const noexcept;

    /**
     * @brief Read the whole file.
     * @return The bytes, or a FileError describing why not.
     */
    [[nodiscard]] FileReadResult read_bytes() const;

    /**
     * @brief Read the whole file as text.
     * @return The contents, or a FileError describing why not. The bytes are
     * returned unvalidated and untranscoded.
     */
    [[nodiscard]] FileTextResult read_string() const;

    /**
     * @brief Write bytes to this path, creating the file if needed.
     * @param bytes Contents to write.
     * @param append Append rather than truncate.
     * @return A no-error FileError on success, otherwise the failure.
     * @note Missing parent directories are created first, as libGDX does.
     * @note Writing to FileType::Internal fails with ReadOnly on every
     * platform, so a build that works in Termux does not surprise you on a
     * device where assets live inside the APK.
     */
    [[nodiscard]] FileError write_bytes(
        std::span<const std::byte> bytes,
        bool append = false
    ) const;

    /**
     * @brief Write text to this path, creating the file if needed.
     * @param text Contents to write.
     * @param append Append rather than truncate.
     * @return A no-error FileError on success, otherwise the failure.
     */
    [[nodiscard]] FileError write_string(
        std::string_view text,
        bool append = false
    ) const;

    /**
     * @brief List the entries of this directory.
     * @param entries Cleared, then filled with one handle per entry, in no
     * guaranteed order. `.` and `..` are excluded.
     * @return A no-error FileError on success, otherwise the failure.
     * @note The caller owns the vector so it can be reused across calls,
     * which is why this is not a value-returning list().
     */
    [[nodiscard]] FileError list(std::vector<FileHandle>& entries) const;

    /**
     * @brief Create this directory and any missing parents.
     * @return A no-error FileError on success, including when the directory
     * already exists.
     */
    [[nodiscard]] FileError make_directories() const;

    /**
     * @brief Remove this file, or this directory when it is empty.
     * @return A no-error FileError on success, NotFound when nothing was
     * there.
     */
    [[nodiscard]] FileError remove() const;

    /** @brief Compare two handles by file system, root and path. */
    [[nodiscard]] bool operator==(const FileHandle& other) const noexcept;

private:
    FileSystem* system_{nullptr};
    FileType type_{FileType::Internal};
    std::string path_;
};

}  // namespace sq::files
