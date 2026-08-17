# Pinned gdx-skins assets

`tools/vendor-gdx-skins.fish` imports a complete gdx-skins ZIP only when its
SHA-256 matches an explicitly supplied pin. It rejects path traversal,
requires one archive root, verifies JSON, atlas, and PNG files, and
creates:

```text
packages/squared-gui/content/app/src/main/assets/gui/gdx-skins/
├── PIN.sha256
├── SOURCE.md
├── ASSET_INDEX.txt
├── gdx-skins.zip
└── selected/gdx-holo/
    ├── default.fnt
    ├── uiskin.atlas
    ├── uiskin.json
    └── uiskin.png
```

The `0.6.0-dev.9` update pins the archive SHA-256 to
`7d1b758af9d787b4cc4962f7e2d5c732dec743c0aaea09c32b0dbf9ed57b5c5b`.
The archive has no repository-wide license file, so the importer records its
source and checksum without requiring license metadata.

The original archive remains byte-for-byte intact. This avoids renaming its
non-portable internal paths and accidentally breaking JSON or atlas references.
The four runtime gdx-holo files have portable names and are projected beside
the archive so current asset APIs can load the selected theme directly. Other
skins remain archive-only until the HoloDisk-backed AssetManager is introduced.

Because this directory is under the GUI package's `content/app` tree,
`squared-pg` copies it into the internal assets of every generated project that
selects the GUI package. Projects do not need the original download after the
asset is vendored.

This milestone pins and distributes the source archive. It deliberately does
not interpret libGDX skin JSON at runtime. That loader remains in
`squared-gui/TODO.md` and will consume assets through the planned portable,
HoloDisk-backed AssetManager, which can mount the ZIP without exposing its
internal paths to the `.sq` package writer or platform file APIs.
