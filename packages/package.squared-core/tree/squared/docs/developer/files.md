# files &mdash; internals

Virtual filesystem: nine public types, three translation units.

Programmer counterpart: [../programmer/files.md](../programmer/files.md)

- Public types: 9
- Translation units: 3

## Why this module exists

Every other subsystem in squared takes bytes it is handed. There is no `fopen`,
no `ifstream` and no `AAssetManager` anywhere in `gui`, `graphics2d`, `scene2d`
or `data` &mdash; `load_libgdx_skin` takes a `std::string_view`, fonts parse
from bytes. This module is what hands them those bytes without any of them
learning where storage comes from.

## `FileSystem` is a deliberate interface

Eight pure virtuals, in a framework whose policy is to refuse extension points
until a second concrete use exists. This is one of the cases the policy names:
Android `Internal` storage is an asset bundle inside the APK and is not
reachable through POSIX at all, while Termux and desktop Linux are ordinary
directories. The second implementation is guaranteed by the architecture, not
hoped for. See [extension-policy.md](extension-policy.md).

The virtuals take `(FileType, std::string_view)` rather than a `FileHandle`, so
a backend never needs the handle type. `FileHandle` in turn only forward
declares `FileSystem`, so the two headers do not form a cycle.

## `FileHandle` is 48 bytes

A `FileSystem*`, a `FileType`, and a `std::string`. Non-owning: the file system
must outlive every handle resolved from it, the same contract
`AssetManager(HoloDrive&)` uses.

Convenience that belongs to every backend lives here rather than in one:
`write_bytes` creates missing parent directories before delegating, matching
libGDX. That costs one directory walk per write. Priority 2 over priority 3,
and it allocates nothing beyond the resolved path, so priority 1 is untouched.

`list()` fills a caller-owned `std::vector<FileHandle>` instead of returning
one. That is a priority-1 choice &mdash; the vector can be reused across calls
&mdash; and it also avoids a genuine cycle, since a result type holding
`std::vector<FileHandle>` would need the complete handle while the handle would
need the complete result type.

## `PosixFileSystem`

The backend for Termux, desktop Linux, and Android `Local` and `External`.
`stat`, `fopen`, `opendir`, `mkdir`, `unlink`, `rmdir`. No allocation beyond
the resolved path string and the read buffer.

Three behaviours worth keeping:

**`Internal` refuses writes even though it is a writable directory here.** The
alternative is a build that works in Termux and fails on a device, which is the
worst possible place to discover the difference.

**Paths that escape their root are refused, not normalised.** A leading `/` or
any `..` segment returns `NotSupported`. Silently resolving outside the root
the caller asked for is a security problem, not a convenience.

**A file can shrink between the size query and the read**, so `read()` resizes
the buffer down to what `fread` actually returned rather than trusting `stat`.

## AndroidAssetFileSystem

The second implementation the interface was shaped for, and it arrived as one
new `.cpp` with no existing file changed &mdash; which is the property that
was the point.

`Internal` reads through `AAssetManager`, because assets inside an APK are zip
entries and no POSIX call can reach them. Everything writable is an ordinary
directory and is delegated to an owned `PosixFileSystem`, whose
`internal_root` is deliberately left empty so a delegated `Internal` call
fails rather than silently resolving on the filesystem.

**The naming trap, stated once.** Android's "internal storage" is this class's
`Local`; this class's `Internal` is the read-only asset bundle, which Android
does not call storage at all. `ANativeActivity::internalDataPath` maps to
`Local`, `externalDataPath` to `External`.

Three things in the implementation are not obvious:

**`AAsset_read` returns short counts.** A compressed asset is decompressed in
chunks and one call does not fill the buffer, without that being an error. The
read loops. A single call would truncate every compressed skin atlas, and it
would work perfectly on an uncompressed one &mdash; which is exactly the kind
of difference that only shows up on someone else's build.

**`AASSET_MODE_STREAMING`, not `BUFFER`.** The asset is read once into a vector
the caller owns; asking the platform to buffer or mmap it as well would hold
two copies of a texture atlas at the same moment.

**Directories come from a build-time index** &mdash; the full reasoning,
including every alternative that was rejected and why, is in
[asset-index.md](asset-index.md). `AAssetDir` &mdash; the NDK's
only way to walk assets &mdash; lists files and never subdirectories. libGDX
sidesteps this by calling the Java `AssetManager.list()`; squared has no Java.
So packaging writes every asset path into `assets/.squared/index`,
and `list()`, `is_directory()` and `exists()` answer directory questions from
it.

That fixes a real bug, not just a gap: with `AAssetDir` alone, a directory
holding only subdirectories &mdash; `skins/` holding `default/` &mdash;
reported as not a directory at all. With nested asset trees that is the normal
case.

The index is read **per call**, not cached. A large game's index is tens of
kilobytes that would otherwise stay resident for a question asked once at
startup; Android has its own caching layers, and anything that lists hot
should cache its own answer.

**`.index` lives in the assets root**, at `sq_android/assets/.index`, beside
the assets it describes. It is generated by packaging on every build, so:

- it is never edited by hand; a hand edit is overwritten on the next build
- a generated project should list `sq_android/assets/.index` in its
  `.gitignore`, since it is output rather than source
- it is one path per line, relative to the assets root, files only &mdash;
  directories are derived from the paths &mdash; sorted with `LC_ALL=C` so an
  unchanged tree produces a byte-identical index and an unchanged APK

`.index` hides itself from every listing, on both backends.
`AndroidAssetFileSystem` skips it on device; `PosixFileSystem` skips it when
listing the root of `Internal`, because there it is an ordinary file sitting in
the directory. Without the second, Termux would list a file the phone does not,
and `Internal` would stop giving the same answer in both places. Without an index &mdash; an APK
built by hand, or by an older Makefile &mdash; everything falls back to
`AAssetDir`: files still list and read at any depth, subdirectories just do
not appear. Degraded rather than broken.

Reading is unaffected by any of this. `AAssetManager_open` takes a full path
at any depth, so `audio/sfx/ui/click.ogg` reads identically with or without an
index.

The header forward declares `AAssetManager` and includes no NDK header, so it
passes the standalone self-containment check on any platform; only the `.cpp`
needs `<android/asset_manager.h>`.

Built only when `SQUARED_PLATFORM=android`. That is a separate variable from
`SQUARED_GRAPHICS_BACKEND` on purpose: "has GLES" and "is Android" coincide
today and are not the same question.

## Not yet written

Nothing in this module.

An `AssetManager` above this module is the other half: this layer answers
"give me these bytes", and that one answers "give me this thing, as this type,
and do not load it twice".
