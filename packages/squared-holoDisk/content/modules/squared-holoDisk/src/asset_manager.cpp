#include "squared/holodisk/asset_manager.hpp"

#include <algorithm>
#include <array>
#include <ranges>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace squared::holodisk {
namespace {

struct AssetFailure final : std::runtime_error {
    explicit AssetFailure(Error error)
        : std::runtime_error(error.message), detail(std::move(error))
    {
    }
    Error detail;
};

[[noreturn]] void fail(ErrorCode code, std::string message)
{
    throw AssetFailure({code, std::move(message)});
}

std::string normalize_asset_path(std::string_view input)
{
    if (input.empty() || input.front() != '/') {
        fail(ErrorCode::InvalidPath, "asset paths must be absolute");
    }
    if (input == "/") return "/";

    std::string result{"/"};
    std::string segment;
    const auto flush = [&] {
        if (segment.empty() || segment == "." || segment == "..") {
            fail(ErrorCode::InvalidPath, "asset path contains an unsafe segment");
        }
        if (result.size() != 1) result.push_back('/');
        result += segment;
        segment.clear();
    };
    for (const char character : input.substr(1)) {
        if (character == '\\' || character == '\0') {
            fail(ErrorCode::InvalidPath, "asset paths must use portable separators");
        }
        if (character == '/') flush();
        else segment.push_back(character);
    }
    flush();
    return result;
}

bool beneath(std::string_view path, std::string_view mount_point) noexcept
{
    return mount_point == "/" || path == mount_point ||
        (path.size() > mount_point.size() && path.starts_with(mount_point) &&
         path[mount_point.size()] == '/');
}

} // namespace

class AssetManager::Implementation final {
public:
    struct AssetKey final {
        std::type_index type;
        std::string path;
        friend bool operator==(const AssetKey&, const AssetKey&) = default;
    };

    struct AssetKeyHash final {
        std::size_t operator()(const AssetKey& key) const noexcept
        {
            const auto first = key.type.hash_code();
            const auto second = std::hash<std::string>{}(key.path);
            return first ^ (second + 0x9e3779b9U + (first << 6U) + (first >> 2U));
        }
    };

    using KeySet = std::unordered_set<AssetKey, AssetKeyHash>;

    struct CacheEntry final {
        ErasedAsset asset;
        KeySet dependencies;
        KeySet dependents;
    };

    struct LoadFrame final {
        AssetKey key;
        KeySet dependencies;
    };

    struct ArchiveMount final {
        DiskId disk;
        MountId mount;
    };

    Implementation(HoloDrive& bound_drive, AssetManagerOptions policy)
        : drive(bound_drive), options(policy)
    {
    }

    HoloDrive& drive;
    AssetManagerOptions options;
    std::unordered_map<std::type_index, ErasedLoader> loaders;
    std::unordered_map<AssetKey, CacheEntry, AssetKeyHash> cache;
    std::vector<LoadFrame> frames;
    std::unordered_map<std::string, ArchiveMount> archive_mounts;
};

AssetManager::AssetManager(HoloDrive& drive, AssetManagerOptions options)
{
    if (options.maximum_cached_assets == 0 || options.maximum_asset_bytes == 0 ||
        options.maximum_dependency_depth == 0) {
        throw std::invalid_argument("AssetManager limits must be positive");
    }
    implementation_ = std::make_unique<Implementation>(drive, options);
}

AssetManager::~AssetManager()
{
    clear();
    while (!implementation_->archive_mounts.empty()) {
        const auto current = implementation_->archive_mounts.begin();
        static_cast<void>(implementation_->drive.unmount(current->second.mount));
        static_cast<void>(implementation_->drive.discard_holodisk(current->second.disk));
        implementation_->archive_mounts.erase(current);
    }
}

Status AssetManager::register_loader_erased(
    std::type_index type,
    ErasedLoader loader
)
{
    try {
        if (implementation_->loaders.contains(type)) {
            return Status::failure(
                {ErrorCode::AlreadyExists, "asset type already has a loader"}
            );
        }
        implementation_->loaders.emplace(type, std::move(loader));
        return Status::success();
    } catch (const std::exception& error) {
        return Status::failure({ErrorCode::LoadFailed, error.what()});
    }
}

Result<AssetManager::ErasedAsset> AssetManager::load_erased(
    std::type_index type,
    std::string_view path,
    bool force_reload
) noexcept
{
    using AssetKey = Implementation::AssetKey;
    using CacheEntry = Implementation::CacheEntry;
    try {
        AssetKey key{type, normalize_asset_path(path)};
        const auto active = std::ranges::find_if(
            implementation_->frames,
            [&key](const Implementation::LoadFrame& frame) {
                return frame.key == key;
            }
        );
        if (active != implementation_->frames.end()) {
            fail(ErrorCode::DependencyCycle,
                 "asset dependency cycle at " + key.path);
        }

        const auto record_in_parent = [&] {
            if (!implementation_->frames.empty()) {
                implementation_->frames.back().dependencies.insert(key);
            }
        };

        const auto cached = implementation_->cache.find(key);
        if (!force_reload && cached != implementation_->cache.end()) {
            record_in_parent();
            return Result<ErasedAsset>::success(cached->second.asset);
        }
        if (implementation_->frames.size() >=
            implementation_->options.maximum_dependency_depth) {
            fail(ErrorCode::LimitExceeded, "asset dependency depth limit exceeded");
        }
        const auto loader = implementation_->loaders.find(type);
        if (loader == implementation_->loaders.end()) {
            fail(ErrorCode::LoaderNotFound, "no loader is registered for asset type");
        }
        if (cached == implementation_->cache.end() &&
            implementation_->cache.size() >=
                implementation_->options.maximum_cached_assets) {
            fail(ErrorCode::LimitExceeded, "asset cache entry limit exceeded");
        }

        implementation_->frames.push_back({key, {}});
        AssetLoadContext context{*this};
        Result<ErasedAsset> loaded = Result<ErasedAsset>::failure(
            {ErrorCode::LoadFailed, "asset loader did not run"}
        );
        try {
            loaded = loader->second(context, key.path);
        } catch (const std::exception& error) {
            loaded = Result<ErasedAsset>::failure(
                {ErrorCode::LoadFailed, error.what()}
            );
        } catch (...) {
            loaded = Result<ErasedAsset>::failure(
                {ErrorCode::LoadFailed, "unknown asset loader failure"}
            );
        }
        auto frame = std::move(implementation_->frames.back());
        implementation_->frames.pop_back();
        if (!loaded) return loaded;
        if (!loaded.value()) {
            return Result<ErasedAsset>::failure(
                {ErrorCode::LoadFailed, "asset loader returned an empty handle"}
            );
        }

        Implementation::KeySet dependents;
        const auto existing = implementation_->cache.find(key);
        if (existing != implementation_->cache.end()) {
            dependents = std::move(existing->second.dependents);
            for (const auto& dependency : existing->second.dependencies) {
                const auto found = implementation_->cache.find(dependency);
                if (found != implementation_->cache.end()) {
                    found->second.dependents.erase(key);
                }
            }
        }
        CacheEntry replacement{
            loaded.value(), std::move(frame.dependencies), std::move(dependents)
        };
        implementation_->cache.insert_or_assign(key, std::move(replacement));
        const auto committed = implementation_->cache.find(key);
        for (const auto& dependency : committed->second.dependencies) {
            const auto found = implementation_->cache.find(dependency);
            if (found != implementation_->cache.end()) {
                found->second.dependents.insert(key);
            }
        }
        record_in_parent();
        return Result<ErasedAsset>::success(committed->second.asset);
    } catch (const AssetFailure& failure) {
        return Result<ErasedAsset>::failure(failure.detail);
    } catch (const std::exception& error) {
        return Result<ErasedAsset>::failure({ErrorCode::LoadFailed, error.what()});
    } catch (...) {
        return Result<ErasedAsset>::failure(
            {ErrorCode::LoadFailed, "unknown AssetManager failure"}
        );
    }
}

Status AssetManager::unload_erased(
    std::type_index type,
    std::string_view path
) noexcept
{
    try {
        Implementation::AssetKey key{type, normalize_asset_path(path)};
        const auto found = implementation_->cache.find(key);
        if (found == implementation_->cache.end()) {
            return Status::failure({ErrorCode::NotFound, "asset is not cached"});
        }
        if (!found->second.dependents.empty()) {
            return Status::failure(
                {ErrorCode::Busy, "cached assets still depend on this asset"}
            );
        }
        for (const auto& dependency : found->second.dependencies) {
            const auto loaded = implementation_->cache.find(dependency);
            if (loaded != implementation_->cache.end()) {
                loaded->second.dependents.erase(key);
            }
        }
        implementation_->cache.erase(found);
        return Status::success();
    } catch (const AssetFailure& failure) {
        return Status::failure(failure.detail);
    } catch (const std::exception& error) {
        return Status::failure({ErrorCode::LoadFailed, error.what()});
    }
}

bool AssetManager::contains_erased(
    std::type_index type,
    std::string_view path
) const noexcept
{
    try {
        return implementation_->cache.contains(
            Implementation::AssetKey{type, normalize_asset_path(path)}
        );
    } catch (...) {
        return false;
    }
}

Result<std::vector<std::byte>> AssetManager::read_bytes(
    std::string_view path
) noexcept
{
    try {
        const auto normalized = normalize_asset_path(path);
        auto opened = implementation_->drive.open(normalized, OpenMode::Read);
        if (!opened) return Result<std::vector<std::byte>>::failure(opened.error());
        const FileId file = opened.value();
        std::vector<std::byte> bytes;
        std::array<std::byte, 8192> buffer{};
        while (true) {
            auto read = implementation_->drive.read(file, buffer);
            if (!read) {
                static_cast<void>(implementation_->drive.close(file));
                return Result<std::vector<std::byte>>::failure(read.error());
            }
            if (read.value() == 0) break;
            if (read.value() > implementation_->options.maximum_asset_bytes -
                    std::min<std::uint64_t>(bytes.size(),
                        implementation_->options.maximum_asset_bytes)) {
                static_cast<void>(implementation_->drive.close(file));
                return Result<std::vector<std::byte>>::failure(
                    {ErrorCode::LimitExceeded, "asset byte limit exceeded"}
                );
            }
            bytes.insert(bytes.end(), buffer.begin(), buffer.begin() + read.value());
        }
        const auto closed = implementation_->drive.close(file);
        if (!closed) return Result<std::vector<std::byte>>::failure(closed.error());
        return Result<std::vector<std::byte>>::success(std::move(bytes));
    } catch (const AssetFailure& failure) {
        return Result<std::vector<std::byte>>::failure(failure.detail);
    } catch (const std::exception& error) {
        return Result<std::vector<std::byte>>::failure(
            {ErrorCode::LoadFailed, error.what()}
        );
    }
}

Result<std::vector<std::byte>> AssetLoadContext::read_bytes(
    std::string_view path
) noexcept
{
    return manager_->read_bytes(path);
}

void AssetManager::clear() noexcept
{
    implementation_->frames.clear();
    implementation_->cache.clear();
}

Status AssetManager::mount_archive(
    std::string_view archive_path,
    std::string_view mount_point
) noexcept
{
    try {
        const auto source = normalize_asset_path(archive_path);
        const auto point = normalize_asset_path(mount_point);
        if (implementation_->archive_mounts.contains(point)) {
            return Status::failure(
                {ErrorCode::AlreadyExists, "nested archive mount already exists"}
            );
        }
        auto bytes = read_bytes(source);
        if (!bytes) return Status::failure(bytes.error());
        auto disk = implementation_->drive.load_holodisk(
            std::span<const std::byte>(bytes.value())
        );
        if (!disk) return Status::failure(disk.error());
        auto mounted = implementation_->drive.mount(
            disk.value(), point, MountAccess::ReadOnly
        );
        if (!mounted) {
            static_cast<void>(implementation_->drive.discard_holodisk(disk.value()));
            return Status::failure(mounted.error());
        }
        try {
            implementation_->archive_mounts.emplace(
                point, Implementation::ArchiveMount{disk.value(), mounted.value()}
            );
        } catch (...) {
            static_cast<void>(implementation_->drive.unmount(mounted.value()));
            static_cast<void>(implementation_->drive.discard_holodisk(disk.value()));
            throw;
        }
        return Status::success();
    } catch (const AssetFailure& failure) {
        return Status::failure(failure.detail);
    } catch (const std::exception& error) {
        return Status::failure({ErrorCode::LoadFailed, error.what()});
    }
}

Status AssetManager::unmount_archive(std::string_view mount_point) noexcept
{
    try {
        const auto point = normalize_asset_path(mount_point);
        const auto found = implementation_->archive_mounts.find(point);
        if (found == implementation_->archive_mounts.end()) {
            return Status::failure(
                {ErrorCode::NotFound, "nested archive mount is not owned by manager"}
            );
        }
        const auto cached_beneath = std::ranges::find_if(
            implementation_->cache,
            [&point](const auto& item) { return beneath(item.first.path, point); }
        );
        if (cached_beneath != implementation_->cache.end()) {
            return Status::failure(
                {ErrorCode::Busy, "unload cached assets beneath nested mount first"}
            );
        }
        const auto unmounted = implementation_->drive.unmount(found->second.mount);
        if (!unmounted) return unmounted;
        const auto discarded = implementation_->drive.discard_holodisk(found->second.disk);
        implementation_->archive_mounts.erase(found);
        return discarded;
    } catch (const AssetFailure& failure) {
        return Status::failure(failure.detail);
    } catch (const std::exception& error) {
        return Status::failure({ErrorCode::LoadFailed, error.what()});
    }
}

} // namespace squared::holodisk
