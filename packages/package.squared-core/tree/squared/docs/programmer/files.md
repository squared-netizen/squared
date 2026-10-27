# files

Virtual filesystem. A `FileHandle` names a location; a `FileSystem` decides
what that location actually is on this platform. Modelled on libGDX.

Developer counterpart: [../developer/files.md](../developer/files.md)

| Type | Header | Purpose |
|---|---|---|
| `FileHandle` | `squared/files/file_handle.hpp` | A named location, modelled on libGDX's FileHandle |
| `FileType` | `squared/files/src/file_type.hpp` | Where a path is rooted |
| `FileSystem` | `squared/files/file_system.hpp` | The framework's porting layer for storage |
| `PosixFileSystem` | `squared/files/posix_file_system.hpp` | Directories each FileType resolves to under a POSIX file system |
| `PosixFileSystemRoots` | `squared/files/posix_file_system.hpp` | Directories each FileType resolves to under a POSIX file system |
| `EmbeddedFileSystem` | `squared/files/embedded_file_system.hpp` | A read-only `Internal` over files compiled into the program |
| `EmbeddedFile` | `squared/files/embedded_file_system.hpp` | One compiled-in file: its path and its bytes |
| `FileReadResult` | `squared/files/src/file_read_result.hpp` | Bytes read from one file, or the failure that prevented it |
| `FileTextResult` | `squared/files/src/file_text_result.hpp` | Text read from one file, or the failure that prevented it |
| `FileError` | `squared/files/src/file_error.hpp` | Structured file system failure information |
| `FileErrorCode` | `squared/files/src/file_error_code.hpp` | Stable error categories produced by file system operations |

## The shortest call site

```cpp
sq::files::PosixFileSystem fs{ {
    .internal_root = app_directory + "/assets",
    .local_root    = app_directory + "/files",
    .external_root = "/sdcard/MyApp"
} };

auto skin = fs.internal("skins/default.json");
if (auto text = skin.read_string()) {
    load_skin(text.text);
}
```

## File types

The same relative path means a different file under each root, and the platform
decides what each root is:

| Type | Android | Termux and desktop |
|---|---|---|
| `Internal` | assets inside the APK, read-only | a directory, still read-only |
| `Local` | the app's private files directory | a directory |
| `External` | shared storage | a directory |
| `Absolute` | the path exactly as given | the same |

**`Internal` refuses writes on every platform**, even where it is an ordinary
writable directory. A build that works in Termux therefore behaves the same on
a device, instead of failing the first time someone runs the APK.

## Handles are values

A handle is 48 bytes: a file system pointer, a type, and a path string.
Constructing one touches nothing, so building handles in a loop costs only the
strings. Nothing is opened until an operation is called and nothing stays open
after one returns.

```cpp
auto atlas = skin.sibling("default.atlas");   // skins/default.atlas
auto dir   = skin.parent();                   // skins
auto page  = dir.child("pages/0.png");        // skins/pages/0.png
skin.name();                                  // default.json
skin.extension();                             // json
skin.name_without_extension();                // default
```

## Failures are returned

Nothing throws, and a missing file is an ordinary result rather than a
programmer error:

```cpp
auto bytes = handle.read_bytes();
if (!bytes) {
    log(bytes.error.code, bytes.error.message, bytes.error.path);
    return;
}
use(bytes.bytes);
```

What each operation returns when it fails, on `PosixFileSystem`:

| Operation | Fails with |
|---|---|
| any, on a path with `..` or a leading `/`, or a type with no root | `NotSupported` |
| `write`, `make_directories`, `remove` on `Internal` | `ReadOnly` |
| `read` of a missing file | `NotFound` |
| `read` of a directory | `IsADirectory` |
| `write` into a missing directory | `NotFound` (`write_bytes` on a handle makes the directories first) |
| `write` onto a directory | `IsADirectory` |
| `list` of a missing path, or of a file | `NotFound`, `NotADirectory` |
| `make_directories` where a file is in the way, or is the path | `NotADirectory` |
| `remove` of a missing path, or of a directory that is not empty | `NotFound`, `IoFailure` |

`exists()`, `is_directory()` and `length()` return plain values, because there
is no useful difference between "missing" and "could not tell". `length()`
returns zero for a missing file, a directory, and an empty file alike; use
`exists()` when the difference matters.

## Writing

```cpp
auto save = fs.local("saves/slot1.json");
if (auto error = save.write_string(document)) { report(error); }
```

Missing parent directories are created first, as libGDX does, so writing to a
fresh path needs no setup call.

**A save that must not be half written** (the process can die during it):

```cpp
if (auto error = fs.local("saves/slot1.json").write_atomically(document)) { report(error); }
```

`write_atomically( bytes )` and `write_atomically( text )` write beside the
file (`slot1.json.new`), flush it to storage, and rename it over the file:
whoever reads it gets the old contents or the new, whole. Missing parent
directories are created first. Failures as `write`'s, and on any failure the
file is as it was and nothing is left beside it. `Internal` refuses it
(`ReadOnly`), as it refuses every write. The same on Termux, a desktop and a
device: private storage is a directory on each.

**The platform's own path**, for code that outlives the file system it came
from (a thread that saves after the activity has gone):

```cpp
const std::string path = fs.resolve_platform_path(sq::files::FileType::Local, "saves/slot1.json");
```

Empty when there is none: bundled files inside an APK, compiled-in files, a
root that is not set, a path that escapes its root. Use it later with
`FileType::Absolute` through any file system.

## Listing

```cpp
std::vector<sq::files::FileHandle> entries;
if (!fs.local("saves").list(entries)) {
    for (const auto& entry : entries) { ... }
}
```

The caller owns the vector so it can be reused across calls rather than
allocating a new one each time, which is why this does not return a value.

## Paths that escape their root

A relative path containing `..`, or starting with `/`, is **refused** rather
than normalised, with `FileErrorCode::NotSupported`. Asking for something under
a root and quietly getting something outside it would be worse than failing.
Use `FileType::Absolute` when you mean an absolute path.

## Files compiled in: `EmbeddedFileSystem`

Small files the program cannot do without (the Stage's default skin is one)
can be compiled into it and read like bundled files, with nothing to
package. `tools/embed_files.sh` turns a directory into a C++ source: the
files' bytes as constants, and a table of `EmbeddedFile`s that a function
returns:

```sh
tools/embed_files.sh sq_app/data data my_data detail/my_data.hpp sq_app/src/my_data.cpp
```

(the directory, the path its files appear under, the function's name in
`sq::detail`, the header that declares it, the output). Then:

```cpp
sq::files::EmbeddedFileSystem files( sq::detail::my_data() );
const auto text = files.internal( "data/levels.json" ).read_string();
```

| Call | Answers |
|---|---|
| `exists`, `is_directory`, `length` | from the table: a directory is any path some file is under; only `Internal` holds anything |
| `read` | a copy of the bytes; `NotFound`, `IsADirectory`; `NotSupported` outside `Internal` |
| `list` | the names directly under a path, each once, sorted; `NotADirectory`, `NotFound` |
| `write`, `make_directories`, `remove` | `ReadOnly` |

The table must outlive the file system: a generated one is a constant. The
bytes are read-only data in the program, mapped from its file and shared,
not heap; a `read` copies them. Lookups scan the table, so keep it to a
handful of files.

## Nested assets on Android

Organise assets in as many directories as you like &mdash; reads work at any
depth, on every platform:

```cpp
fs.internal("audio/sfx/ui/click.ogg").read_bytes();
fs.internal("skins").list(entries);          // returns default/ on device too
```

On Android, listing and `is_directory()` are answered from an index the build
writes into the APK at `assets/SQ-INF/index`. You do not create or maintain
it; `make apk` regenerates it whenever an asset changes, and an app update
replaces it along with everything else, so it cannot go stale.

Two things to know:

- Keep `sq_android/assets/SQ-INF/index` in your `.gitignore`. It is a
  build product.
- Do not put assets under `assets/SQ-INF/`. It is the bundle's metadata
  directory, excluded from the index and hidden from listings &mdash; from
  `Internal`'s root listing in Termux and on a desktop too, so every platform
  lists the same names.

The details, and why it works this way rather than any of the obvious
alternatives, are in
[../developer/asset-index.md](../developer/asset-index.md).
