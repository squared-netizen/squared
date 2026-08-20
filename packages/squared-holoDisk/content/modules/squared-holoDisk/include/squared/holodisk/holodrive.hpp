#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace squared::holodisk {

/**
 * @brief Stable failures returned by every HoloDrive implementation.
 *
 * All HoloDrive operations report failures in this space through a returned
 * Result or Status rather than by throwing. Code values are stable across
 * releases so callers can switch on them. No Lua binding exists.
 */
enum class ErrorCode {
    None,               ///< No error; the operation succeeded.
    InvalidArgument,    ///< A supplied argument or option is outside its valid range.
    InvalidPath,        ///< A path failed archive or virtual namespace validation.
    NotFound,           ///< A disk, mount, file, directory, or host path was not found.
    AlreadyExists,      ///< A mount point is in use or a destination already exists.
    LimitExceeded,      ///< A configured disk, mount, open-file, entry, or size limit was hit.
    InvalidArchive,     ///< The ZIP HoloDisk is malformed, duplicated, or cannot be decompressed.
    UnsupportedArchive, ///< The ZIP contains an unsupported or encrypted entry.
    ReadOnly,           ///< The mount or file does not permit the requested write.
    Busy,               ///< An open file or mount prevents the requested operation.
    InvalidHandle,      ///< A DiskId, MountId, or FileId is zero or unknown to the drive.
    Io,                 ///< A host filesystem, scratch, or ZIP write/install failure.
    LoaderNotFound,     ///< No typed AssetManager loader is registered for the requested type.
    DependencyCycle,    ///< Typed asset dependencies form a cycle.
    LoadFailed          ///< A registered asset loader rejected or could not construct the asset.
};

/**
 * @brief One dependency-free HoloDrive failure.
 *
 * Value type describing a single failure. It owns nothing outside itself, so
 * it is safe to copy, move, return, or store. A default-constructed Error
 * reports success (code ErrorCode::None, empty message).
 */
struct Error {
    /** @brief Stable category of the failure. */
    ErrorCode code{ErrorCode::None};

    /** @brief Human-readable failure detail; empty when code is None. */
    std::string message;

    /**
     * @brief Return whether the error describes a real failure.
     * @return `true` when code is not ErrorCode::None.
     */
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return code != ErrorCode::None;
    }
};

/**
 * @brief Value-or-error result used by the standalone HoloDisk API.
 *
 * Owns either a single value or a single Error, never both. Prefer it over a
 * raw value plus error channel because it is `noexcept`-transparent and gives
 * the error stable ownership independent of the value type. The result is
 * value-semantic; when T is move-only, the result is move-only. No Lua
 * binding exists.
 * @tparam T Value type stored on success; moved or copied as supplied to
 * Result::success.
 */
template<typename T>
class Result final {
public:
    /**
     * @brief Construct a success result containing a value.
     * @param value Value to store; ownership transfers (is moved) into the
     * result.
     * @return Result in the success state; operator bool is `true`.
     */
    [[nodiscard]] static Result success(T value)
    {
        Result result;
        result.value_.emplace(std::move(value));
        return result;
    }

    /**
     * @brief Construct a failure result carrying an Error.
     * @param error Failure detail; ownership transfers (is moved) into the
     * result.
     * @return Result in the failure state; operator bool is `false`.
     */
    [[nodiscard]] static Result failure(Error error)
    {
        Result result;
        result.error_ = std::move(error);
        return result;
    }

    /**
     * @brief Return whether a value is present.
     * @return `true` when the result was built by Result::success and holds a
     * value; `false` when built by Result::failure.
     */
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return value_.has_value();
    }

    /**
     * @brief Access the stored value for modification.
     * @return Reference to the stored value.
     * @pre The result holds a value (operator bool is `true`).
     * @throws std::bad_optional_access when the result holds only an Error.
     * @note Valid until the Result is destroyed or reassigned.
     */
    [[nodiscard]] T& value() &
    {
        return value_.value();
    }

    /**
     * @brief Access the stored value as a read-only reference.
     * @return Const reference to the stored value.
     * @pre The result holds a value (operator bool is `true`).
     * @throws std::bad_optional_access when the result holds only an Error.
     * @note Valid until the Result is destroyed or reassigned.
     */
    [[nodiscard]] const T& value() const&
    {
        return value_.value();
    }

    /**
     * @brief Access the stored value for moves out of a temporary result.
     * @return Rvalue reference to the stored value; move from it.
     * @pre The result holds a value (operator bool is `true`).
     * @throws std::bad_optional_access when the result holds only an Error.
     * @note Use on deferenced temporaries such as `std::move(result).value()`
     * to transfer ownership (for example the drive out of
     * Result<std::unique_ptr<HoloDrive>>).
     */
    [[nodiscard]] T&& value() &&
    {
        return std::move(value_).value();
    }

    /**
     * @brief Obtain the stored failure detail.
     * @return Const reference to the stored Error. A success result returns an
     * Error with code ErrorCode::None; inspect it only when operator bool is
     * `false`.
     * @note Valid until the Result is destroyed or reassigned.
     */
    [[nodiscard]] const Error& error() const noexcept
    {
        return error_;
    }

private:
    std::optional<T> value_;
    Error error_;
};

/**
 * @brief Success-or-error result for operations without a value.
 *
 * Equivalent to Result<void>: reports success or a single Error. Statistically
 * sized for the no-value HoloDrive operations (mounting, unmounting,
 * materializing, discarding, closing). No Lua binding exists.
 */
class Status final {
public:
    /**
     * @brief Construct a success Status.
     * @return Status in the success state; operator bool is `true`.
     */
    [[nodiscard]] static Status success() noexcept
    {
        return {};
    }

    /**
     * @brief Construct a failure Status carrying an Error.
     * @param error Failure detail; ownership transfers (is moved) into the
     * Status.
     * @return Status in the failure state; operator bool is `false`.
     */
    [[nodiscard]] static Status failure(Error error)
    {
        Status result;
        result.error_ = std::move(error);
        return result;
    }

    /**
     * @brief Return whether the operation succeeded.
     * @return `true` when no Error was recorded.
     */
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return !error_;
    }

    /**
     * @brief Obtain the recorded failure detail.
     * @return Const reference to the stored Error. A success Status returns an
     * Error with code ErrorCode::None; inspect it only when operator bool is
     * `false`.
     * @note Valid until the Status is destroyed or reassigned.
     */
    [[nodiscard]] const Error& error() const noexcept
    {
        return error_;
    }

private:
    Error error_;
};

/**
 * @brief Opaque identity of one loaded or emulated ZIP HoloDisk.
 *
 * Issued by HoloDrive::create_holodisk() and HoloDrive::load_holodisk().
 * Valid only for the drive that issued it and while that disk has not been
 * discarded or the drive destroyed. Zero never matches a live disk.
 */
struct DiskId {
    /** @brief Monotonic per-drive non-zero value; zero means invalid/empty. */
    std::uint64_t value{0};
    /** @brief Return whether the handle is valid (non-zero). */
    [[nodiscard]] explicit operator bool() const noexcept { return value != 0; }
    /** @brief Compare two handles by value (member == and !=). */
    friend bool operator==(DiskId, DiskId) = default;
};

/**
 * @brief Opaque identity of one mounted HoloDisk.
 *
 * Issued by HoloDrive::mount(). Valid only for the drive that issued it and
 * while that mount has not been unmounted or the drive destroyed. Zero never
 * matches a live mount.
 */
struct MountId {
    /** @brief Monotonic per-drive non-zero value; zero means invalid/empty. */
    std::uint64_t value{0};
    /** @brief Return whether the handle is valid (non-zero). */
    [[nodiscard]] explicit operator bool() const noexcept { return value != 0; }
    /** @brief Compare two handles by value (member == and !=). */
    friend bool operator==(MountId, MountId) = default;
};

/**
 * @brief Opaque identity of one file owned by a HoloDrive.
 *
 * Issued by HoloDrive::open(). Valid only for the drive that issued it and
 * while the file has not been closed or the drive destroyed. Zero never
 * matches a live file.
 */
struct FileId {
    /** @brief Monotonic per-drive non-zero value; zero means invalid/empty. */
    std::uint64_t value{0};
    /** @brief Return whether the handle is valid (non-zero). */
    [[nodiscard]] explicit operator bool() const noexcept { return value != 0; }
    /** @brief Compare two handles by value (member == and !=). */
    friend bool operator==(FileId, FileId) = default;
};

/**
 * @brief Access granted to one mounted HoloDisk.
 *
 * Passed to HoloDrive::mount() and fixed for the life of the mount.
 */
enum class MountAccess {
    ReadOnly,  ///< Files may be opened for reading only; writable modes fail with ErrorCode::ReadOnly.
    ReadWrite  ///< Files may be opened with any OpenMode.
};

/**
 * @brief Opening behavior for a path in the mounted drive namespace.
 *
 * Passed to HoloDrive::open(). Writable modes route mutations through the
 * drive-owned scratch overlay; the ZIP image is not modified until
 * HoloDrive::write_holodisk().
 */
enum class OpenMode {
    Read,          ///< Sequential read; fails with ErrorCode::NotFound when the path is absent.
    WriteTruncate, ///< Write-only; clears existing content before the first write.
    ReadWrite,     ///< Read and write; creates the file when absent and preserves existing bytes otherwise.
    Append         ///< Write-only; positions at the end of the file for appended writes.
};

/**
 * @brief Reference point for a HoloDrive seek.
 *
 * Passed to HoloDrive::seek() to interpret the signed byte offset.
 */
enum class SeekOrigin {
    Begin,   ///< Relative to the start of the file (absolute position 0).
    Current, ///< Relative to the current file position.
    End      ///< Relative to the end (logical size) of the file.
};

/**
 * @brief Resource and scratch policy fixed when a drive is created.
 *
 * Governs how many opaque resources one drive may own and how large logical
 * disk data may grow. The factory rejects a policy whose scratch_directory is
 * empty or containing any zero limit with ErrorCode::InvalidArgument, then
 * creates the scratch directory tree under scratch_directory.
 */
struct DriveOptions {
    /** @brief Host directory for scratch-backed mutations and isolation; must
     *  be non-empty and is created when missing. */
    std::string scratch_directory;
    /** @brief Maximum simultaneously live disks (loaded or emulated); range
     *  [1, UINT64_MAX], default 16. */
    std::size_t maximum_disks{16};
    /** @brief Maximum simultaneously live mounts; range [1, UINT64_MAX],
     *  default 32. */
    std::size_t maximum_mounts{32};
    /** @brief Maximum concurrently open file handles; range [1, UINT64_MAX],
     *  default 64. */
    std::size_t maximum_open_files{64};
    /** @brief Maximum directory entries per disk, counting archived entries
     *  plus files created by writable opens; range [1, UINT64_MAX], default
     *  4096. */
    std::size_t maximum_entries_per_disk{4096};
    /** @brief Maximum uncompressed size in bytes of a single file entry; range
     *  [1, UINT64_MAX], default 64 MiB (67,108,864). */
    std::uint64_t maximum_file_size{64U * 1024U * 1024U};
    /** @brief Maximum total uncompressed size in bytes of all entries of one
     *  disk; range [1, UINT64_MAX], default 512 MiB (536,870,912). */
    std::uint64_t maximum_expanded_size{512U * 1024U * 1024U};
    /** @brief Maximum compressed size in bytes of an archive loaded from
     *  memory; range [1, UINT64_MAX], default 256 MiB (268,435,456). */
    std::uint64_t maximum_archive_size{256U * 1024U * 1024U};
};

/**
 * @brief ZIP materialization controls passed to HoloDrive::write_holodisk.
 */
struct WriteOptions {
    /** @brief Deflate compression level; range 0..9 (0 = store, 9 = best
     *  ratio), default 6. Any other value fails with
     *  ErrorCode::InvalidArgument. */
    int compression_level{6};
    /** @brief Whether an existing destination file may be replaced; default
     *  false. When false, an existing destination fails with
     *  ErrorCode::AlreadyExists. */
    bool replace_existing{false};
};

/**
 * @brief One immediate child returned by HoloDrive::list.
 */
struct Entry {
    /** @brief Child name; a single path segment that never contains '/' or
     *  '\\'. */
    std::string name;
    /** @brief Whether the child is a directory (its size is then 0). */
    bool directory{false};
    /** @brief Current logical size in bytes; 0 for directories and for files
     *  created but not yet written. */
    std::uint64_t size{0};
};

/**
 * @brief Sole operational boundary for loaded and emulated HoloDisks.
 *
 * HoloDisk ZIP images, mounts, and files remain owned by the drive. Clients
 * operate only on opaque identities and never receive a ZIP or host-file
 * object. Destruction discards every unmaterialized mutation.
 *
 * Every member returns a Result or Status that carries each failure; the
 * noexcept members never propagate exceptions. A drive is not thread-safe:
 * confine each drive to one thread or guard all of its calls with a single
 * external lock.
 */
class HoloDrive {
public:
    /** @brief Destroy the drive and release every disk, mount, file, and
     *  scratch-backed mutation it owns. */
    virtual ~HoloDrive() = default;

    /**
     * @brief Create an empty emulated HoloDisk in the drive.
     * @return Result holding a new unique DiskId on success, or a failure.
     * @note Fails with ErrorCode::LimitExceeded when maximum_disks live disks
     * are already present. The emulated disk owns no host file until
     * write_holodisk() materializes it.
     */
    [[nodiscard]] virtual Result<DiskId> create_holodisk() noexcept = 0;
    /**
     * @brief Load an existing ZIP HoloDisk from a host file path.
     * @param location Host filesystem path to a ZIP archive; relative paths
     * resolve against the current working directory. Must not be empty.
     * @return Result holding a new unique DiskId on success, or a failure.
     * @note Validates the archive during the call: NotFound for a missing
     * host file, InvalidArchive for a malformed or duplicated ZIP,
     * UnsupportedArchive for encrypted or unsupported entries, and
     * LimitExceeded for entry or expanded-size violations. Payload bytes are
     * not copied into RAM.
     */
    [[nodiscard]] virtual Result<DiskId> load_holodisk(
        std::string_view location
    ) noexcept = 0;
    /**
     * @brief Load a ZIP HoloDisk from caller-owned memory.
     * @param archive Complete ZIP bytes; copied during the call and bounded by
     * DriveOptions::maximum_archive_size.
     * @return Result holding a new DiskId, or a validation/resource failure.
     * @note The drive owns its copy after success. This overload enables
     * nested pinned archives to be mounted without extracting them into a
     * package directory.
     */
    [[nodiscard]] virtual Result<DiskId> load_holodisk(
        std::span<const std::byte> archive
    ) noexcept = 0;
    /**
     * @brief Materialize the current state of one disk to a ZIP HoloDisk.
     * @param disk DiskId from create_holodisk() or load_holodisk().
     * @param destination Host path for the new ZIP; must not be empty and its
     * parent directory must already exist.
     * @param options Materialization controls (compression level and whether
     * an existing destination may be replaced).
     * @return Status reporting success or one failure.
     * @note Failures: InvalidHandle for an unknown disk, Busy while any file
     * of the disk is open, InvalidArgument for an empty destination or an
     * out-of-range compression level, AlreadyExists when the destination
     * exists and replace_existing is false, NotFound when the destination
     * parent directory is missing, and Io for write or install errors. The
     * ZIP is written to a temporary file beside the destination and installed
     * only after finalization succeeds.
     */
    [[nodiscard]] virtual Status write_holodisk(
        DiskId disk,
        std::string_view destination,
        const WriteOptions& options = {}
    ) noexcept = 0;
    /**
     * @brief Remove a disk from the drive, forgetting all of its state.
     * @param disk DiskId to discard.
     * @return Status reporting success or one failure.
     * @note Failures: InvalidHandle for an unknown disk, Busy while the disk
     * is still mounted. Unmaterialized mutations of the disk are lost.
     */
    [[nodiscard]] virtual Status discard_holodisk(DiskId disk) noexcept = 0;

    /**
     * @brief Mount a disk into the drive's single virtual path namespace.
     * @param disk DiskId of the disk to mount.
     * @param mount_point Absolute virtual path such as "/cartridge"; the
     * namespace root "/" matches every path. Must not be empty.
     * @param access ReadOnly or ReadWrite permission granted to the mount.
     * @return Result holding a new unique MountId on success, or a failure.
     * @note Failures: InvalidHandle for an unknown disk, InvalidPath for a
     * non-absolute mount point, AlreadyExists when the mount point is already
     * in use, and LimitExceeded when maximum_mounts live mounts are present.
     * The longest matching mount point wins for later path resolution.
     */
    [[nodiscard]] virtual Result<MountId> mount(
        DiskId disk,
        std::string_view mount_point,
        MountAccess access
    ) noexcept = 0;
    /**
     * @brief Remove one mount from the drive.
     * @param mount MountId returned by HoloDrive::mount().
     * @return Status reporting success or one failure.
     * @note Failures: InvalidHandle for an unknown mount, Busy while files of
     * the mount remain open.
     */
    [[nodiscard]] virtual Status unmount(MountId mount) noexcept = 0;

    /**
     * @brief Open a file in the mounted namespace for reading or writing.
     * @param path Absolute virtual path beneath a mounted disk; opened as a
     * file, so the namespace root itself is rejected.
     * @param mode Whether to read, overwrite, read-write, or append.
     * @return Result holding a new unique FileId on success, or a failure.
     * @note Writable modes require a ReadWrite mount (ReadOnly otherwise), and
     * only one writer may exist per path (Busy otherwise). Reading requires
     * the path to exist (NotFound otherwise). Other failures: NotFound when no
     * mount covers the path, InvalidPath for unsafe paths or for the namespace
     * root itself, and LimitExceeded when maximum_open_files live handles are
     * present. The returned FileId must be released with HoloDrive::close().
     */
    [[nodiscard]] virtual Result<FileId> open(
        std::string_view path,
        OpenMode mode
    ) noexcept = 0;
    /**
     * @brief Read up to the destination capacity at the current file position.
     * @param file FileId returned by HoloDrive::open().
     * @param destination Output buffer receiving the bytes; its size bounds
     * the transfer.
     * @return Result holding the number of bytes read (0 at the end of the
     * file), or a failure.
     * @note Advances the file position by the returned count. Failures:
     * InvalidHandle for an unknown or closed file, InvalidArgument for a
     * write-only handle, InvalidArchive when a compressed entry cannot be
     * decompressed. Never throws.
     */
    [[nodiscard]] virtual Result<std::size_t> read(
        FileId file,
        std::span<std::byte> destination
    ) noexcept = 0;
    /**
     * @brief Write source bytes at the current position through the scratch
     * overlay.
     * @param file FileId returned by HoloDrive::open() with a writable mode.
     * @param source Bytes to write; not retained. Its size bounds the transfer.
     * @return Result holding the number of bytes written, or a failure. The
     * entire source is written or the operation fails before advancing.
     * @note Advances the position and grows the logical file size. Failures:
     * InvalidHandle for an unknown or closed file, ReadOnly for an unwritable
     * handle, LimitExceeded when maximum_file_size or maximum_expanded_size
     * would be exceeded, and Io for scratch storage errors. Content reaches a
     * ZIP only through write_holodisk().
     */
    [[nodiscard]] virtual Result<std::size_t> write(
        FileId file,
        std::span<const std::byte> source
    ) noexcept = 0;
    /**
     * @brief Change the file position relative to an origin.
     * @param file FileId returned by HoloDrive::open().
     * @param offset Signed byte offset from origin; may be negative.
     * @param origin Begin, Current, or End reference point.
     * @return Result holding the resulting absolute position (0-based byte
     * offset from the start of the file), or a failure.
     * @note The resulting position must lie within [0, size]; otherwise
     * ErrorCode::InvalidArgument. Failures also include InvalidHandle for an
     * unknown or closed file.
     */
    [[nodiscard]] virtual Result<std::uint64_t> seek(
        FileId file,
        std::int64_t offset,
        SeekOrigin origin
    ) noexcept = 0;
    /**
     * @brief Release one open file handle.
     * @param file FileId returned by HoloDrive::open().
     * @return Status reporting success or one failure.
     * @note Failures: InvalidHandle for an unknown or already-closed file.
     * Closing is required before the containing disk can be unmounted,
     * discarded, or materialized without ErrorCode::Busy.
     */
    [[nodiscard]] virtual Status close(FileId file) noexcept = 0;

    /**
     * @brief List the immediate children of a virtual directory.
     * @param path Absolute virtual path of an existing directory; the
     * namespace root "/" is valid.
     * @return Result holding the children sorted by name, or a failure. Each
     * Entry carries a single-segment name, a directory flag, and a size in
     * bytes (0 for directories).
     * @note Failures: NotFound when no mount covers the path or the directory
     * does not exist, InvalidPath for unsafe paths.
     */
    [[nodiscard]] virtual Result<std::vector<Entry>> list(
        std::string_view path
    ) noexcept = 0;
};

/**
 * @brief Optional extension interface used only to create a HoloDrive.
 *
 * Kept separate so application code can swap the drive backend without
 * touching the operational HoloDrive interface.
 */
class HoloDriveFactory {
public:
    /** @brief Destroy the factory. */
    virtual ~HoloDriveFactory() = default;

    /**
     * @brief Create a HoloDrive implementing the standard ZIP and
     * filesystem-backed backend.
     * @param options Non-owning policy that the drive copies; consulted only
     * during this call.
     * @return Result holding the new drive under unique ownership on success,
     * or a failure.
     * @note Failures: InvalidArgument when scratch_directory is empty or any
     * DriveOptions limit is zero; Io when the scratch directory tree cannot be
     * created. The returned drive exclusively owns all subsequent disks,
     * mounts, files, and scratch state until destroyed. A drive is not
     * thread-safe; confine each drive to one thread or guard its calls with a
     * single external lock.
     */
    [[nodiscard]] virtual Result<std::unique_ptr<HoloDrive>> create(
        const DriveOptions& options
    ) const noexcept = 0;
};

/**
 * @brief Create the standard ZIP and filesystem-backed factory.
 *
 * The returned implementation depends only on this module's vendored miniz
 * and the C++20 standard library.
 * @return New HoloDriveFactory under unique ownership; never null on success.
 * @note Pass a valid DriveOptions to HoloDriveFactory::create() to obtain a
 * drive. No Lua binding exists.
 */
[[nodiscard]] std::unique_ptr<HoloDriveFactory>
make_standard_holodrive_factory();

}  // namespace squared::holodisk
