# Squared GUI File Picker History

## 0.6.0-dev.3

- Advanced the exact GUI dependency to `0.6.0-dev.17` while preserving the HoloDisk `0.6.0-dev.3`, HoloDrive, AssetManager, and gdx-holo integration.

## 0.6.0-dev.2

- Advanced the exact GUI dependency to `0.6.0-dev.16` so file-picker builds
  resolve the ScrollBar/ListView milestone together with HoloDisk dev.3.
- Preserved the HoloDrive, AssetManager, and packaged gdx-holo integration;
  this is a dependency-alignment release with no file-picker API change.

## 0.6.0-dev.1

- Added a `Window`-based file picker composed from Squared GUI primitives.
- Added HoloDrive directory navigation and AssetManager-backed typed selection loading.
- Added transactional loading of the packaged gdx-holo skin JSON through AssetManager.
- Declared exact GUI dev.15 and HoloDisk dev.3 package dependencies.
