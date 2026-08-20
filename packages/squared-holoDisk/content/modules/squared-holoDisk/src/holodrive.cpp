#include "squared/holodisk/holodrive.hpp"

#define MINIZ_NO_ZLIB_APIS
#include "miniz.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <system_error>
#include <vector>

namespace squared::holodisk {
namespace {

namespace fs = std::filesystem;

struct Failure final : std::runtime_error {
    Failure(ErrorCode code, std::string message)
        : std::runtime_error(message), error{code, std::move(message)}
    {
    }

    Error error;
};

[[noreturn]] void fail(ErrorCode code, std::string message)
{
    throw Failure(code, std::move(message));
}

std::string zip_error(mz_zip_archive& archive, std::string_view operation)
{
    return std::string(operation) + ": " +
        mz_zip_get_error_string(mz_zip_get_last_error(&archive));
}

std::string normalize_archive_path(std::string_view input)
{
    if (input.empty() || input.front() == '/' || input.front() == '\\') {
        fail(ErrorCode::InvalidPath, "HoloDisk paths must be relative");
    }

    std::string result;
    std::string segment;
    const auto flush = [&]() {
        if (segment.empty() || segment == "." || segment == "..") {
            fail(ErrorCode::InvalidPath, "HoloDisk path contains an unsafe segment");
        }
        if (!result.empty()) {
            result.push_back('/');
        }
        result += segment;
        segment.clear();
    };

    for (char character : input) {
        if (character == '\\' || character == '\0') {
            fail(ErrorCode::InvalidPath, "HoloDisk paths must use portable separators");
        }
        if (character == '/') {
            flush();
        } else {
            segment.push_back(character);
        }
    }
    flush();
    return result;
}

std::string normalize_virtual_path(std::string_view input)
{
    if (input.empty() || input.front() != '/') {
        fail(ErrorCode::InvalidPath, "mounted paths must be absolute");
    }
    if (input == "/") {
        return "/";
    }
    auto relative = normalize_archive_path(input.substr(1));
    return "/" + relative;
}

struct ZipReader final {
    mz_zip_archive archive{};
    bool active{false};

    explicit ZipReader(const fs::path& path)
    {
        mz_zip_zero_struct(&archive);
        const auto text = path.string();
        if (!mz_zip_reader_init_file(&archive, text.c_str(), 0)) {
            fail(ErrorCode::InvalidArchive, zip_error(archive, "cannot open ZIP HoloDisk"));
        }
        active = true;
    }

    explicit ZipReader(std::span<const std::byte> bytes)
    {
        mz_zip_zero_struct(&archive);
        if (bytes.empty() || !mz_zip_reader_init_mem(
                &archive, bytes.data(), bytes.size(), 0)) {
            fail(ErrorCode::InvalidArchive,
                 zip_error(archive, "cannot open memory ZIP HoloDisk"));
        }
        active = true;
    }

    ~ZipReader()
    {
        if (active) {
            mz_zip_reader_end(&archive);
        }
    }

    ZipReader(const ZipReader&) = delete;
    ZipReader& operator=(const ZipReader&) = delete;
};

struct ArchiveEntry {
    mz_uint index{0};
    std::uint64_t size{0};
};

struct OverlayEntry {
    fs::path path;
    std::uint64_t size{0};
};

struct DiskState {
    fs::path source;
    std::shared_ptr<const std::vector<std::byte>> memory_source;
    std::map<std::string, ArchiveEntry> archive_entries;
    std::map<std::string, OverlayEntry> overlay_entries;
};

struct MountState {
    DiskId disk;
    std::string point;
    MountAccess access{MountAccess::ReadOnly};
};

struct FileState {
    MountId mount;
    DiskId disk;
    std::string path;
    bool readable{false};
    bool writable{false};
    std::uint64_t position{0};
    std::uint64_t size{0};
    std::fstream overlay;
    std::unique_ptr<ZipReader> reader;
    mz_zip_reader_extract_iter_state* iterator{nullptr};
    mz_uint archive_index{0};

    ~FileState()
    {
        if (iterator != nullptr) {
            mz_zip_reader_extract_iter_free(iterator);
        }
    }
};

class StandardHoloDrive final : public HoloDrive {
public:
    StandardHoloDrive(DriveOptions options, fs::path scratch)
        : options_(std::move(options)), scratch_(std::move(scratch))
    {
    }

    ~StandardHoloDrive() override
    {
        files_.clear();
        std::error_code ignored;
        fs::remove_all(scratch_, ignored);
    }

    Result<DiskId> create_holodisk() noexcept override
    {
        return guard<DiskId>([&] {
            ensure_capacity(disks_.size(), options_.maximum_disks, "HoloDisks");
            const DiskId id{next_disk_++};
            disks_.emplace(id.value, DiskState{});
            return id;
        });
    }

    Result<DiskId> load_holodisk(std::string_view location) noexcept override
    {
        return guard<DiskId>([&] {
            ensure_capacity(disks_.size(), options_.maximum_disks, "HoloDisks");
            if (location.empty()) {
                fail(ErrorCode::InvalidArgument, "HoloDisk location is empty");
            }
            std::error_code error;
            fs::path source = fs::absolute(fs::path(location), error);
            if (error || !fs::is_regular_file(source, error)) {
                fail(ErrorCode::NotFound, "HoloDisk file does not exist");
            }
            const auto compressed_size = fs::file_size(source, error);
            if (error) {
                fail(ErrorCode::Io, "cannot inspect HoloDisk file size");
            }
            if (compressed_size > options_.maximum_archive_size) {
                fail(ErrorCode::LimitExceeded, "HoloDisk archive size limit exceeded");
            }

            ZipReader reader(source);
            DiskState disk;
            disk.source = std::move(source);
            return inspect_archive(reader, std::move(disk));
        });
    }

    Result<DiskId> load_holodisk(
        std::span<const std::byte> archive
    ) noexcept override
    {
        return guard<DiskId>([&] {
            ensure_capacity(disks_.size(), options_.maximum_disks, "HoloDisks");
            if (archive.empty()) {
                fail(ErrorCode::InvalidArgument, "HoloDisk archive bytes are empty");
            }
            if (archive.size() > options_.maximum_archive_size) {
                fail(ErrorCode::LimitExceeded, "HoloDisk archive size limit exceeded");
            }
            auto storage = std::make_shared<std::vector<std::byte>>(
                archive.begin(), archive.end()
            );
            ZipReader reader{std::span<const std::byte>(*storage)};
            DiskState disk;
            disk.memory_source = std::move(storage);
            return inspect_archive(reader, std::move(disk));
        });
    }

    Status write_holodisk(
        DiskId disk_id,
        std::string_view destination,
        const WriteOptions& options
    ) noexcept override
    {
        return guard_status([&] {
            auto& disk = get_disk(disk_id);
            if (destination.empty() || options.compression_level < 0 ||
                options.compression_level > 9) {
                fail(ErrorCode::InvalidArgument, "invalid HoloDisk write options");
            }
            for (const auto& [_, file] : files_) {
                if (file->disk == disk_id) {
                    fail(ErrorCode::Busy, "close HoloDisk files before materializing");
                }
            }

            fs::path target(destination);
            std::error_code error;
            if (fs::exists(target, error) && !options.replace_existing) {
                fail(ErrorCode::AlreadyExists, "HoloDisk destination already exists");
            }
            const auto parent = target.has_parent_path() ? target.parent_path() : fs::current_path();
            if (!fs::is_directory(parent, error)) {
                fail(ErrorCode::NotFound, "HoloDisk destination directory does not exist");
            }
            fs::path temporary;
            do {
                temporary = parent /
                    (target.filename().string() + ".squared-tmp-" +
                     std::to_string(next_temp_++));
                error.clear();
            } while (fs::exists(temporary, error));
            if (error) {
                fail(ErrorCode::Io, "cannot inspect HoloDisk destination: " + error.message());
            }

            try {
                write_archive(disk, temporary, options.compression_level);
                install_archive(temporary, target, options.replace_existing);
            } catch (...) {
                fs::remove(temporary, error);
                throw;
            }
        });
    }

    Status discard_holodisk(DiskId disk) noexcept override
    {
        return guard_status([&] {
            get_disk(disk);
            for (const auto& [_, mount] : mounts_) {
                if (mount.disk == disk) {
                    fail(ErrorCode::Busy, "unmount HoloDisk before discarding it");
                }
            }
            disks_.erase(disk.value);
        });
    }

    Result<MountId> mount(
        DiskId disk,
        std::string_view mount_point,
        MountAccess access
    ) noexcept override
    {
        return guard<MountId>([&] {
            get_disk(disk);
            ensure_capacity(mounts_.size(), options_.maximum_mounts, "mounts");
            auto point = normalize_virtual_path(mount_point);
            for (const auto& [_, existing] : mounts_) {
                if (existing.point == point) {
                    fail(ErrorCode::AlreadyExists, "mount point is already in use");
                }
            }
            const MountId id{next_mount_++};
            mounts_.emplace(id.value, MountState{disk, std::move(point), access});
            return id;
        });
    }

    Status unmount(MountId mount) noexcept override
    {
        return guard_status([&] {
            get_mount(mount);
            for (const auto& [_, file] : files_) {
                if (file->mount == mount) {
                    fail(ErrorCode::Busy, "close mounted files before unmounting");
                }
            }
            mounts_.erase(mount.value);
        });
    }

    Result<FileId> open(std::string_view path, OpenMode mode) noexcept override
    {
        return guard<FileId>([&] {
            ensure_capacity(files_.size(), options_.maximum_open_files, "open files");
            const auto resolved = resolve(path);
            auto& mount_state = *resolved.mount;
            auto& disk = get_disk(mount_state.disk);
            if (resolved.relative.empty()) {
                fail(ErrorCode::InvalidPath, "cannot open a mount root as a file");
            }

            const bool writable = mode != OpenMode::Read;
            if (writable && mount_state.access != MountAccess::ReadWrite) {
                fail(ErrorCode::ReadOnly, "HoloDisk mount is read-only");
            }
            if (writable) {
                for (const auto& [_, existing] : files_) {
                    if (existing->disk == mount_state.disk &&
                        existing->path == resolved.relative && existing->writable) {
                        fail(ErrorCode::Busy, "HoloDisk path already has a writer");
                    }
                }
            }

            const auto overlay_it = disk.overlay_entries.find(resolved.relative);
            const auto archive_it = disk.archive_entries.find(resolved.relative);
            const bool exists = overlay_it != disk.overlay_entries.end() ||
                archive_it != disk.archive_entries.end();
            if (!exists && mode == OpenMode::Read) {
                fail(ErrorCode::NotFound, "HoloDisk file was not found");
            }

            auto file = std::make_unique<FileState>();
            file->mount = resolved.id;
            file->disk = mount_state.disk;
            file->path = resolved.relative;
            file->readable = mode == OpenMode::Read || mode == OpenMode::ReadWrite;
            file->writable = writable;

            if (writable) {
                prepare_overlay(disk, resolved.relative, mode);
                auto& overlay = disk.overlay_entries.at(resolved.relative);
                file->size = overlay.size;
                auto flags = std::ios::binary | std::ios::in | std::ios::out;
                file->overlay.open(overlay.path, flags);
                if (!file->overlay) {
                    fail(ErrorCode::Io, "cannot open HoloDisk scratch file");
                }
                file->position = mode == OpenMode::Append ? file->size : 0;
            } else if (overlay_it != disk.overlay_entries.end()) {
                file->size = overlay_it->second.size;
                file->overlay.open(overlay_it->second.path, std::ios::binary | std::ios::in);
                if (!file->overlay) {
                    fail(ErrorCode::Io, "cannot open HoloDisk scratch file");
                }
            } else {
                file->size = archive_it->second.size;
                file->archive_index = archive_it->second.index;
                initialize_archive_reader(*file, disk);
            }

            const FileId id{next_file_++};
            files_.emplace(id.value, std::move(file));
            return id;
        });
    }

    Result<std::size_t> read(
        FileId id,
        std::span<std::byte> destination
    ) noexcept override
    {
        return guard<std::size_t>([&] {
            auto& file = get_file(id);
            if (!file.readable) {
                fail(ErrorCode::InvalidArgument, "HoloDisk file is not readable");
            }
            const auto remaining = file.size - file.position;
            const auto count = static_cast<std::size_t>(std::min<std::uint64_t>(
                remaining, destination.size()
            ));
            if (count == 0) {
                return std::size_t{0};
            }
            if (file.overlay.is_open()) {
                file.overlay.clear();
                file.overlay.seekg(static_cast<std::streamoff>(file.position));
                file.overlay.read(reinterpret_cast<char*>(destination.data()),
                                  static_cast<std::streamsize>(count));
                const auto actual = static_cast<std::size_t>(file.overlay.gcount());
                file.position += actual;
                return actual;
            }
            const auto actual = mz_zip_reader_extract_iter_read(
                file.iterator, destination.data(), count
            );
            if (actual == 0 && file.position < file.size) {
                fail(ErrorCode::InvalidArchive, "cannot decompress HoloDisk file");
            }
            file.position += actual;
            return actual;
        });
    }

    Result<std::size_t> write(
        FileId id,
        std::span<const std::byte> source
    ) noexcept override
    {
        return guard<std::size_t>([&] {
            auto& file = get_file(id);
            if (!file.writable) {
                fail(ErrorCode::ReadOnly, "HoloDisk file is not writable");
            }
            if (source.size() > options_.maximum_file_size - file.position) {
                fail(ErrorCode::LimitExceeded, "HoloDisk file size limit exceeded");
            }
            const auto new_size = std::max<std::uint64_t>(
                file.size, file.position + source.size()
            );
            auto& disk = get_disk(file.disk);
            ensure_expanded_size(disk, file.path, new_size);
            file.overlay.clear();
            file.overlay.seekp(static_cast<std::streamoff>(file.position));
            file.overlay.write(reinterpret_cast<const char*>(source.data()),
                               static_cast<std::streamsize>(source.size()));
            file.overlay.flush();
            if (!file.overlay) {
                fail(ErrorCode::Io, "cannot write HoloDisk scratch file");
            }
            file.position += source.size();
            file.size = new_size;
            disk.overlay_entries.at(file.path).size = new_size;
            return source.size();
        });
    }

    Result<std::uint64_t> seek(
        FileId id,
        std::int64_t offset,
        SeekOrigin origin
    ) noexcept override
    {
        return guard<std::uint64_t>([&] {
            auto& file = get_file(id);
            const std::uint64_t base = origin == SeekOrigin::Begin ? 0 :
                origin == SeekOrigin::Current ? file.position : file.size;
            std::uint64_t position = 0;
            if (offset < 0) {
                const auto distance = static_cast<std::uint64_t>(-(offset + 1)) + 1;
                if (distance > base) {
                    fail(ErrorCode::InvalidArgument, "seek precedes HoloDisk file");
                }
                position = base - distance;
            } else {
                const auto distance = static_cast<std::uint64_t>(offset);
                if (distance > file.size - std::min(file.size, base)) {
                    fail(ErrorCode::InvalidArgument, "seek exceeds HoloDisk file");
                }
                position = base + distance;
            }
            if (position > file.size) {
                fail(ErrorCode::InvalidArgument, "seek exceeds HoloDisk file");
            }
            if (file.iterator != nullptr && position != file.position) {
                reset_archive_position(file, position);
            }
            file.position = position;
            return position;
        });
    }

    Status close(FileId file) noexcept override
    {
        return guard_status([&] {
            get_file(file);
            files_.erase(file.value);
        });
    }

    Result<std::vector<Entry>> list(std::string_view path) noexcept override
    {
        return guard<std::vector<Entry>>([&] {
            const auto resolved = resolve(path);
            auto& disk = get_disk(resolved.mount->disk);
            const std::string prefix = resolved.relative.empty() ? "" : resolved.relative + "/";
            std::map<std::string, Entry> children;
            const auto add = [&](const std::string& name, std::uint64_t size) {
                if (!name.starts_with(prefix) || name == resolved.relative) {
                    return;
                }
                const auto tail = name.substr(prefix.size());
                const auto slash = tail.find('/');
                if (slash == std::string::npos) {
                    children[tail] = Entry{tail, false, size};
                } else {
                    const auto child = tail.substr(0, slash);
                    children[child] = Entry{child, true, 0};
                }
            };
            for (const auto& [name, entry] : disk.archive_entries) {
                add(name, entry.size);
            }
            for (const auto& [name, entry] : disk.overlay_entries) {
                add(name, entry.size);
            }
            if (!resolved.relative.empty() && children.empty()) {
                fail(ErrorCode::NotFound, "HoloDisk directory was not found");
            }
            std::vector<Entry> result;
            result.reserve(children.size());
            for (auto& [_, entry] : children) {
                result.push_back(std::move(entry));
            }
            return result;
        });
    }

private:
    struct Resolved {
        MountId id;
        MountState* mount;
        std::string relative;
    };

    template<typename T, typename Function>
    static Result<T> guard(Function&& function) noexcept
    {
        try {
            return Result<T>::success(function());
        } catch (const Failure& failure) {
            return Result<T>::failure(failure.error);
        } catch (const std::exception& error) {
            return Result<T>::failure({ErrorCode::Io, error.what()});
        } catch (...) {
            return Result<T>::failure({ErrorCode::Io, "unknown HoloDrive failure"});
        }
    }

    template<typename Function>
    static Status guard_status(Function&& function) noexcept
    {
        try {
            function();
            return Status::success();
        } catch (const Failure& failure) {
            return Status::failure(failure.error);
        } catch (const std::exception& error) {
            return Status::failure({ErrorCode::Io, error.what()});
        } catch (...) {
            return Status::failure({ErrorCode::Io, "unknown HoloDrive failure"});
        }
    }

    static void ensure_capacity(std::size_t used, std::size_t limit, const char* resource)
    {
        if (used >= limit) {
            fail(ErrorCode::LimitExceeded, std::string("maximum ") + resource + " reached");
        }
    }

    DiskId inspect_archive(ZipReader& reader, DiskState disk)
    {
        std::uint64_t expanded = 0;
        const auto count = mz_zip_reader_get_num_files(&reader.archive);
        if (count > options_.maximum_entries_per_disk) {
            fail(ErrorCode::LimitExceeded, "HoloDisk entry limit exceeded");
        }
        for (mz_uint index = 0; index < count; ++index) {
            mz_zip_archive_file_stat stat{};
            if (!mz_zip_reader_file_stat(&reader.archive, index, &stat)) {
                fail(ErrorCode::InvalidArchive,
                     zip_error(reader.archive, "cannot inspect ZIP entry"));
            }
            if (!stat.m_is_supported || stat.m_is_encrypted) {
                fail(ErrorCode::UnsupportedArchive,
                     "HoloDisk contains an unsupported ZIP entry");
            }
            const auto filename_size = mz_zip_reader_get_filename(
                &reader.archive, index, nullptr, 0
            );
            if (filename_size == 0 || filename_size > 4097) {
                fail(ErrorCode::InvalidArchive,
                     "HoloDisk entry path is invalid or too long");
            }
            std::vector<char> filename(filename_size);
            if (mz_zip_reader_get_filename(
                    &reader.archive, index, filename.data(), filename_size
                ) != filename_size) {
                fail(ErrorCode::InvalidArchive, "cannot read HoloDisk entry path");
            }
            std::string raw(filename.data(), filename_size - 1);
            if (raw.empty()) {
                fail(ErrorCode::InvalidArchive, "HoloDisk contains an unnamed entry");
            }
            if (stat.m_is_directory) {
                if (raw.back() == '/') raw.pop_back();
                if (!raw.empty()) normalize_archive_path(raw);
                continue;
            }
            auto name = normalize_archive_path(raw);
            if (stat.m_uncomp_size > options_.maximum_file_size) {
                fail(ErrorCode::LimitExceeded, "HoloDisk file size limit exceeded");
            }
            if (stat.m_uncomp_size > options_.maximum_expanded_size ||
                expanded > options_.maximum_expanded_size - stat.m_uncomp_size) {
                fail(ErrorCode::LimitExceeded, "HoloDisk expanded size limit exceeded");
            }
            expanded += stat.m_uncomp_size;
            if (!disk.archive_entries.emplace(
                    std::move(name), ArchiveEntry{index, stat.m_uncomp_size}
                ).second) {
                fail(ErrorCode::InvalidArchive, "HoloDisk contains duplicate paths");
            }
        }

        const DiskId id{next_disk_++};
        disks_.emplace(id.value, std::move(disk));
        return id;
    }

    static std::unique_ptr<ZipReader> make_reader(const DiskState& disk)
    {
        if (disk.memory_source) {
            return std::make_unique<ZipReader>(
                std::span<const std::byte>(*disk.memory_source)
            );
        }
        return std::make_unique<ZipReader>(disk.source);
    }

    DiskState& get_disk(DiskId disk)
    {
        const auto found = disks_.find(disk.value);
        if (!disk || found == disks_.end()) {
            fail(ErrorCode::InvalidHandle, "invalid HoloDisk identity");
        }
        return found->second;
    }

    MountState& get_mount(MountId mount)
    {
        const auto found = mounts_.find(mount.value);
        if (!mount || found == mounts_.end()) {
            fail(ErrorCode::InvalidHandle, "invalid HoloDisk mount identity");
        }
        return found->second;
    }

    FileState& get_file(FileId file)
    {
        const auto found = files_.find(file.value);
        if (!file || found == files_.end()) {
            fail(ErrorCode::InvalidHandle, "invalid HoloDisk file identity");
        }
        return *found->second;
    }

    Resolved resolve(std::string_view input)
    {
        auto path = normalize_virtual_path(input);
        MountState* selected = nullptr;
        MountId selected_id;
        for (auto& [id, mount] : mounts_) {
            const bool matches = mount.point == "/" || path == mount.point ||
                (path.size() > mount.point.size() && path.starts_with(mount.point) &&
                 path[mount.point.size()] == '/');
            if (matches && (selected == nullptr || mount.point.size() > selected->point.size())) {
                selected = &mount;
                selected_id = MountId{id};
            }
        }
        if (selected == nullptr) {
            fail(ErrorCode::NotFound, "no HoloDisk is mounted for this path");
        }
        std::string relative;
        if (selected->point == "/") {
            relative = path == "/" ? "" : path.substr(1);
        } else if (path != selected->point) {
            relative = path.substr(selected->point.size() + 1);
        }
        return {selected_id, selected, std::move(relative)};
    }

    std::uint64_t expanded_size(const DiskState& disk, std::string_view replaced = {},
                                std::uint64_t replacement = 0) const
    {
        std::uint64_t total = 0;
        std::set<std::string> names;
        for (const auto& [name, entry] : disk.archive_entries) {
            names.insert(name);
            total += name == replaced ? replacement : entry.size;
        }
        for (const auto& [name, entry] : disk.overlay_entries) {
            if (names.contains(name)) {
                if (name != replaced) {
                    total -= disk.archive_entries.at(name).size;
                    total += entry.size;
                }
            } else {
                total += name == replaced ? replacement : entry.size;
            }
            names.insert(name);
        }
        if (!replaced.empty() && !names.contains(std::string(replaced))) {
            total += replacement;
        }
        return total;
    }

    void ensure_expanded_size(const DiskState& disk, std::string_view path,
                              std::uint64_t size) const
    {
        const auto total = expanded_size(disk, path, size);
        if (total > options_.maximum_expanded_size) {
            fail(ErrorCode::LimitExceeded, "HoloDisk expanded size limit exceeded");
        }
    }

    fs::path new_overlay_path()
    {
        return scratch_ / ("entry-" + std::to_string(next_temp_++));
    }

    void extract_archive_entry(const DiskState& disk, const ArchiveEntry& entry,
                               const fs::path& destination)
    {
        auto reader = make_reader(disk);
        const auto text = destination.string();
        if (!mz_zip_reader_extract_to_file(
                &reader->archive, entry.index, text.c_str(), 0)) {
            fail(ErrorCode::InvalidArchive,
                 zip_error(reader->archive, "cannot extract HoloDisk entry"));
        }
    }

    void prepare_overlay(DiskState& disk, const std::string& path, OpenMode mode)
    {
        auto found = disk.overlay_entries.find(path);
        if (found == disk.overlay_entries.end()) {
            const auto scratch = new_overlay_path();
            const auto archive = disk.archive_entries.find(path);
            if (archive == disk.archive_entries.end()) {
                std::size_t entries = disk.archive_entries.size();
                for (const auto& [name, _] : disk.overlay_entries) {
                    if (!disk.archive_entries.contains(name)) {
                        ++entries;
                    }
                }
                if (entries >= options_.maximum_entries_per_disk) {
                    fail(ErrorCode::LimitExceeded, "HoloDisk entry limit exceeded");
                }
            }
            if (archive != disk.archive_entries.end() && mode != OpenMode::WriteTruncate) {
                extract_archive_entry(disk, archive->second, scratch);
                found = disk.overlay_entries.emplace(
                    path, OverlayEntry{scratch, archive->second.size}
                ).first;
            } else {
                std::ofstream create(scratch, std::ios::binary | std::ios::trunc);
                if (!create) {
                    fail(ErrorCode::Io, "cannot create HoloDisk scratch file");
                }
                found = disk.overlay_entries.emplace(path, OverlayEntry{scratch, 0}).first;
            }
        }
        if (mode == OpenMode::WriteTruncate) {
            std::ofstream truncate(found->second.path, std::ios::binary | std::ios::trunc);
            if (!truncate) {
                fail(ErrorCode::Io, "cannot truncate HoloDisk scratch file");
            }
            found->second.size = 0;
        }
    }

    static void initialize_archive_reader(FileState& file, const DiskState& disk)
    {
        file.reader = make_reader(disk);
        file.iterator = mz_zip_reader_extract_iter_new(
            &file.reader->archive, file.archive_index, 0
        );
        if (file.iterator == nullptr) {
            fail(ErrorCode::InvalidArchive, "cannot stream HoloDisk entry");
        }
    }

    void reset_archive_position(FileState& file, std::uint64_t target)
    {
        mz_zip_reader_extract_iter_free(file.iterator);
        file.iterator = nullptr;
        file.reader.reset();
        initialize_archive_reader(file, get_disk(file.disk));
        std::array<std::byte, 8192> buffer{};
        std::uint64_t remaining = target;
        while (remaining != 0) {
            const auto request = static_cast<std::size_t>(std::min<std::uint64_t>(
                remaining, buffer.size()
            ));
            const auto actual = mz_zip_reader_extract_iter_read(
                file.iterator, buffer.data(), request
            );
            if (actual == 0) {
                fail(ErrorCode::InvalidArchive, "cannot seek in HoloDisk entry");
            }
            remaining -= actual;
        }
    }

    struct InputCallback {
        std::ifstream stream;
    };

    static size_t read_overlay(void* opaque, mz_uint64 offset, void* buffer, size_t count)
    {
        auto& input = *static_cast<InputCallback*>(opaque);
        input.stream.clear();
        input.stream.seekg(static_cast<std::streamoff>(offset));
        input.stream.read(static_cast<char*>(buffer), static_cast<std::streamsize>(count));
        return static_cast<size_t>(input.stream.gcount());
    }

    void write_archive(const DiskState& disk, const fs::path& destination, int compression)
    {
        mz_zip_archive writer{};
        mz_zip_zero_struct(&writer);
        const auto destination_text = destination.string();
        if (!mz_zip_writer_init_file(&writer, destination_text.c_str(), 0)) {
            fail(ErrorCode::Io, zip_error(writer, "cannot create ZIP HoloDisk"));
        }
        bool writer_active = true;
        try {
            std::unique_ptr<ZipReader> source;
            if (!disk.source.empty() || disk.memory_source) {
                source = make_reader(disk);
            }
            std::set<std::string> names;
            for (const auto& [name, _] : disk.archive_entries) {
                names.insert(name);
            }
            for (const auto& [name, _] : disk.overlay_entries) {
                names.insert(name);
            }
            for (const auto& name : names) {
                const auto overlay = disk.overlay_entries.find(name);
                if (overlay == disk.overlay_entries.end()) {
                    const auto& archived = disk.archive_entries.at(name);
                    if (!mz_zip_writer_add_from_zip_reader(
                            &writer, &source->archive, archived.index
                        )) {
                        fail(ErrorCode::Io, zip_error(writer, "cannot copy ZIP HoloDisk entry"));
                    }
                    continue;
                }
                InputCallback input{std::ifstream(overlay->second.path, std::ios::binary)};
                if (!input.stream) {
                    fail(ErrorCode::Io, "cannot read HoloDisk scratch file");
                }
                MZ_TIME_T timestamp{};
                if (!mz_zip_writer_add_read_buf_callback(
                        &writer, name.c_str(), &read_overlay, &input,
                        overlay->second.size, &timestamp, nullptr, 0,
                        static_cast<mz_uint>(compression), nullptr, 0, nullptr, 0
                    )) {
                    fail(ErrorCode::Io, zip_error(writer, "cannot write ZIP HoloDisk entry"));
                }
            }
            if (!mz_zip_writer_finalize_archive(&writer)) {
                fail(ErrorCode::Io, zip_error(writer, "cannot finalize ZIP HoloDisk"));
            }
            if (!mz_zip_writer_end(&writer)) {
                writer_active = false;
                fail(ErrorCode::Io, "cannot close ZIP HoloDisk");
            }
            writer_active = false;
        } catch (...) {
            if (writer_active) {
                mz_zip_writer_end(&writer);
            }
            throw;
        }
    }

    void install_archive(const fs::path& temporary, const fs::path& destination,
                         bool replace)
    {
        std::error_code error;
        if (!replace || !fs::exists(destination, error)) {
            fs::rename(temporary, destination, error);
            if (error) {
                fail(ErrorCode::Io, "cannot install materialized HoloDisk: " + error.message());
            }
            return;
        }

        const fs::path backup = destination.parent_path() /
            (destination.filename().string() + ".squared-backup-" +
             std::to_string(next_temp_++));
        fs::rename(destination, backup, error);
        if (error) {
            fail(ErrorCode::Io, "cannot replace HoloDisk destination: " + error.message());
        }
        fs::rename(temporary, destination, error);
        if (error) {
            std::error_code restore_error;
            fs::rename(backup, destination, restore_error);
            fail(ErrorCode::Io, "cannot install replacement HoloDisk: " + error.message());
        }
        fs::remove(backup, error);
    }

    DriveOptions options_;
    fs::path scratch_;
    std::map<std::uint64_t, DiskState> disks_;
    std::map<std::uint64_t, MountState> mounts_;
    std::map<std::uint64_t, std::unique_ptr<FileState>> files_;
    std::uint64_t next_disk_{1};
    std::uint64_t next_mount_{1};
    std::uint64_t next_file_{1};
    std::uint64_t next_temp_{1};
};

class StandardHoloDriveFactory final : public HoloDriveFactory {
public:
    Result<std::unique_ptr<HoloDrive>> create(
        const DriveOptions& options
    ) const noexcept override
    {
        try {
            if (options.scratch_directory.empty() || options.maximum_disks == 0 ||
                options.maximum_mounts == 0 || options.maximum_open_files == 0 ||
                options.maximum_entries_per_disk == 0 || options.maximum_file_size == 0 ||
                options.maximum_expanded_size == 0 || options.maximum_archive_size == 0) {
                fail(ErrorCode::InvalidArgument, "invalid HoloDrive resource policy");
            }
            std::error_code error;
            fs::create_directories(options.scratch_directory, error);
            if (error) {
                fail(ErrorCode::Io, "cannot create HoloDrive scratch root: " + error.message());
            }
            static std::atomic<std::uint64_t> sequence{1};
            fs::path scratch;
            do {
                scratch = fs::path(options.scratch_directory) /
                    ("squared-holodrive-" + std::to_string(sequence.fetch_add(1)));
            } while (fs::exists(scratch, error));
            if (!fs::create_directory(scratch, error) || error) {
                fail(ErrorCode::Io, "cannot create isolated HoloDrive scratch directory");
            }
            return Result<std::unique_ptr<HoloDrive>>::success(
                std::make_unique<StandardHoloDrive>(options, std::move(scratch))
            );
        } catch (const Failure& failure) {
            return Result<std::unique_ptr<HoloDrive>>::failure(failure.error);
        } catch (const std::exception& error) {
            return Result<std::unique_ptr<HoloDrive>>::failure({ErrorCode::Io, error.what()});
        } catch (...) {
            return Result<std::unique_ptr<HoloDrive>>::failure(
                {ErrorCode::Io, "unknown HoloDrive factory failure"}
            );
        }
    }
};

}  // namespace

std::unique_ptr<HoloDriveFactory> make_standard_holodrive_factory()
{
    return std::make_unique<StandardHoloDriveFactory>();
}

}  // namespace squared::holodisk
