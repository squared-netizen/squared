#include <squared/assets/asset_manager.hpp>

#include <squared/assets/asset_error.hpp>
#include <squared/assets/asset_error_code.hpp>
#include <squared/assets/asset_load_context.hpp>
#include <squared/assets/asset_manager_options.hpp>
#include <squared/assets/asset_type_id.hpp>
#include <squared/files/file_handle.hpp>
#include <squared/files/file_read_result.hpp>
#include <squared/files/file_system.hpp>

#include <cassert>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace sq::assets {

namespace {

AssetError make_error(
    AssetErrorCode code,
    std::string message,
    std::string_view path
)
{
    return AssetError{
        .code = code,
        .message = std::move(message),
        .path = std::string{path}
    };
}

/** @brief One cached asset plus the edges its loader recorded. */
struct CacheEntry final {
    std::shared_ptr<const void> asset;
    std::vector<std::pair<AssetTypeId, std::string>> dependencies;
};

using PathTable = std::unordered_map<std::string, CacheEntry>;

}  // namespace

class AssetManager::Implementation final {
public:
    Implementation(files::FileSystem& bound, AssetManagerOptions policy)
        : file_system(bound)
        , options(policy)
    {
    }

    files::FileSystem& file_system;
    AssetManagerOptions options;

    std::unordered_map<AssetTypeId, ErasedLoader> loaders;
    std::unordered_map<AssetTypeId, PathTable> cache;

    /** @brief Nesting depth of the load currently in progress. */
    std::size_t depth{0};

    /** @brief Edges recorded for the load at each depth, innermost last. */
    std::vector<std::vector<std::pair<AssetTypeId, std::string>>> pending;

    /** @brief Keys of the loads in progress, used to detect a cycle. */
    std::vector<std::pair<AssetTypeId, std::string>> in_progress;

    [[nodiscard]] std::size_t cached_count() const noexcept
    {
        std::size_t total = 0;
        for (const auto& [type, paths] : cache) total += paths.size();
        return total;
    }

    [[nodiscard]] const CacheEntry* find(
        AssetTypeId type,
        std::string_view path
    ) const noexcept
    {
        const auto type_entry = cache.find(type);
        if (type_entry == cache.end()) return nullptr;
        const auto path_entry = type_entry->second.find(std::string{path});
        if (path_entry == type_entry->second.end()) return nullptr;
        return &path_entry->second;
    }
};

AssetManager::AssetManager(
    files::FileSystem& file_system,
    AssetManagerOptions options
)
    : implementation_(
          std::make_unique<Implementation>(file_system, options)
      )
{
    // Preconditions, not recoverable failures: a zero limit is a bug at the
    // call site, and the framework's rule is assert for programmer error.
    assert(options.maximum_cached_assets > 0);
    assert(options.maximum_asset_bytes > 0);
    assert(options.maximum_dependency_depth > 0);
}

AssetManager::~AssetManager() = default;

AssetError AssetManager::register_loader_erased(
    AssetTypeId type,
    ErasedLoader loader
)
{
    auto& loaders = implementation_->loaders;
    if (loaders.find(type) != loaders.end()) {
        return make_error(
            AssetErrorCode::LoaderAlreadyRegistered,
            "a loader is already registered for this asset type",
            {}
        );
    }
    loaders.emplace(type, std::move(loader));
    return {};
}

AssetError AssetManager::load_erased(
    AssetTypeId type,
    std::string_view path,
    bool force_reload,
    ErasedAsset& asset
)
{
    Implementation& state = *implementation_;

    if (path.empty()) {
        return make_error(
            AssetErrorCode::InvalidArgument, "asset path is empty", path
        );
    }

    const std::pair<AssetTypeId, std::string> key{type, std::string{path}};

    if (!force_reload) {
        if (const CacheEntry* entry = state.find(type, path)) {
            asset = entry->asset;
            if (state.depth > 0) state.pending.back().push_back(key);
            return {};
        }
    }

    const auto loader = state.loaders.find(type);
    if (loader == state.loaders.end()) {
        return make_error(
            AssetErrorCode::NoLoader,
            "no loader is registered for this asset type",
            path
        );
    }

    if (state.depth >= state.options.maximum_dependency_depth) {
        return make_error(
            AssetErrorCode::DependencyTooDeep,
            "dependency nesting exceeded the configured maximum",
            path
        );
    }

    for (const auto& active : state.in_progress) {
        if (active == key) {
            return make_error(
                AssetErrorCode::DependencyCycle,
                "asset depends on itself, directly or through a cycle",
                path
            );
        }
    }

    if (!force_reload
        && state.cached_count() >= state.options.maximum_cached_assets) {
        return make_error(
            AssetErrorCode::CacheFull,
            "cache is at its configured maximum",
            path
        );
    }

    state.in_progress.push_back(key);
    state.pending.emplace_back();
    ++state.depth;

    AssetLoadContext context{*this};
    ErasedAsset produced;
    AssetError error = loader->second(context, path, produced);

    auto recorded = std::move(state.pending.back());
    state.pending.pop_back();
    --state.depth;
    state.in_progress.pop_back();

    if (error) return error;
    if (!produced) {
        return make_error(
            AssetErrorCode::LoaderFailed, "loader produced no asset", path
        );
    }

    state.cache[type][std::string{path}] =
        CacheEntry{.asset = produced, .dependencies = std::move(recorded)};

    if (state.depth > 0) state.pending.back().push_back(key);
    asset = std::move(produced);
    return {};
}

AssetError AssetManager::unload_erased(AssetTypeId type, std::string_view path)
{
    Implementation& state = *implementation_;

    const auto type_entry = state.cache.find(type);
    if (type_entry == state.cache.end()) {
        return make_error(
            AssetErrorCode::NotLoaded, "asset is not cached", path
        );
    }

    const auto path_entry = type_entry->second.find(std::string{path});
    if (path_entry == type_entry->second.end()) {
        return make_error(
            AssetErrorCode::NotLoaded, "asset is not cached", path
        );
    }

    // Copy the edges before erasing: the entry owns them.
    const auto dependencies = path_entry->second.dependencies;
    type_entry->second.erase(path_entry);

    // Dependencies leave the cache with their dependent. Anything still held
    // elsewhere stays alive through its handle.
    for (const auto& [dependency_type, dependency_path] : dependencies) {
        static_cast<void>(unload_erased(dependency_type, dependency_path));
    }
    return {};
}

bool AssetManager::contains_erased(
    AssetTypeId type,
    std::string_view path
) const noexcept
{
    return implementation_->find(type, path) != nullptr;
}

files::FileReadResult AssetManager::read_bytes(std::string_view path)
{
    Implementation& state = *implementation_;

    files::FileHandle handle = state.file_system.internal(path);
    if (handle.length() > state.options.maximum_asset_bytes) {
        return files::FileReadResult{
            .bytes = {},
            .error = files::FileError{
                .code = files::FileErrorCode::TooLarge,
                .message = "source exceeds the configured asset byte limit",
                .path = std::string{path}
            }
        };
    }
    return handle.read_bytes();
}

void AssetManager::clear() noexcept
{
    implementation_->cache.clear();
}

std::size_t AssetManager::cached_count() const noexcept
{
    return implementation_->cached_count();
}

const AssetManagerOptions& AssetManager::options() const noexcept
{
    return implementation_->options;
}

files::FileReadResult AssetLoadContext::read_bytes(std::string_view path)
{
    return manager_->read_bytes(path);
}

}  // namespace sq::assets
