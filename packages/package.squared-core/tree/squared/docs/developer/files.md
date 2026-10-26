# files &mdash; internals

Virtual filesystem: nine public types, three translation units.

Programmer counterpart: [../programmer/files.md](../programmer/files.md)

- Public types: 9
- Translation units: 3

## Why this module exists

Every other subsystem in squared takes bytes it is handed. There is no `fopen`,
no `ifstream` and no `AAssetManager` anywhere in `gui`, `graphics`
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
`stat`, `open`/`fstat`/`read`, `open`/`write`, `opendir`, `mkdir`, C's
`remove` (a file or an empty directory). **One allocation per call**, the
resolved path, reserved to its final size, plus `read`'s buffer; the test
counts them (six everyday calls, seven allocations). Every operation starts
with one helper, `target()`, which applies the root, the escape rule and the
read-only rule and builds the error; before 2026-10-19 that check was written
out six times.

**`std::filesystem` and `std::fstream` were passed over (2026-10-19, measured
on the phone).** The rule is to use them in framework file code when they save
code and state, unless a priority says otherwise; here priority 1 does. Three
versions were built and compared (`sq-compare-fs`: the same 48 checks, the
demo's real link, NDK 29, libc++ static as in the APK):

| | before | this one (POSIX) | `<filesystem>` + `<fstream>` |
|---|---|---|---|
| statements | 149 | 138 | 91 |
| `libdemo.so`, stripped | 1,110,896 | 1,110,152 | 1,723,944 (+613,048) |
| allocations / iteration | 12 | 9 | 79 |
| bytes allocated / iteration | 5,040 | 4,784 | 19,568 |
| microseconds / iteration | 149 | 137 | 145 |

Linking either library pulls in most of libc++'s locale and stream code (each
alone added about 600 KB to a small library, both together not much more), and
the demo had none of it. A third less source does not pay for 55% more
library in every app. A second reason came out of the comparison: the same
`std::filesystem` call answered differently on the two standard libraries
(`create_directories` over a file: `AlreadyExists` on libc++,
`NotADirectory` on libstdc++), so it would have needed code of its own to keep
Termux, a desktop and a device saying the same thing. Tools and tests, which
link the platform's shared C++ library, may use both freely. Revisit if
streams arrive for another reason (data processing was raised and deferred):
once they are linked, `<filesystem>` costs about 67 KB more.

Behaviours worth keeping:

**`Internal` refuses writes even though it is a writable directory here.** The
alternative is a build that works in Termux and fails on a device, which is the
worst possible place to discover the difference.

**Paths that escape their root are refused, not normalised.** A leading `/` or
any `..` segment returns `NotSupported`. Silently resolving outside the root
the caller asked for is a security problem, not a convenience.

**A file can shrink between the size query and the read**, so `read()` loops
until `read` returns 0 and keeps what arrived, rather than trusting `fstat`.
It asks `fstat` on the open file, so the size and the "is a directory" answer
are about the file it reads.

**Reading a directory fails with `IsADirectory`, and `make_directories` over a
file with `NotADirectory`.** Until 2026-10-19 both returned success (the first
with no bytes): `fopen` opens a directory on Linux and `length()` said 0, and
`mkdir`'s `EEXIST` was accepted for the last part as for the others. Found by
the comparison's checks; now in `test/posix_file_system_test.cpp`.

**Writes are checked to the end:** a short `write` is continued, `EINTR` is
retried, and `close`'s result counts, because a file system that delays
writes reports a failed one there.

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
So packaging writes every asset path into `assets/SQ-INF/index`,
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

**The index lives at `sq_android/assets/SQ-INF/index`**, in the bundle's
metadata directory beside the assets it describes ([asset-index.md](asset-index.md)
has why there and not a hidden file, which aapt2 drops). It is generated by
packaging on every build, so:

- it is never edited by hand; a hand edit is overwritten on the next build
- a generated project should list `sq_android/assets/SQ-INF/index` in its
  `.gitignore`, since it is output rather than source
- it is one path per line, relative to the assets root, files only &mdash;
  directories are derived from the paths &mdash; sorted with `LC_ALL=C` so an
  unchanged tree produces a byte-identical index and an unchanged APK

`SQ-INF/` hides itself from root listings, on both backends.
`AndroidAssetFileSystem` skips it on device; `PosixFileSystem` skips it when
listing the root of `Internal`, because there it is an ordinary directory.
Without the second, Termux would list a directory the phone does not, and
`Internal` would stop giving the same answer in both places. (Until
2026-10-19 this page said the POSIX side did this and it did not; the test
checks it now.) Without an index &mdash; an APK
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

## `EmbeddedFileSystem` (7.4)

The third implementation, made for the Stage's default skin
([stage.md](stage.md#the-default-skin-74)): the skin's files are compiled
into the stage library, and reading them through a `FileSystem` keeps every
loader (`TextureAtlas`, `BitmapFont`, the skin loader) and a texture's
reload after a lost context (`ReloadFromAsset` keeps its `FileHandle`)
exactly as they are for bundled files. Rejected: from-memory overloads in
graphics, which would have doubled each loader's surface and given textures
a second reload path.

24 bytes: the vtable pointer and a span over a table of `EmbeddedFile`s (a
`string_view` path and a `span<const unsigned char>`, 32 bytes each, in
relocated read-only data). Only `Internal` holds anything. Every lookup is a
linear scan, O(n) in the files, which is the right cost for the handful it
holds; a directory is any prefix some path continues with `/`, so there is
no directory list to keep. `read` copies the bytes into the result's
vector, as every backend does; `list` builds its names, sorts them and
drops repeats. Writing returns `ReadOnly`.

**`tools/embed_files.sh`** writes the table: `od -An -v -t x1` and `sed`,
16 bytes to a line as `unsigned char` arrays, files in byte order of their
names, then the table and one function returning it, inside
`// clang-format off` (the formatter would repack the lists). It writes the
file only when it changes. POSIX tools only, so it runs in Termux. The
output is committed: no build step, no host tool, and the source tree
builds anywhere; a test where it is used (the stage test, for the skin)
checks the output against the files.

Tested by `files/test/embedded_file_system_test.cpp`: lookups (a prefix
that is not a directory, the root, other file types), reads (bytes with
zeros, an empty file, the errors), listing (direct children, each once,
sorted; a file; a missing path), the refusals, and reading through a
handle.

## Not yet written

Nothing in this module.

An `AssetManager` above this module is the other half: this layer answers
"give me these bytes", and that one answers "give me this thing, as this type,
and do not load it twice".
