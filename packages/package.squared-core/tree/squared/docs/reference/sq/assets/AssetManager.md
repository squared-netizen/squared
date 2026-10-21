# class AssetManager

*Defined at [assets/asset_manager.hpp:43](../../../../assets/asset_manager.hpp#L43)*



**brief** Typed asset cache over one file system.

 One place to ask for an asset: the manager loads it once, hands out shared immutable references, records what each asset pulled in, and drops the lot on unload. It knows nothing about graphics, GUI, or any other subsystem; consumers register a loader per type.

 It is a cache, not an allocator. Subsystems still own their own memory; this owns loaded asset bytes and the handles to them, bounded by AssetManagerOptions.

 Not thread-safe. The file system is referenced, not owned, and must outlive the manager.



## Functions

### AssetManager

*public void AssetManager(files::FileSystem & file_system, AssetManagerOptions options)*



**brief** Bind a manager to a file system that outlives it.

**file_system** Byte source for every load.

**options** Limits; every field is a positive-value precondition.

### AssetManager

*public void AssetManager(const AssetManager & )*

*Defined at [assets/asset_manager.hpp:54](../../../../assets/asset_manager.hpp#L54)*

### operator=

*public AssetManager & operator=(const AssetManager & )*

*Defined at [assets/asset_manager.hpp:55](../../../../assets/asset_manager.hpp#L55)*

### AssetManager

*public void AssetManager(AssetManager && )*

*Defined at [assets/asset_manager.hpp:56](../../../../assets/asset_manager.hpp#L56)*

### operator=

*public AssetManager & operator=(AssetManager && )*

*Defined at [assets/asset_manager.hpp:57](../../../../assets/asset_manager.hpp#L57)*

### ~AssetManager

*public void ~AssetManager()*

### register_loader

*public AssetError register_loader(AssetLoader<T> loader)*

*Defined at [assets/asset_manager.hpp:140](../../../../assets/asset_manager.hpp#L140)*



**brief** Register the loader for one asset type.

**T** Asset value type; each type accepts one loader.

**loader** Non-empty callback, invoked on the calling thread.

**return** A no-error AssetError, LoaderAlreadyRegistered, or InvalidArgument for an empty callback.

### load

*public AssetLoadResult<T> load(std::string_view path)*

*Defined at [assets/asset_manager.hpp:167](../../../../assets/asset_manager.hpp#L167)*



**brief** Load one asset, or return the cached copy.

**T** Asset value type with a registered loader.

**path** Path relative to the file system's root.

**return** The asset, or the failure that prevented it.

### reload

*public AssetLoadResult<T> reload(std::string_view path)*

*Defined at [assets/asset_manager.hpp:179](../../../../assets/asset_manager.hpp#L179)*



**brief** Reload one asset from its source, replacing the cached copy.

**T** Asset value type with a registered loader.

**path** Path relative to the file system's root.

**return** The reloaded asset, or the failure that prevented it.

**note** References already handed out keep pointing at the old asset. That is what makes reload safe to call while a frame is in flight.

### unload

*public AssetError unload(std::string_view path)*

*Defined at [assets/asset_manager.hpp:191](../../../../assets/asset_manager.hpp#L191)*



**brief** Drop one cached asset and the dependency edges it recorded.

**T** Asset value type.

**path** Path relative to the file system's root.

**return** A no-error AssetError, or NotLoaded when nothing was cached.

**note** Dependencies are dropped from the cache too, but any still held by another asset or by the caller stay alive through their handles.

### contains

*public bool contains(std::string_view path)*

*Defined at [assets/asset_manager.hpp:197](../../../../assets/asset_manager.hpp#L197)*



**brief** Report whether one asset is currently cached. 

### clear

*public void clear()*



**brief** Drop every cached asset. Registered loaders are kept. 

### cached_count

*public std::size_t cached_count()*



**brief** Return the number of cached assets across all types. 

### options

*public const AssetManagerOptions & options()*



**brief** Return the limits this manager was constructed with. 



