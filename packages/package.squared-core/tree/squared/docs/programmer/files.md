# files

Virtual filesystem. A `FileHandle` names a location; a `FileSystem` decides
what that location actually is on this platform. Modelled on libGDX.

Developer counterpart: [../developer/files.md](../developer/files.md)

| Type | Header | Purpose |
|---|---|---|
| `FileHandle` | `squared/files/file_handle.hpp` | A named location, modelled on libGDX's FileHandle |
| `FileType` | `squared/files/file_type.hpp` | Where a path is rooted |
| `FileSystem` | `squared/files/file_system.hpp` | The framework's porting layer for storage |
| `PosixFileSystem` | `squared/files/posix_file_system.hpp` | Directories each FileType resolves to under a POSIX file system |
| `PosixFileSystemRoots` | `squared/files/posix_file_system.hpp` | Directories each FileType resolves to under a POSIX file system |
| `FileReadResult` | `squared/files/file_read_result.hpp` | Bytes read from one file, or the failure that prevented it |
| `FileTextResult` | `squared/files/file_text_result.hpp` | Text read from one file, or the failure that prevented it |
| `FileError` | `squared/files/file_error.hpp` | Structured file system failure information |
| `FileErrorCode` | `squared/files/file_error_code.hpp` | Stable error categories produced by file system operations |

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

## Nested assets on Android

Organise assets in as many directories as you like &mdash; reads work at any
depth, on every platform:

```cpp
fs.internal("audio/sfx/ui/click.ogg").read_bytes();
fs.internal("skins").list(entries);          // returns default/ on device too
```

On Android, listing and `is_directory()` are answered from an index the build
writes into the APK at `assets/.squared/index`. You do not create or maintain
it; `make apk` regenerates it whenever an asset changes, and an app update
replaces it along with everything else, so it cannot go stale.

Two things to know:

- Keep `sq_android/assets/.squared/` in your `.gitignore`. It is a build
  product.
- Do not put your own files under `assets/.squared/`. That directory is
  squared's, and it is hidden from listings.

The details, and why it works this way rather than any of the obvious
alternatives, are in
[../developer/asset-index.md](../developer/asset-index.md).
