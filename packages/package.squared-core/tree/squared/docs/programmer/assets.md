# assets

One place to ask for an asset. The manager loads it once, hands out shared
immutable references, records what each asset pulled in, and drops the lot on
unload.

Developer counterpart: [../developer/assets.md](../developer/assets.md)

| Type | Header | Purpose |
|---|---|---|
| `AssetManager` | `squared/assets/asset_manager.hpp` | Typed asset cache over one file system |
| `AssetManagerOptions` | `squared/assets/asset_manager_options.hpp` | Resource limits fixed for one AssetManager |
| `AssetHandle` | `squared/assets/asset_handle.hpp` | Shared immutable reference to one loaded asset |
| `AssetLoader` | `squared/assets/asset_loader.hpp` | Callback that turns a path into one asset of type T |
| `AssetLoadContext` | `squared/assets/asset_load_context.hpp` | The restricted view of the manager a loader is given |
| `AssetLoadResult` | `squared/assets/asset_load_result.hpp` | One loaded asset, or the failure that prevented loading it |
| `AssetTypeId` | `squared/assets/asset_type_id.hpp` | Stable per-type identifier used to key the loader and cache tables |
| `AssetError` | `squared/assets/asset_error.hpp` | Structured asset loading failure information |
| `AssetErrorCode` | `squared/assets/asset_error_code.hpp` | Stable error categories produced by asset loading |

## The shortest call site

```cpp
sq::assets::AssetManager manager{file_system};

if (auto error = manager.register_loader<Texture>(load_texture)) {
    report(error);
}

if (auto texture = manager.load<Texture>("art/hero.png")) {
    draw(*texture.asset);
}
```

A second `load` of the same path and type returns the cached handle without
calling the loader again.

## Writing a loader

A loader gets an `AssetLoadContext`, not the manager. It can read bytes and ask
for typed dependencies, and nothing else:

```cpp
auto load_skin = [](sq::assets::AssetLoadContext& context,
                    std::string_view path) {
    auto atlas = context.load<TextureAtlas>("skins/default.atlas");
    if (!atlas) {
        return sq::assets::AssetLoadResult<Skin>{
            .asset = {}, .error = std::move(atlas.error)};
    }

    auto bytes = context.read_bytes(path);
    if (!bytes) { ... }

    return sq::assets::AssetLoadResult<Skin>{
        .asset = std::make_shared<const Skin>(build(bytes.bytes, atlas.asset)),
        .error = {}};
};
```

Every dependency asked for through the context is recorded, so unloading the
skin drops the atlas with it.

One loader per type. Registering a second for the same type returns
`LoaderAlreadyRegistered` rather than replacing it.

## Reload and unload

```cpp
manager.reload<Skin>("skins/default.json");   // re-reads the source
manager.unload<Skin>("skins/default.json");   // drops it and its dependencies
manager.clear();                              // drops everything, keeps loaders
```

**Reload does not disturb handles already handed out.** They keep pointing at
the old asset until their last holder drops them, which is what makes reload
safe to call while a frame is in flight.

**Unload cascades** to the dependencies the loader recorded. Anything still
held elsewhere &mdash; by another asset or by you &mdash; stays alive through
its handle; only the cache entry goes.

## Limits

```cpp
sq::assets::AssetManager manager{file_system, {
    .maximum_cached_assets = 512,
    .maximum_asset_bytes = 16U * 1024U * 1024U,
    .maximum_dependency_depth = 32
}};
```

Every limit is a precondition: zero is a programmer error and is asserted, not
reported. Exceeding one at run time is an ordinary failure &mdash; `CacheFull`,
`SourceTooLarge`, `DependencyTooDeep` &mdash; and comes back in the result.

An asset that depends on itself, directly or through a chain, returns
`DependencyCycle` rather than recursing until the stack runs out.
