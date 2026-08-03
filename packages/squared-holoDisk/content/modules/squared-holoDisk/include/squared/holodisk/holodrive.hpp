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

/** @brief Stable failures returned by every HoloDrive implementation. */
enum class ErrorCode {
    None,
    InvalidArgument,
    InvalidPath,
    NotFound,
    AlreadyExists,
    LimitExceeded,
    InvalidArchive,
    UnsupportedArchive,
    ReadOnly,
    Busy,
    InvalidHandle,
    Io
};

/** @brief One dependency-free HoloDrive failure. */
struct Error {
    ErrorCode code{ErrorCode::None};
    std::string message;

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return code != ErrorCode::None;
    }
};

/** @brief Value-or-error result used by the standalone HoloDisk API. */
template<typename T>
class Result final {
public:
    [[nodiscard]] static Result success(T value)
    {
        Result result;
        result.value_.emplace(std::move(value));
        return result;
    }

    [[nodiscard]] static Result failure(Error error)
    {
        Result result;
        result.error_ = std::move(error);
        return result;
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return value_.has_value();
    }

    [[nodiscard]] T& value() &
    {
        return value_.value();
    }

    [[nodiscard]] const T& value() const&
    {
        return value_.value();
    }

    [[nodiscard]] T&& value() &&
    {
        return std::move(value_).value();
    }

    [[nodiscard]] const Error& error() const noexcept
    {
        return error_;
    }

private:
    std::optional<T> value_;
    Error error_;
};

/** @brief Success-or-error result for operations without a value. */
class Status final {
public:
    [[nodiscard]] static Status success() noexcept
    {
        return {};
    }

    [[nodiscard]] static Status failure(Error error)
    {
        Status result;
        result.error_ = std::move(error);
        return result;
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return !error_;
    }

    [[nodiscard]] const Error& error() const noexcept
    {
        return error_;
    }

private:
    Error error_;
};

/** @brief Opaque identity of one loaded or emulated ZIP HoloDisk. */
struct DiskId {
    std::uint64_t value{0};
    [[nodiscard]] explicit operator bool() const noexcept { return value != 0; }
    friend bool operator==(DiskId, DiskId) = default;
};

/** @brief Opaque identity of one mounted HoloDisk. */
struct MountId {
    std::uint64_t value{0};
    [[nodiscard]] explicit operator bool() const noexcept { return value != 0; }
    friend bool operator==(MountId, MountId) = default;
};

/** @brief Opaque identity of one file owned by a HoloDrive. */
struct FileId {
    std::uint64_t value{0};
    [[nodiscard]] explicit operator bool() const noexcept { return value != 0; }
    friend bool operator==(FileId, FileId) = default;
};

/** @brief Access granted to one mounted HoloDisk. */
enum class MountAccess {
    ReadOnly,
    ReadWrite
};

/** @brief Opening behavior for a path in the mounted drive namespace. */
enum class OpenMode {
    Read,
    WriteTruncate,
    ReadWrite,
    Append
};

/** @brief Reference point for a HoloDrive seek. */
enum class SeekOrigin {
    Begin,
    Current,
    End
};

/** @brief Resource and scratch policy fixed when a drive is created. */
struct DriveOptions {
    std::string scratch_directory;
    std::size_t maximum_disks{16};
    std::size_t maximum_mounts{32};
    std::size_t maximum_open_files{64};
    std::size_t maximum_entries_per_disk{4096};
    std::uint64_t maximum_file_size{64U * 1024U * 1024U};
    std::uint64_t maximum_expanded_size{512U * 1024U * 1024U};
};

/** @brief ZIP materialization controls. */
struct WriteOptions {
    int compression_level{6};
    bool replace_existing{false};
};

/** @brief One immediate child returned by HoloDrive::list. */
struct Entry {
    std::string name;
    bool directory{false};
    std::uint64_t size{0};
};

/**
 * @brief Sole operational boundary for loaded and emulated HoloDisks.
 *
 * HoloDisk ZIP images, mounts, and files remain owned by the drive. Clients
 * operate only on opaque identities and never receive a ZIP or host-file
 * object. Destruction discards every unmaterialized mutation.
 */
class HoloDrive {
public:
    virtual ~HoloDrive() = default;

    [[nodiscard]] virtual Result<DiskId> create_holodisk() noexcept = 0;
    [[nodiscard]] virtual Result<DiskId> load_holodisk(
        std::string_view location
    ) noexcept = 0;
    [[nodiscard]] virtual Status write_holodisk(
        DiskId disk,
        std::string_view destination,
        const WriteOptions& options = {}
    ) noexcept = 0;
    [[nodiscard]] virtual Status discard_holodisk(DiskId disk) noexcept = 0;

    [[nodiscard]] virtual Result<MountId> mount(
        DiskId disk,
        std::string_view mount_point,
        MountAccess access
    ) noexcept = 0;
    [[nodiscard]] virtual Status unmount(MountId mount) noexcept = 0;

    [[nodiscard]] virtual Result<FileId> open(
        std::string_view path,
        OpenMode mode
    ) noexcept = 0;
    [[nodiscard]] virtual Result<std::size_t> read(
        FileId file,
        std::span<std::byte> destination
    ) noexcept = 0;
    [[nodiscard]] virtual Result<std::size_t> write(
        FileId file,
        std::span<const std::byte> source
    ) noexcept = 0;
    [[nodiscard]] virtual Result<std::uint64_t> seek(
        FileId file,
        std::int64_t offset,
        SeekOrigin origin
    ) noexcept = 0;
    [[nodiscard]] virtual Status close(FileId file) noexcept = 0;

    [[nodiscard]] virtual Result<std::vector<Entry>> list(
        std::string_view path
    ) noexcept = 0;
};

/** @brief Optional extension interface used only to create a HoloDrive. */
class HoloDriveFactory {
public:
    virtual ~HoloDriveFactory() = default;

    [[nodiscard]] virtual Result<std::unique_ptr<HoloDrive>> create(
        const DriveOptions& options
    ) const noexcept = 0;
};

/**
 * @brief Create the standard ZIP and filesystem-backed factory.
 *
 * The returned implementation depends only on this module's vendored miniz
 * and the C++20 standard library.
 */
[[nodiscard]] std::unique_ptr<HoloDriveFactory>
make_standard_holodrive_factory();

}  // namespace squared::holodisk
