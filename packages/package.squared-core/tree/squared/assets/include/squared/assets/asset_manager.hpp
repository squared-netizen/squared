#pragma once

#include <squared/assets/asset_error.hpp>
#include <squared/assets/asset_error_code.hpp>
#include <squared/assets/asset_handle.hpp>
#include <squared/assets/asset_load_context.hpp>
#include <squared/assets/asset_load_result.hpp>
#include <squared/assets/asset_loader.hpp>
#include <squared/assets/asset_manager_options.hpp>
#include <squared/assets/asset_type_id.hpp>
#include <squared/files/file_read_result.hpp>

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace sq::files {
class FileSystem;
}  // namespace sq::files

namespace sq::assets {

/**
 * @brief Typed asset cache over one file system.
 *
 * One place to ask for an asset: the manager loads it once, hands out shared
 * immutable references, records what each asset pulled in, and drops the lot
 * on unload. It knows nothing about graphics, GUI, or any other subsystem;
 * consumers register a loader per type.
 *
 * It is a cache, not an allocator. Subsystems still own their own memory; this
 * owns loaded asset bytes and the handles to them, bounded by
 * AssetManagerOptions.
 *
 * Not thread-safe. The file system is referenced, not owned, and must outlive
 * the manager.
 */
class AssetManager final {
public:
    /**
     * @brief Bind a manager to a file system that outlives it.
     * @param file_system Byte source for every load.
     * @param options Limits; every field is a positive-value precondition.
     */
    explicit AssetManager(
        files::FileSystem& file_system,
        AssetManagerOptions options = {}
    );
    ~AssetManager();

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;
    AssetManager(AssetManager&&) = delete;
    AssetManager& operator=(AssetManager&&) = delete;

    /**
     * @brief Register the loader for one asset type.
     * @tparam T Asset value type; each type accepts one loader.
     * @param loader Non-empty callback, invoked on the calling thread.
     * @return A no-error AssetError, LoaderAlreadyRegistered, or
     * InvalidArgument for an empty callback.
     */
    template <typename T>
    [[nodiscard]] AssetError register_loader(AssetLoader<T> loader);

    /**
     * @brief Load one asset, or return the cached copy.
     * @tparam T Asset value type with a registered loader.
     * @param path Path relative to the file system's root.
     * @return The asset, or the failure that prevented it.
     */
    template <typename T>
    [[nodiscard]] AssetLoadResult<T> load(std::string_view path);

    /**
     * @brief Reload one asset from its source, replacing the cached copy.
     * @tparam T Asset value type with a registered loader.
     * @param path Path relative to the file system's root.
     * @return The reloaded asset, or the failure that prevented it.
     * @note References already handed out keep pointing at the old asset. That
     * is what makes reload safe to call while a frame is in flight.
     */
    template <typename T>
    [[nodiscard]] AssetLoadResult<T> reload(std::string_view path);

    /**
     * @brief Drop one cached asset and the dependency edges it recorded.
     * @tparam T Asset value type.
     * @param path Path relative to the file system's root.
     * @return A no-error AssetError, or NotLoaded when nothing was cached.
     * @note Dependencies are dropped from the cache too, but any still held by
     * another asset or by the caller stay alive through their handles.
     */
    template <typename T>
    [[nodiscard]] AssetError unload(std::string_view path);

    /** @brief Report whether one asset is currently cached. */
    template <typename T>
    [[nodiscard]] bool contains(std::string_view path) const noexcept;

    /** @brief Drop every cached asset. Registered loaders are kept. */
    void clear() noexcept;

    /** @brief Return the number of cached assets across all types. */
    [[nodiscard]] std::size_t cached_count() const noexcept;

    /** @brief Return the limits this manager was constructed with. */
    [[nodiscard]] const AssetManagerOptions& options() const noexcept;

private:
    friend class AssetLoadContext;

    /** @brief Type-erased cached asset; cast back by its AssetTypeId. */
    using ErasedAsset = std::shared_ptr<const void>;

    /** @brief Type-erased loader; the typed one is wrapped on registration. */
    using ErasedLoader = std::function<
        AssetError(AssetLoadContext&, std::string_view, ErasedAsset&)
    >;

    [[nodiscard]] AssetError register_loader_erased(
        AssetTypeId type,
        ErasedLoader loader
    );

    [[nodiscard]] AssetError load_erased(
        AssetTypeId type,
        std::string_view path,
        bool force_reload,
        ErasedAsset& asset
    );

    [[nodiscard]] AssetError unload_erased(
        AssetTypeId type,
        std::string_view path
    );

    [[nodiscard]] bool contains_erased(
        AssetTypeId type,
        std::string_view path
    ) const noexcept;

    [[nodiscard]] files::FileReadResult read_bytes(std::string_view path);

    class Implementation;
    std::unique_ptr<Implementation> implementation_;
};

template <typename T>
AssetError AssetManager::register_loader(AssetLoader<T> loader)
{
    if (!loader) {
        return AssetError{
            .code = AssetErrorCode::InvalidArgument,
            .message = "asset loader callback is empty",
            .path = {}
        };
    }

    return register_loader_erased(
        asset_type_id<T>(),
        [typed = std::move(loader)](
            AssetLoadContext& context,
            std::string_view path,
            ErasedAsset& out
        ) -> AssetError {
            AssetLoadResult<T> result = typed(context, path);
            if (!result) {
                if (result.error) return std::move(result.error);
                return AssetError{
                    .code = AssetErrorCode::LoaderFailed,
                    .message = "loader returned no asset and no error",
                    .path = std::string{path}
                };
            }
            out = std::move(result.asset);
            return {};
        }
    );
}

template <typename T>
AssetLoadResult<T> AssetManager::load(std::string_view path)
{
    ErasedAsset erased;
    AssetError error = load_erased(asset_type_id<T>(), path, false, erased);
    if (error) return AssetLoadResult<T>{.asset = {}, .error = std::move(error)};
    return AssetLoadResult<T>{
        .asset = std::static_pointer_cast<const T>(std::move(erased)),
        .error = {}
    };
}

template <typename T>
AssetLoadResult<T> AssetManager::reload(std::string_view path)
{
    ErasedAsset erased;
    AssetError error = load_erased(asset_type_id<T>(), path, true, erased);
    if (error) return AssetLoadResult<T>{.asset = {}, .error = std::move(error)};
    return AssetLoadResult<T>{
        .asset = std::static_pointer_cast<const T>(std::move(erased)),
        .error = {}
    };
}

template <typename T>
AssetError AssetManager::unload(std::string_view path)
{
    return unload_erased(asset_type_id<T>(), path);
}

template <typename T>
bool AssetManager::contains(std::string_view path) const noexcept
{
    return contains_erased(asset_type_id<T>(), path);
}

template <typename T>
AssetLoadResult<T> AssetLoadContext::load(std::string_view path)
{
    return manager_->load<T>(path);
}

}  // namespace sq::assets
