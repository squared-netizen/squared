# Squared HoloDisk TODO

Only unfinished work belongs here. Move completed work into `history.md`.

## Next

- [ ] Design a portable AssetManager that resolves, caches, reference-counts,
  and reloads typed assets through `HoloDrive` without making HoloDisk depend
  on graphics or GUI packages; include mounting pinned nested archives such as
  GUI's `gdx-skins.zip` without extracting them into `.sq` package paths.
- [ ] Define cartridge metadata and compatibility validation.
- [ ] Add interrupted-write and recovery tests for scratch-backed changes.

## Later

- [ ] Evaluate explicit read-only mounts and atomic export workflows.
