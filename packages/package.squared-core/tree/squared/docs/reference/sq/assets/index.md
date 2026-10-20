# namespace assets

## Namespaces

* [detail](detail/index.md)


## Records

* [AssetError](AssetError.md)
* [AssetLoadContext](AssetLoadContext.md)
* [AssetLoadResult](AssetLoadResult.md)
* [AssetManagerOptions](AssetManagerOptions.md)
* [AssetManager](AssetManager.md)


## Functions

### asset_type_id

*AssetTypeId asset_type_id()*

*Defined at [assets/src/asset_type_id.hpp:27](../../../../assets/src/asset_type_id.hpp#L27)*



**brief** Return the identifier for one asset type.

**T** Any type; no registration or declaration is required.

**return** An address unique to T and stable for the life of the program.



**note** This replaces std::type_index, which cannot be used: typeid is rejected outright under -fno-rtti, and squared builds that way. An inline variable has one address across every translation unit, so this is unique, stable, and costs nothing at run time.

## Enums

| enum class AssetErrorCode |

--

| None |
| NoLoader |
| LoaderAlreadyRegistered |
| InvalidArgument |
| SourceUnavailable |
| SourceTooLarge |
| CacheFull |
| DependencyTooDeep |
| DependencyCycle |
| LoaderFailed |
| NotLoaded |


*Defined at [assets/src/asset_error_code.hpp:9](../../../../assets/src/asset_error_code.hpp#L9)*



**brief** Stable error categories produced by asset loading. 

