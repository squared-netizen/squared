#pragma once

#include <squared/holodisk/holodrive.hpp>

#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeindex>
#include <utility>
#include <vector>

namespace squared::holodisk {

/**
 * @brief Shared, immutable typed asset ownership returned by AssetManager.
 * @tparam T Application-defined asset value type.
 */
template<typename T>
using AssetHandle = std::shared_ptr<const T>;

/** @brief Resource limits fixed for one AssetManager. */
struct AssetManagerOptions final {
    /** @brief Maximum cached typed assets; must be positive. */
    std::size_t maximum_cached_assets{1024};
    /** @brief Maximum bytes read for one source asset; must be positive. */
    std::uint64_t maximum_asset_bytes{64U * 1024U * 1024U};
    /** @brief Maximum nested typed dependency depth; must be positive. */
    std::size_t maximum_dependency_depth{64};
};

class AssetManager;

/**
 * @brief Restricted services supplied to one typed asset loader.
 *
 * A context reads bounded bytes and requests typed dependencies while the
 * manager records dependency edges. It is valid only during its loader call.
 */
class AssetLoadContext final {
public:
    /**
     * @brief Read an entire mounted source file under the manager byte limit.
     * @param path Absolute HoloDrive virtual path.
     * @return Owned bytes, or a HoloDrive/resource failure.
     */
    [[nodiscard]] Result<std::vector<std::byte>> read_bytes(
        std::string_view path
    ) noexcept;

    /**
     * @brief Load and record one typed dependency.
     * @tparam T Registered asset type.
     * @param path Absolute HoloDrive virtual path.
     * @return Shared immutable dependency or a loader/cache failure.
     */
    template<typename T>
    [[nodiscard]] Result<AssetHandle<T>> load(std::string_view path) noexcept;

private:
    friend class AssetManager;
    explicit AssetLoadContext(AssetManager& manager) noexcept : manager_(&manager) {}
    AssetManager* manager_;
};

/**
 * @brief Strategy callback that constructs one typed asset.
 * @tparam T Application-defined asset value type constructed by the callback.
 */
template<typename T>
using AssetLoader = std::function<Result<AssetHandle<T>>(
    AssetLoadContext& context,
    std::string_view path
)>;

/**
 * @brief Synchronous typed asset cache over one HoloDrive.
 *
 * The manager is application-owned and non-thread-safe. It does not know any
 * graphics or GUI type: consumers register typed loader strategies. Cached
 * assets and external handles use shared immutable ownership. Nested archive
 * mounts are manager-owned and are removed when explicitly unmounted or when
 * the manager is destroyed.
 */
class AssetManager final {
public:
    /**
     * @brief Bind a manager to a drive that outlives it.
     * @param drive Non-owning drive reference.
     * @param options Positive cache, byte, and dependency limits.
     * @throws std::invalid_argument when a limit is zero.
     */
    explicit AssetManager(HoloDrive& drive, AssetManagerOptions options = {});
    ~AssetManager();

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;
    AssetManager(AssetManager&&) = delete;
    AssetManager& operator=(AssetManager&&) = delete;

    /**
     * @brief Register one loader strategy for T.
     * @tparam T Asset type uniquely associated with the callback.
     * @param loader Non-empty synchronous construction callback; copied into
     * the registry and invoked on the manager thread.
     * @return Success, AlreadyExists when T already has a loader, or
     * InvalidArgument for an empty callback.
     */
    template<typename T>
    [[nodiscard]] Status register_loader(AssetLoader<T> loader);

    /**
     * @brief Load or return the cached asset at path.
     * @tparam T Registered asset type.
     * @param path Absolute virtual HoloDrive path.
     * @return Shared immutable handle, or a path, loader, limit, cycle, or
     * loader-provided failure. Never throws.
     */
    template<typename T>
    [[nodiscard]] Result<AssetHandle<T>> load(std::string_view path) noexcept;

    /**
     * @brief Transactionally rebuild and replace one cached asset.
     * @tparam T Registered asset type.
     * @param path Absolute virtual HoloDrive path.
     * @return Success after commit, or a failure that leaves the cache intact.
     * @note Existing handles keep the prior object; cached dependents are not
     * automatically rebuilt.
     */
    template<typename T>
    [[nodiscard]] Status reload(std::string_view path) noexcept;

    /**
     * @brief Remove one cached asset when no cached asset depends on it.
     * @tparam T Asset type forming part of the exact cache key.
     * @param path Absolute virtual HoloDrive path.
     * @return Success, NotFound, Busy, or InvalidPath. Never throws.
     * @note External shared handles remain valid after successful unload.
     */
    template<typename T>
    [[nodiscard]] Status unload(std::string_view path) noexcept;

    /**
     * @brief Report whether one exact typed path is cached.
     * @tparam T Asset type forming part of the exact cache key.
     * @param path Absolute virtual HoloDrive path.
     * @return true only for a valid normalized path with a matching entry.
     */
    template<typename T>
    [[nodiscard]] bool contains(std::string_view path) const noexcept;

    /**
     * @brief Drop all cached entries; external handles remain valid.
     * @post contains<T>() is false for every formerly cached entry.
     */
    void clear() noexcept;

    /**
     * @brief Read a mounted ZIP asset, load it in memory, and mount it read-only.
     * @param archive_path Absolute virtual path of the nested ZIP.
     * @param mount_point New absolute virtual mount point.
     * @return Success after the disk and mount are owned, or a drive/resource
     * failure with all partially created state rolled back.
     */
    [[nodiscard]] Status mount_archive(
        std::string_view archive_path,
        std::string_view mount_point
    ) noexcept;

    /**
     * @brief Remove a manager-owned nested mount.
     * @param mount_point Exact absolute point passed to mount_archive().
     * @return Success, NotFound, Busy, InvalidPath, or a drive cleanup error.
     * @note Fails with Busy while cached assets beneath the mount remain.
     */
    [[nodiscard]] Status unmount_archive(std::string_view mount_point) noexcept;

private:
    friend class AssetLoadContext;
    using ErasedAsset = std::shared_ptr<const void>;
    using ErasedLoader = std::function<Result<ErasedAsset>(
        AssetLoadContext&,
        std::string_view
    )>;

    [[nodiscard]] Status register_loader_erased(
        std::type_index type,
        ErasedLoader loader
    );
    [[nodiscard]] Result<ErasedAsset> load_erased(
        std::type_index type,
        std::string_view path,
        bool force_reload
    ) noexcept;
    [[nodiscard]] Status unload_erased(
        std::type_index type,
        std::string_view path
    ) noexcept;
    [[nodiscard]] bool contains_erased(
        std::type_index type,
        std::string_view path
    ) const noexcept;
    [[nodiscard]] Result<std::vector<std::byte>> read_bytes(
        std::string_view path
    ) noexcept;

    class Implementation;
    std::unique_ptr<Implementation> implementation_;
};

template<typename T>
Result<AssetHandle<T>> AssetLoadContext::load(std::string_view path) noexcept
{
    return manager_->load<T>(path);
}

template<typename T>
Status AssetManager::register_loader(AssetLoader<T> loader)
{
    if (!loader) {
        return Status::failure({ErrorCode::InvalidArgument, "asset loader is empty"});
    }
    return register_loader_erased(
        std::type_index(typeid(T)),
        [loader = std::move(loader)](
            AssetLoadContext& context,
            std::string_view path
        ) -> Result<ErasedAsset> {
            auto loaded = loader(context, path);
            if (!loaded) return Result<ErasedAsset>::failure(loaded.error());
            if (!loaded.value()) {
                return Result<ErasedAsset>::failure(
                    {ErrorCode::LoadFailed, "asset loader returned an empty handle"}
                );
            }
            return Result<ErasedAsset>::success(
                std::static_pointer_cast<const void>(std::move(loaded).value())
            );
        }
    );
}

template<typename T>
Result<AssetHandle<T>> AssetManager::load(std::string_view path) noexcept
{
    auto loaded = load_erased(std::type_index(typeid(T)), path, false);
    if (!loaded) return Result<AssetHandle<T>>::failure(loaded.error());
    return Result<AssetHandle<T>>::success(
        std::static_pointer_cast<const T>(std::move(loaded).value())
    );
}

template<typename T>
Status AssetManager::reload(std::string_view path) noexcept
{
    auto loaded = load_erased(std::type_index(typeid(T)), path, true);
    return loaded ? Status::success() : Status::failure(loaded.error());
}

template<typename T>
Status AssetManager::unload(std::string_view path) noexcept
{
    return unload_erased(std::type_index(typeid(T)), path);
}

template<typename T>
bool AssetManager::contains(std::string_view path) const noexcept
{
    return contains_erased(std::type_index(typeid(T)), path);
}

} // namespace squared::holodisk
