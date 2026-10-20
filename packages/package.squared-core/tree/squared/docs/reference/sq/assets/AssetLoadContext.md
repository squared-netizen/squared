# class AssetLoadContext

*Defined at [assets/src/asset_load_context.hpp:23](../../../../assets/src/asset_load_context.hpp#L23)*



**brief** The restricted view of the manager a loader is given.

 A loader reads bytes and asks for typed dependencies through this; it never sees the manager itself. Every dependency requested here is recorded, so reloading or unloading an asset knows what it pulled in.

 A context is valid only for the duration of the loader call that received it. Storing one and using it later is undefined.



## Functions

### read_bytes

*public files::FileReadResult read_bytes(int path)*



**brief** Read one source file, subject to the manager's byte limit.

**path** Path relative to the manager's file system root.

**return** The bytes, or the file system failure that prevented them.

### load

*public AssetLoadResult<T> load(int path)*

*Defined at [assets/asset_manager.hpp:203](../../../../assets/asset_manager.hpp#L203)*



**brief** Load one typed dependency and record the edge.

**T** Asset type with a registered loader.

**path** Path relative to the manager's file system root.

**return** The dependency, or the failure that prevented it.

**note** Defined in asset_manager.hpp, which you need anyway to have a manager to load from.



