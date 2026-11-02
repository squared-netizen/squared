# class AssetManager

*Defined at [assets/asset_manager.cppm:32](../../../../assets/asset_manager.cppm#L32)*

## Functions

### AssetManager

*public void AssetManager(files::FileSystem & file_system, AssetManagerOptions options)*



**brief** Bind a manager to a file system that outlives it.

**file_system** Byte source for every load.

**options** Limits; every field is a positive-value precondition.

### AssetManager

*public void AssetManager(const AssetManager & )*

*Defined at [assets/asset_manager.cppm:43](../../../../assets/asset_manager.cppm#L43)*

### operator=

*public AssetManager & operator=(const AssetManager & )*

*Defined at [assets/asset_manager.cppm:44](../../../../assets/asset_manager.cppm#L44)*

### AssetManager

*public void AssetManager(AssetManager && )*

*Defined at [assets/asset_manager.cppm:45](../../../../assets/asset_manager.cppm#L45)*

### operator=

*public AssetManager & operator=(AssetManager && )*

*Defined at [assets/asset_manager.cppm:46](../../../../assets/asset_manager.cppm#L46)*

### ~AssetManager

*public void ~AssetManager()*

### register_loader

*public AssetError register_loader(AssetLoader<T> loader)*

*Defined at [assets/asset_manager.cppm:129](../../../../assets/asset_manager.cppm#L129)*

### load

*public AssetLoadResult<T> load(std::string_view path)*

*Defined at [assets/asset_manager.cppm:156](../../../../assets/asset_manager.cppm#L156)*

### reload

*public AssetLoadResult<T> reload(std::string_view path)*

*Defined at [assets/asset_manager.cppm:168](../../../../assets/asset_manager.cppm#L168)*

### unload

*public AssetError unload(std::string_view path)*

*Defined at [assets/asset_manager.cppm:180](../../../../assets/asset_manager.cppm#L180)*

### contains

*public bool contains(std::string_view path)*

*Defined at [assets/asset_manager.cppm:186](../../../../assets/asset_manager.cppm#L186)*

### clear

*public void clear()*



**brief** Drop every cached asset. Registered loaders are kept. 

### cached_count

*public std::size_t cached_count()*



**brief** Return the number of cached assets across all types. 

### options

*public const AssetManagerOptions & options()*



**brief** Return the limits this manager was constructed with. 



