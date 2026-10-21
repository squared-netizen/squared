# struct AssetManagerOptions

*Defined at [assets/src/asset_manager_options.hpp:16](../../../../assets/src/asset_manager_options.hpp#L16)*



**brief** Resource limits fixed for one AssetManager.

 Every limit is a precondition: passing zero is a programmer error and is asserted, not reported. These are the numbers that make the manager's footprint knowable before it runs.



## Members

public std::size_t maximum_cached_assets

public std::uint64_t maximum_asset_bytes

public std::size_t maximum_dependency_depth



