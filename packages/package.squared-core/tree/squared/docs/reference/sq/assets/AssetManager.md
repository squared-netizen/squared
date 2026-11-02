# class AssetManager

*Defined at [assets/asset_manager.cppm:32](../../../../assets/asset_manager.cppm#L32)*



**brief** Typed asset cache over one file system.

 One place to ask for an asset: the manager loads it once, hands out shared immutable references, records what each asset pulled in, and drops the lot on unload. It knows nothing about graphics, GUI, or any other subsystem; consumers register a loader per type.

 It is a cache, not an allocator. Subsystems still own their own memory; this owns loaded asset bytes and the handles to them, bounded by AssetManagerOptions.

 Not thread-safe. The file system is referenced, not owned, and must outlive the manager.



## Functions

### AssetManager

*public void AssetManager(AssetManager && )*

*Defined at [assets/asset_manager.cppm:45](../../../../assets/asset_manager.cppm#L45)*

### AssetManager

*public void AssetManager(const AssetManager & )*

*Defined at [assets/asset_manager.cppm:43](../../../../assets/asset_manager.cppm#L43)*

### AssetManager

*public void AssetManager(files::FileSystem & file_system, AssetManagerOptions options)*



**brief** Bind a manager to a file system that outlives it.

**file_system** Byte source for every load.

**options** Limits; every field is a positive-value precondition.

### cached_count

*public std::size_t cached_count()*



**brief** Return the number of cached assets across all types. 

### clear

*public void clear()*



**brief** Drop every cached asset. Registered loaders are kept. 

### contains

*public bool contains(std::string_view path)*

*Defined at [assets/asset_manager.cppm:186](../../../../assets/asset_manager.cppm#L186)*



**brief** Report whether one asset is currently cached. 

### load

*public AssetLoadResult<T> load(std::string_view path)*

*Defined at [assets/asset_manager.cppm:156](../../../../assets/asset_manager.cppm#L156)*



**brief** Load one asset, or return the cached copy.

**T** Asset value type with a registered loader.

**path** Path relative to the file system's root.

**return** The asset, or the failure that prevented it.

### operator=

*public AssetManager & operator=(AssetManager && )*

*Defined at [assets/asset_manager.cppm:46](../../../../assets/asset_manager.cppm#L46)*

### operator=

*public AssetManager & operator=(const AssetManager & )*

*Defined at [assets/asset_manager.cppm:44](../../../../assets/asset_manager.cppm#L44)*

### options

*public const AssetManagerOptions & options()*



**brief** Return the limits this manager was constructed with. 

### register_loader

*public AssetError register_loader(AssetLoader<T> loader)*

*Defined at [assets/asset_manager.cppm:129](../../../../assets/asset_manager.cppm#L129)*



**brief** Register the loader for one asset type.

**T** Asset value type; each type accepts one loader.

**loader** Non-empty callback, invoked on the calling thread.

**return** A no-error AssetError, LoaderAlreadyRegistered, or InvalidArgument for an empty callback.

### reload

*public AssetLoadResult<T> reload(std::string_view path)*

*Defined at [assets/asset_manager.cppm:168](../../../../assets/asset_manager.cppm#L168)*



**brief** Reload one asset from its source, replacing the cached copy.

**T** Asset value type with a registered loader.

**path** Path relative to the file system's root.

**return** The reloaded asset, or the failure that prevented it.

**note** References already handed out keep pointing at the old asset. That is what makes reload safe to call while a frame is in flight.

### unload

*public AssetError unload(std::string_view path)*

*Defined at [assets/asset_manager.cppm:180](../../../../assets/asset_manager.cppm#L180)*



**brief** Drop one cached asset and the dependency edges it recorded.

**T** Asset value type.

**path** Path relative to the file system's root.

**return** A no-error AssetError, or NotLoaded when nothing was cached.

**note** Dependencies are dropped from the cache too, but any still held by another asset or by the caller stay alive through their handles.

### ~AssetManager

*public void ~AssetManager()*

