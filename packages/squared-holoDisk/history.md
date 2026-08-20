# Squared HoloDisk History

## 0.6.0-dev.3

- Added the application-owned synchronous `AssetManager` with typed loader
  strategies, immutable shared handles, bounded byte reads, caching,
  dependency tracking, cycle detection, transactional reload, and explicit
  unload/clear operations.
- Added manager-owned nested ZIP mounts and memory-backed `HoloDrive` loading
  so pinned archives such as `gdx-skins.zip` can be mounted without extraction
  into package paths.
- Added focused host coverage for cache identity, loader failures, resource
  limits, dependency-protected unload, reload handle stability, cycles,
  memory-backed disks, and nested archive lifecycles.

## 0.6.0-dev.2

- Expanded Doxygen coverage on the HoloDrive boundary header.
- Documentation-only release: no ABI or behavior change.

## 0.6.0-dev.1

- Added emulated and ZIP-backed cartridges behind one `HoloDrive` boundary.
- Added streamed reads, scratch-backed writes, safe paths, resource limits, and ZIP materialization.
- Vendored miniz so the package remains independent.
