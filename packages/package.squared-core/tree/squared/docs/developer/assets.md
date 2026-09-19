# assets &mdash; internals

Typed asset cache over `sq::files::FileSystem`.

Programmer counterpart: [../programmer/assets.md](../programmer/assets.md)

- Public types: 9
- Translation units: 1

## No RTTI, and how

The obvious implementation keys the loader and cache tables on
`std::type_index`. That is unavailable: `typeid` is rejected outright under
`-fno-rtti`, for any type, and squared builds that way.

`asset_type_id<T>()` returns the address of `detail::asset_type_tag<T>`, an
inline variable. An inline variable has one address across every translation
unit, so the result is unique per type, stable for the life of the program,
`constexpr`, and free at run time. No registration, no macro, no central list.

This is the same problem `scene2d::actor_cast` solves, answered the same way.

## What it is, and is not

It is a cache: identity, deduplication, lifetime, dependency edges, bounded by
`AssetManagerOptions`.

It is **not** the framework's memory manager, and should not become one. Most
of squared's memory is not assets &mdash; `gui::Skin`'s hash maps,
`SpriteBatch`'s vertex buffer, per-frame scratch, the widget tree. Routing
those through here would mean one type understanding allocation patterns it has
no business knowing.

The useful version of that instinct: the manager is the framework's *largest
client* of memory, so giving it an arena sized from `maximum_asset_bytes` would
make it the first subsystem to actually satisfy the memory rule. Nothing in the
tree does yet; everything uses global `new`.

## Dependency tracking

`load_erased` keeps three stacks: the nesting depth, one pending edge list per
depth, and the keys currently in progress.

A loader that calls `context.load<T>()` pushes an edge onto the innermost
pending list, including on a cache hit &mdash; a dependency that was already
loaded is still a dependency. When the loader returns, that list becomes the
entry's `dependencies`.

The in-progress stack is what turns a cycle into a `DependencyCycle` result
instead of a stack overflow. The depth limit catches deep-but-acyclic chains.

`unload_erased` copies the edge list before erasing the entry, because the
entry owns it, then recurses. Cascaded dependencies that are still referenced
survive through their `shared_ptr`; only the cache entry goes.

## Standard library deviations

| Facility | Where | Why it stays |
|---|---|---|
| `std::shared_ptr` | `AssetHandle<T>` | Genuinely shared, genuinely dynamic: a texture outlives the skin that requested it when a widget still holds it. 16 bytes plus a control block per distinct asset, never per frame. |
| `std::function` | `AssetLoader<T>`, `ErasedLoader` | One per asset type, registered once at startup. A concept-constrained template would force the cache to be a template and give every asset type its own copy of the class. |
| `std::unordered_map` | loaders and cache tables | Keyed by `AssetTypeId` and path. Bounded by `maximum_cached_assets`; the same interning argument that applies to `gui::Skin` applies here, and matters less because assets are counted in hundreds. |

## Type erasure

Cached assets are `std::shared_ptr<const void>`, cast back with
`std::static_pointer_cast<const T>`. That is safe because the cache is keyed by
`AssetTypeId`: an entry found under `asset_type_id<T>()` was stored by the
loader registered for `T`, and nothing else can reach it. The typed
`register_loader<T>` wraps the user's callback in the erased signature at
registration, so the erasure happens once per type rather than per load.

## Not carried over

The original had `mount_archive` and `unmount_archive`. Those belong to the
file system, not the asset cache &mdash; mounting is about where bytes come
from, which is exactly what `sq::files::FileSystem` is for. Adding a zip-backed
`FileSystem` gives every consumer archive support, not only the ones going
through the manager.
