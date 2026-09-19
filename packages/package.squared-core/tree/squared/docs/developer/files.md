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

## Not yet written

The Android asset backend for `FileType::Internal`, over `AAssetManager`. It is
a second `.cpp` added to `files/Makefile` &mdash; no existing file changes,
which is the property the interface was shaped to give.

An `AssetManager` above this module is the other half: this layer answers
"give me these bytes", and that one answers "give me this thing, as this type,
and do not load it twice".
