# Squared HoloDisk — Developer Guide

Squared HoloDisk is the optional, standalone ZIP cartridge drive of the
framework. A single operational interface, `HoloDrive`, fronts an emulated
and ZIP-backed backend whose disk images, mounts, host files, and streams stay
private. Writes are buffered in a drive-owned scratch overlay and reach a ZIP
only through explicit materialization, which is transactional. The package
is independent: it requires no other Squared module and relies only on the
vendored miniz 3.1.2 source shipped inside the package.

- Programmer counterpart: [Squared HoloDisk — Programmer Guide](../programmer/squared-holoDisk/README.md)
- Package payload: [HoloDisk.md](../../packages/squared-holoDisk/content/docs/HoloDisk.md)
- Documentation index: [Developer documentation](../README.md)
- Diagrams:
  - [Drive creation, mount, and unmount lifecycle](drive-lifecycle.dot)
  - [ZIP read and scratch-write flow](zip-scratch-flow.dot)
  - [Path validation and rejection flow](path-validation.dot)

## Dependency boundary

The manifest declares no `module.requires` entry: the package is standalone
and optional. The CMake target `squared_holodisk` is a `STATIC` library that
compiles `src/holodrive.cpp` together with the vendored miniz sources
(`miniz.c`, `miniz_tdef.c`, `miniz_tinfl.c`, `miniz_zip.c` from
`content/third_party/miniz-3.1.2`) under `MINIZ_NO_ZLIB_APIS`. The miniz
include directory is `PRIVATE`; the public header
`include/squared/holodisk/holodrive.hpp` includes only C++ standard headers
(`<cstddef>`, `<cstdint>`, `<memory>`, `<optional>`, `<span>`, `<string>`,
`<string_view>`, `<utility>`, `<vector>`).

The GUI and other core packages must not depend on HoloDisk before an explicit
integration milestone; no package currently requires it.

## Architecture

| Component | Source | Responsibility |
| --- | --- | --- |
| `HoloDrive` (interface) | `include/squared/holodisk/holodrive.hpp` | Sole operational API; every member returns `Result`/`Status`. |
| `HoloDriveFactory` (interface) | `include/squared/holodisk/holodrive.hpp` | Backend selection separated from operation. |
| `make_standard_holodrive_factory` | `src/holodrive.cpp` | Returns the standard ZIP-backed factory. |
| `StandardHoloDrive` | `src/holodrive.cpp` (private) | Disks, mounts, files, scratch overlay, materialization. |
| `StandardHoloDriveFactory` | `src/holodrive.cpp` (private) | Option validation and scratch root creation. |
| `ZipReader` | `src/holodrive.cpp` (private) | RAII `mz_zip_archive` reader wrapper. |
| `DiskState` / `MountState` / `FileState` | `src/holodrive.cpp` (private) | Per-object bookkeeping. |
| `ErrorCode` / `Error` / `Result<T>` / `Status` | `include/squared/holodisk/holodrive.hpp` | Dependency-free failure and result channels. |

## Ownership and threading

- The drive owns three handle-indexed maps — `disks_`, `mounts_`, `files_` —
  keyed by monotonically increasing `std::uint64_t` ids (`next_disk_`,
  `next_mount_`, `next_file_`, all starting at 1). A zero handle never matches.
- `files_` holds `std::unique_ptr<FileState>`; `FileState` itself owns a
  `std::fstream` overlay and a `std::unique_ptr<ZipReader>`. Its destructor
  frees the live `mz_zip_reader_extract_iter_state*` iterator, so stream
  resources are released on `close()` or drive destruction.
- `ZipReader` wraps `mz_zip_reader_init_file`/`mz_zip_reader_end` with RAII;
  every archive access goes through it or a fresh per-file reader.
- The factory creates the scratch root under `scratch_directory` (a unique
  `squared-holodrive-<n>` directory guarded by a static atomic sequence) and
  the drive destructor does `remove_all` on it, discarding every
  unmaterialized mutation. The options are copied at creation; later mutation
  has no effect.
- A drive is not thread-safe: confine it to one thread or guard all calls with
  a single external lock. `guard`/`guard_status` translate a private `Failure`
  exception (and any other `std::exception`) into the corresponding `Error`,
  keeping every public member `noexcept`.

## Invariants and failure behavior

- Validation invariants: `scratch_directory` non-empty and every
  `DriveOptions` limit strictly positive (enforced by the factory);
  compression level in `[0, 9]`; handles non-zero and present in the owning
  map; mount points absolute; archive paths relative, portable, and free of
  empty/`.`/`..` segments; one writer per path; `ReadOnly` mounts refuse
  writable opens; a disk cannot be unmounted-while-mounted, discarded-while-
  mounted, or materialized while any of its files are open.
- Load validation: each ZIP entry must be supported, unencrypted, named
  (`InvalidArchive`/`UnsupportedArchive` otherwise), within
  `maximum_file_size` and `maximum_expanded_size`, and have no duplicate
  normalized path. Expanded size accumulates across entries.
- Capacity invariants: live disks, mounts, and open files are bounded by the
  corresponding options; entry count by `maximum_entries_per_disk`.
- Materialization transactionality: `write_holodisk` writes to a unique
  `*.squared-tmp-*` sibling, finalizes the ZIP, then installs — `rename` when
  the destination is free, or a backup `*.squared-backup-*` plus rename with
  rollback when replacing. Any failure removes the temporary and leaves the
  destination untouched.
- The expanded-size accounting recomputes the whole disk total (archive plus
  overlay entries, replacing overlaid names) before committing a write.
- Error codes are stable across releases; a default `Error` means success
  (`ErrorCode::None`).

## Data structures and complexity

- `disks_`, `mounts_`, `files_` are `std::map<std::uint64_t, ...>` with
  monotonic ids: lookup and erasure are O(log n) per handle operation.
- Each `DiskState` keeps `archive_entries` and `overlay_entries` as
  `std::map<std::string, ...>` keyed by normalized path: sorted, so `list`
  aggregates children by iterating both maps and collating into a
  `std::map<std::string, Entry>` (O(n log n) for the collation, O(n) result).
- `list` derives directory children from prefixes, so a single path is
  resolved by longest mount-point match — O(mounts) per resolve, O(entries)
  per list.
- `ensure_expanded_size` recomputes the total with a `std::set<std::string>`
  of names: O(entries) per write. This is a correctness-preserving but
  non-incremental accounting choice.
- ZIP payloads are never loaded into RAM; `read` streams from the archive
  iterator or from the overlay `fstream`, O(1) per call plus the compressed
  read cost. `reset_archive_position` reinitializes the reader and discards
  forward through the entry, which is O(target position) on a seek.

## Algorithms and execution order

`StandardHoloDriveFactory::create`:

1. Validate `scratch_directory` and all limits (`InvalidArgument`).
2. `create_directories` the scratch root and a unique isolated drive
   directory (`Io` on failure).
3. Return `Result<std::unique_ptr<HoloDrive>>::success(...)`.

`load_holodisk(location)`:

1. Resolve to an absolute host path; a missing file is `NotFound`.
2. Open a `ZipReader`; enumerate entries: validate support/encryption, name
   length and contents, per-file and expanded size limits, and duplicate
   paths (normalizing directory names). Directory entries are validated but
   not stored.
3. Store the index and return a new `DiskId`. Payload bytes are not copied.

`mount`/`unmount`/`open`:

1. `mount` resolves the disk, normalizes the mount point (absolute virtual
   path), checks uniqueness and the mount limit, and records the access mode.
2. `open` resolves the path against the mounts (longest matching point wins),
   rejects a mount root opened as a file, applies the writable/read-only and
   single-writer rules, then prepares the file: writable modes
   `prepare_overlay` (extract the archived entry into scratch, or create an
   empty scratch file, or truncate for `WriteTruncate`), a read-only open
   prefers an existing overlay entry, otherwise an archive stream.
3. `unmount`/`discard_holodisk` refuse while files (respectively mounts)
   are open.

`read`/`write`/`seek`:

1. `read` checks readability, caps the transfer by `size - position`, then
   streams from the overlay or the archive iterator, advancing position by the
   actual count. A short compressed read before end-of-file is
   `InvalidArchive`.
2. `write` checks writability and both size limits before writing, then
   updates `position`, `size`, and the overlay entry size. `Append` sets the
   initial position to the logical size at open.
3. `seek` validates the resulting position against `[0, size]`, and for an
   archive-backed file resets and re-discards the iterator to the target.

`write_holodisk(disk, destination, options)`:

1. Reject empty destination or out-of-range compression (`InvalidArgument`);
   reject open files of the disk (`Busy`); reject an existing destination
   without `replace_existing` (`AlreadyExists`); require the parent directory
   (`NotFound`).
2. `write_archive` opens a writer on the temporary path, copies unchanged
   archive entries via `mz_zip_writer_add_from_zip_reader` and overlay entries
   via a read callback, finalizes, and closes; `mz_zip_writer_end` runs on
   failure.
3. `install_archive` renames into place (or backs up, renames, and removes the
   backup with rollback on failure). The temporary is removed on any error.

## Design patterns

- **Facade** — `HoloDrive` is the sole operational boundary over ZIP parsing,
  host filesystem access, and scratch management; disk images, mounts,
  streams, and miniz detail never reach the client.
- **Factory** — `HoloDriveFactory` (and `make_standard_holodrive_factory`)
  separates backend selection from operation so application code can swap the
  drive backend without touching the operational interface.
- **Adapter/RAII over miniz** — `ZipReader` wraps the C-style
  `mz_zip_archive` lifetime; `FileState` wraps the extract iterator. miniz is
  the pinned third-party boundary, confined to `src/holodrive.cpp`.
- **Overlay (copy-on-write scratch)** — writes go to drive-owned scratch
  files; the ZIP is never mutated in place, and `write_holodisk` materializes
  the merged view. This gives cheap mutation and explicit transactional
  persistence.
- **Validation chain** — `normalize_archive_path` and `normalize_virtual_path`
  reject unsafe segments in a single pass before any namespace operation, so
  traversal defense is centralized.
- **Guard** — `guard`/`guard_status` map an internal `Failure` (and any
  exception) to returned `Error`s, keeping the public surface `noexcept`.
- **Value/error result** — `Result<T>`/`Status` make the no-throw contract
  structural instead of relying on errno-style globals.

Alternatives rejected: a direct miniz API surface (rejected — leaks the
third-party boundary); expanding archives into RAM (rejected — untrusted
cartridges must be bounded and reads streamed); in-place ZIP mutation
(rejected — breaks transactionality); a full virtual filesystem abstraction
(rejected — `TODO.md` keeps an asset manager as future work without making
HoloDisk depend on graphics or GUI).

## Limitations and technical debt

- Single virtual path namespace: one namespace with longest-prefix mount
  resolution; multiple overlapping namespaces are not modeled.
- `ensure_expanded_size` recomputes the whole disk per write (O(entries)),
  and archive seeks re-read from the entry start; fine for current sizes,
  not for large-file random access.
- `TODO.md` records the next AssetManager design (resolving/caching typed
  assets through `HoloDrive`, including mounting pinned nested archives such
  as GUI's `gdx-skins.zip`), cartridge metadata/compatibility validation, and
  interrupted-write recovery tests as unfinished.
- No explicit read-only-mount option beyond the per-mount `MountAccess`, and
  no atomic export workflow yet (`TODO.md` "Later").
- Materialization rewrites the whole disk; incremental or append-only export
  is not implemented.
- miniz 3.1.2 is a pinned vendored dependency; upgrades require re-running
  the unsafe-path, limit, cleanup, and malformed-cartridge tests.
