# Squared HoloDisk — Programmer Guide

Squared HoloDisk is an optional, standalone ZIP cartridge extension. Through
the single `HoloDrive` boundary it creates empty emulated disks, loads
existing ZIP cartridges, mounts them into one virtual path namespace, opens
and streams files, routes writes through a drive-owned scratch overlay, and
materializes the current state back to a ZIP. It is independent of every
other Squared module; its only implementation dependency is the vendored miniz
source shipped inside the package.

## Package availability

| Module | Version | Requires |
| --- | --- | --- |
| `dev.squarednetizen.squared.holodisk` | `0.6.0-dev.3` | (none) |

The CMake target is `squared_holodisk`. The module is optional; the GUI does
not depend on it yet.

## Public API overview

| Type | Header | Purpose |
| --- | --- | --- |
| `squared::holodisk::ErrorCode` | `squared/holodisk/holodrive.hpp` | Stable failure categories. |
| `squared::holodisk::Error` | `squared/holodisk/holodrive.hpp` | One failure: code plus message. |
| `squared::holodisk::Result<T>` | `squared/holodisk/holodrive.hpp` | Value-or-error result. |
| `squared::holodisk::Status` | `squared/holodisk/holodrive.hpp` | Success-or-error result. |
| `squared::holodisk::DiskId` / `MountId` / `FileId` | `squared/holodisk/holodrive.hpp` | Opaque drive-owned identities. |
| `squared::holodisk::MountAccess` | `squared/holodisk/holodrive.hpp` | Read-only or read-write mount. |
| `squared::holodisk::OpenMode` | `squared/holodisk/holodrive.hpp` | Read, truncate-write, read-write, append. |
| `squared::holodisk::SeekOrigin` | `squared/holodisk/holodrive.hpp` | Begin, current, or end seek reference. |
| `squared::holodisk::DriveOptions` | `squared/holodisk/holodrive.hpp` | Resource and scratch policy fixed at creation. |
| `squared::holodisk::WriteOptions` | `squared/holodisk/holodrive.hpp` | Materialization compression and replacement policy. |
| `squared::holodisk::Entry` | `squared/holodisk/holodrive.hpp` | One listed directory child. |
| `squared::holodisk::HoloDrive` | `squared/holodisk/holodrive.hpp` | Sole operational boundary. |
| `squared::holodisk::HoloDriveFactory` | `squared/holodisk/holodrive.hpp` | Optional extension interface that creates a drive. |
| `make_standard_holodrive_factory` | `squared/holodisk/holodrive.hpp` | Create the standard ZIP-backed factory. |
| `AssetHandle<T>` / `AssetLoader<T>` | `squared/holodisk/asset_manager.hpp` | Immutable typed handle and loader strategy. |
| `AssetManagerOptions` | `squared/holodisk/asset_manager.hpp` | Cache, byte, and dependency-depth limits. |
| `AssetLoadContext` | `squared/holodisk/asset_manager.hpp` | Bounded source reads and typed dependency requests. |
| `AssetManager` | `squared/holodisk/asset_manager.hpp` | Application-owned typed cache and nested-archive owner. |

## Creating a drive

`make_standard_holodrive_factory()` returns the standard factory. The factory
copies `DriveOptions` at `create()` time; the options fix the scratch
directory and every resource limit for the life of the drive.

```cpp
#include <squared/holodisk/holodrive.hpp>

#include <memory>

int main()
{
    auto factory = squared::holodisk::make_standard_holodrive_factory();
    if (!factory) {
        return 1;
    }

    squared::holodisk::DriveOptions options;
    options.scratch_directory = "scratch";   // created when missing
    options.maximum_open_files = 8;

    auto created = factory->create(options);
    if (!created) {
        return 2;   // InvalidArgument or Io; see created.error()
    }
    auto drive = std::move(created).value();
    return 0;
}
```

`create()` fails with `ErrorCode::InvalidArgument` when `scratch_directory` is
empty or any limit is zero, and with `ErrorCode::Io` when the scratch
directory tree cannot be created. The returned drive exclusively owns all
subsequent disks, mounts, files, and scratch state until it is destroyed.

## Disks, mounting, and files

A disk is either emulated (`create_holodisk()`, no host file yet) or loaded
from a ZIP (`load_holodisk(location)`). Mount it at an absolute virtual path;
the namespace root `"/"` matches every path, and the longest matching mount
point wins for path resolution.

```cpp
#include <squared/holodisk/holodrive.hpp>

#include <array>
#include <cstddef>
#include <span>
#include <string>

int main()
{
    auto factory = squared::holodisk::make_standard_holodrive_factory();
    squared::holodisk::DriveOptions options;
    options.scratch_directory = "scratch";
    auto drive = std::move(factory->create(options)).value();

    auto disk = drive->create_holodisk();
    auto mount = drive->mount(
        disk.value(),
        "/game",
        squared::holodisk::MountAccess::ReadWrite
    );
    if (!mount) {
        return 1;
    }

    auto file = drive->open(
        "/game/config/settings.txt",
        squared::holodisk::OpenMode::ReadWrite
    );
    if (!file) {
        return 2;  // InvalidPath, NotFound, ReadOnly, Busy, LimitExceeded
    }

    const std::string text = "portable-holodisk";
    auto written = drive->write(
        file.value(),
        std::span<const std::byte>{
            reinterpret_cast<const std::byte*>(text.data()),
            text.size()
        }
    );

    auto rewind = drive->seek(file.value(), 0,
                              squared::holodisk::SeekOrigin::Begin);
    std::array<std::byte, 8> buffer{};
    std::size_t total = 0;
    while (true) {
        auto read = drive->read(file.value(), buffer);
        if (!read || read.value() == 0) {
            break;
        }
        total += read.value();
    }
    drive->close(file.value());
    drive->unmount(mount.value());
    return 0;
}
```

- `open(path, mode)` — `Read` requires the path to exist; writable modes
  require a `ReadWrite` mount, and only one writer may exist per path
  (`Busy` otherwise). The returned `FileId` must be released with `close()`.
- `read(file, span)` — reads up to the destination capacity at the current
  position and advances it; `0` means end of file. Writable-only handles
  fail with `InvalidArgument`.
- `write(file, span)` — writes the entire source through the scratch overlay
  or fails before advancing. Size limits are enforced
  (`LimitExceeded`); content reaches a ZIP only through `write_holodisk()`.
- `seek(file, offset, origin)` — the resulting position must lie within
  `[0, size]`, else `InvalidArgument`; returns the absolute position.
- `list(path)` — immediate children sorted by name, each with a single-segment
  name, directory flag, and size in bytes (0 for directories).

All paths are portable relative paths: absolute paths, backslashes, empty
segments, and `.` or `..` traversal segments are rejected with
`InvalidPath`. A virtual namespace path must be absolute and open as a file
(the namespace root itself is rejected).

## Materializing and cleanup

`write_holodisk(disk, destination, options)` materializes the current state
of one disk as a ZIP. It writes to a temporary file beside the destination and
installs it only after finalization succeeds; an existing destination is
rejected with `AlreadyExists` unless `WriteOptions::replace_existing` is set.
The disk must not have open files (`Busy`) and the destination parent must
exist (`NotFound`).

`discard_holodisk(disk)` forgets a disk's state; it fails with `Busy` while
the disk is mounted. Destroying the drive discards every unmaterialized
mutation, closes all files, and removes the drive's isolated scratch
directory.

```cpp
#include <squared/holodisk/holodrive.hpp>

int main()
{
    auto factory = squared::holodisk::make_standard_holodrive_factory();
    squared::holodisk::DriveOptions options;
    options.scratch_directory = "scratch";
    auto drive = std::move(factory->create(options)).value();

    auto disk = drive->create_holodisk();
    // ... write files through the scratch overlay ...

    squared::holodisk::WriteOptions write;
    write.compression_level = 6;
    auto materialized = drive->write_holodisk(
        disk.value(), "game.holodisk", write
    );
    if (!materialized) {
        return 1;  // Busy, InvalidArgument, AlreadyExists, NotFound, Io
    }

    auto reloaded = drive->load_holodisk("game.holodisk");
    auto mount = drive->mount(
        reloaded.value(), "/cartridge",
        squared::holodisk::MountAccess::ReadOnly
    );
    // reading works; writable opens fail with ReadOnly
    drive->unmount(mount.value());
    drive->discard_holodisk(reloaded.value());
    return 0;
}
```

## Typed asset loading

Construct an `AssetManager` after its `HoloDrive`; the drive must outlive the
manager. Register one loader for each C++ asset type. A loader reads bounded
source bytes or requests typed dependencies through its call-scoped
`AssetLoadContext`.

```cpp
#include <squared/holodisk/asset_manager.hpp>

#include <memory>
#include <string>

struct TextAsset final { std::string text; };

squared::holodisk::Status register_text(
    squared::holodisk::AssetManager& assets
)
{
    using namespace squared::holodisk;
    return assets.register_loader<TextAsset>([](
        AssetLoadContext& context,
        std::string_view path
    ) -> Result<AssetHandle<TextAsset>> {
        auto bytes = context.read_bytes(path);
        if (!bytes) return Result<AssetHandle<TextAsset>>::failure(bytes.error());
        return Result<AssetHandle<TextAsset>>::success(
            std::make_shared<TextAsset>(TextAsset{std::string(
                reinterpret_cast<const char*>(bytes.value().data()),
                bytes.value().size()
            )})
        );
    });
}
```

- `load<T>(path)` returns the cached handle when present, otherwise invokes
  the registered strategy. Missing strategies report `LoaderNotFound`.
- `AssetLoadContext::load<T>()` records a typed dependency. A dependency cycle
  reports `DependencyCycle`; depth and cache limits report `LimitExceeded`.
- `reload<T>()` builds a replacement transactionally. On failure the cached
  object is unchanged. Existing handles intentionally keep the old object,
  and cached dependents are not rebuilt automatically.
- `unload<T>()` reports `Busy` while cached dependents remain. External shared
  handles survive a successful unload. `clear()` drops all cache ownership.
- Paths are absolute virtual `HoloDrive` paths and receive the same traversal
  rejection. The manager is synchronous and single-threaded.

## Nested pinned archives

`mount_archive(source, point)` reads the source under
`maximum_asset_bytes`, validates it under the drive's archive limits, and
owns a new read-only mount. This is intended for assets such as a pinned
`gdx-skins.zip` stored inside another cartridge.

```cpp
using namespace squared::holodisk;

AssetManager assets(*drive);
auto mounted = assets.mount_archive("/packages/gdx-skins.zip", "/skins");
if (!mounted) return 1;

// Registered loaders can now read /skins/gdx-holo/uiskin.json.
assets.clear();
auto removed = assets.unmount_archive("/skins");
```

Unloading a nested mount reports `Busy` while the cache contains any typed
asset beneath it. The manager removes its nested mounts on destruction after
dropping its cache. Do not keep direct `HoloDrive` file handles open beneath a
manager-owned mount during manager destruction.

## Unsafe-path rejection

Path validation rejects anything that could escape the archive or virtual
namespace:

```cpp
#include <squared/holodisk/holodrive.hpp>

int main()
{
    auto factory = squared::holodisk::make_standard_holodrive_factory();
    squared::holodisk::DriveOptions options;
    options.scratch_directory = "scratch";
    auto drive = std::move(factory->create(options)).value();

    auto disk = drive->create_holodisk();
    auto mount = drive->mount(
        disk.value(), "/cartridge",
        squared::holodisk::MountAccess::ReadWrite
    );

    const auto traversal = drive->open(
        "/cartridge/../escape.txt",
        squared::holodisk::OpenMode::ReadWrite
    );
    if (!traversal &&
        traversal.error().code == squared::holodisk::ErrorCode::InvalidPath) {
        // ".." segments are rejected as unsafe
    }

    const auto outside = drive->open(
        "/cartridge/../../etc/passwd",
        squared::holodisk::OpenMode::ReadWrite
    );
    if (!outside &&
        outside.error().code == squared::holodisk::ErrorCode::InvalidPath) {
        // empty and "." traversal segments are also rejected
    }
    return 0;
}
```

## Errors and failure behavior

Every operation returns a `Result` or `Status`; nothing throws. A
default-constructed `Error` reports success (`ErrorCode::None`). `Result<T>`
holds either a value or an `Error`, never both; `value()` throws
`std::bad_optional_access` if a failure result is dereferenced, so always
test the result first. Stable `ErrorCode` categories:

| Code | Meaning |
| --- | --- |
| `InvalidArgument` | Supplied argument or option is out of range. |
| `InvalidPath` | Path failed archive or virtual namespace validation. |
| `NotFound` | Disk, mount, file, directory, or host path not found. |
| `AlreadyExists` | Mount point in use, or destination exists without replacement. |
| `LimitExceeded` | Disk, mount, open-file, entry, or size limit hit. |
| `InvalidArchive` | ZIP malformed, duplicated, or not decompressible. |
| `UnsupportedArchive` | ZIP has an unsupported or encrypted entry. |
| `ReadOnly` | Mount or file does not permit the requested write. |
| `Busy` | An open file or mount blocks the operation. |
| `InvalidHandle` | A `DiskId`, `MountId`, or `FileId` is zero or unknown. |
| `Io` | Host filesystem, scratch, or ZIP write/install failure. |
| `LoaderNotFound` | No loader is registered for the requested C++ asset type. |
| `DependencyCycle` | Typed asset dependencies contain a cycle. |
| `LoadFailed` | A loader threw, returned an empty handle, or failed construction. |

Code values are stable across releases so callers can switch on them.

## Ownership and lifetime

- The drive owns every disk, mount, file handle, and scratch file. Clients
  hold only opaque `DiskId`/`MountId`/`FileId` values, valid for the drive
  that issued them and until the corresponding object is discarded, closed,
  unmounted, or the drive is destroyed. Zero never matches a live object.
- `Result<T>` is value-semantic; when `T` is move-only (as with the drive
  out of `create()`), the result is move-only.
- `DriveOptions` is consulted (copied) only by `create()`; later option
  changes have no effect. `WriteOptions` is per-call.
- Loading a ZIP indexes metadata only; payload bytes are not expanded into
  RAM. The host file must remain readable for streamed reads while files of
  that disk are open.
- Loading from memory copies the compressed archive once into drive-owned
  storage. The caller's input span may be released when the call returns.
- `AssetManager` owns its cache and nested mounts but not its drive. Every
  `AssetHandle<T>` has shared immutable ownership independent of cache entry
  lifetime.

## Threading

A drive and its AssetManager are not thread-safe: confine both to one thread
or guard all calls with a single external lock. The standard factory is also
used single-threaded. Operational members report failures via return values;
the AssetManager constructor alone throws `std::invalid_argument` for a zero
resource limit.

## Lua bindings

None of the types or functions in this package have a Lua 5.4 binding.

## Related documentation

- Package payload: [HoloDisk.md](../../../packages/squared-holoDisk/content/docs/HoloDisk.md)
- Implementation details: [Squared HoloDisk — Developer Guide](../developer/squared-holoDisk/README.md)
- Documentation index: [Programmer documentation](../README.md)
