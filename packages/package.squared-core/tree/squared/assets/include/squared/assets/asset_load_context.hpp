#pragma once

#include <squared/assets/asset_load_result.hpp>
#include <squared/files/file_read_result.hpp>

#include <string_view>

namespace sq::assets {

class AssetManager;

/**
 * @brief The restricted view of the manager a loader is given.
 *
 * A loader reads bytes and asks for typed dependencies through this; it never
 * sees the manager itself. Every dependency requested here is recorded, so
 * reloading or unloading an asset knows what it pulled in.
 *
 * A context is valid only for the duration of the loader call that received
 * it. Storing one and using it later is undefined.
 */
class AssetLoadContext final {
public:
    /**
     * @brief Read one source file, subject to the manager's byte limit.
     * @param path Path relative to the manager's file system root.
     * @return The bytes, or the file system failure that prevented them.
     */
    [[nodiscard]] files::FileReadResult read_bytes(std::string_view path);

    /**
     * @brief Load one typed dependency and record the edge.
     * @tparam T Asset type with a registered loader.
     * @param path Path relative to the manager's file system root.
     * @return The dependency, or the failure that prevented it.
     * @note Defined in asset_manager.hpp, which you need anyway to have a
     * manager to load from.
     */
    template <typename T>
    [[nodiscard]] AssetLoadResult<T> load(std::string_view path);

private:
    friend class AssetManager;
    explicit AssetLoadContext(AssetManager& manager) noexcept
        : manager_(&manager)
    {
    }

    AssetManager* manager_;
};

}  // namespace sq::assets
