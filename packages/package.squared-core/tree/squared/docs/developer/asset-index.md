# The asset index

Programmer counterpart: [../programmer/files.md](../programmer/files.md)

## In one line

An APK's asset tree is described by a file inside the APK,
`assets/SQ-INF/index`, written by the build, replaced wholesale on every
update, and therefore never stale.

## Why it exists

Android assets are not files. They are entries in the APK's zip, reachable
only through `AAssetManager`. The NDK offers exactly one way to walk them,
`AAssetDir`, and it **lists files only** &mdash; subdirectories are invisible
to it.

That breaks any nested asset tree. Given

```
assets/skins/default/skin/uiskin.atlas
```

`AAssetDir` on `skins/` returns nothing, because `skins/` contains only the
subdirectory `default/`. So `list()` came back empty and `is_directory()`
reported that `skins` was not a directory at all.

libGDX avoids this by calling the Java `AssetManager.list()`, which does see
subdirectories. squared has no Java layer, by decision, so it cannot. The
information has to come from somewhere else.

**Reading was never affected.** `AAssetManager_open` takes a full path at any
depth. `fs.internal("audio/sfx/ui/click.ogg").read_bytes()` works identically
with or without an index. Only *directory* questions need it: `list()`,
`is_directory()`, and `exists()` on a directory.

## What it is

A plain text file, one asset path per line, relative to the asset root, `/`
separated, files only:

```
audio/sfx/ui/click.ogg
skins/default/skin/default.fnt
skins/default/skin/uiskin.atlas
skins/default/skin/uiskin.json
skins/default/skin/uiskin.png
```

Directories are not listed; they are implied by the paths. `skins/` exists
because some path begins `skins/`. That is what lets a directory holding only
directories be recognised.

Lines are sorted with `LC_ALL=C sort`, so the file is byte-for-byte
reproducible: an unchanged asset tree produces an identical index and an
identical APK.

## Where it lives, and why there

`assets/SQ-INF/index` &mdash; inside the APK, in the bundle's metadata
directory.

**Inside the APK** because the index describes the APK. The information only
exists at build time, when the full tree is on disk. On the device, generating
it would require exactly the directory walk `AAssetDir` cannot do, so it must
be carried in rather than computed.

**In `SQ-INF/`** because the index is metadata *about* the asset bundle, not
content in it. `SQ-INF` is the same convention sqcart uses for
`SQ-INF/manifest.json`, after Java's `META-INF`: one directory per bundle that
describes the bundle. Everything in it is excluded from the index and hidden
from listings of the root, and an uppercase hyphenated name is very unlikely
to collide with an application's own assets.

It is unrelated to the `META-INF/` at the root of every APK, which holds the
signatures. That sits at the zip root; `SQ-INF/` sits under `assets/`.

### Why not a hidden directory

The first version put the index at `assets/.squared/index`. It was generated,
passed to `aapt2 link -A`, and **never reached the APK**: aapt2 silently skips
hidden files and directories when packaging assets. `unzip -l` on the built
APK showed all thirty-two skin files and no index.

aapt's default ignore pattern also skips directories whose names begin with an
underscore, so `_SQ-INF/` would fail the same way. The name must start with
neither.

Nothing broke visibly when the index was missing: opening a file by full path
never uses it, so the launch check still passed. Only directory questions
degraded. That is why it went unnoticed until the APK was inspected, and why
the location is now verified by listing the APK rather than assumed.

## Why it is never stale

This is the property that decided the location.

An app update replaces the whole APK. The new APK carries its own index,
generated from its own assets in the same packaging step. The index and the
tree it describes are always the same version, because they are in the same
file, built at the same moment. There is nothing to update separately and
nothing that can fall out of step.

On the build side, the index's make rule depends on every asset file. Adding,
removing or renaming any asset regenerates it on the next `make apk`.

## Alternatives that were rejected

**The app's internal storage or cache.** Two independent reasons against it.

It cannot originate the index: building it on the device needs the directory
walk the index exists to replace. It could only hold a copy of one shipped in
the APK, which is the same data in two places.

And **internal storage survives app updates** &mdash; it is cleared only on
uninstall or "Clear data". A cached index would outlive the APK that produced
it. After an update the new assets are installed while the old index still
describes the old tree: `list()` returns files that no longer exist and misses
ones that were added, silently. Preventing that means version-stamping the
cache and invalidating on mismatch, which is machinery to solve a problem the
APK location does not have.

**Compiled into the application's `.so`** as a generated `constexpr` array.
Rejected: every asset change would force a recompile and relink, and the
framework would need the application to hand it the array at construction.
More coupling than the problem justifies.

**Java `AssetManager.list()` through JNI.** This is what libGDX does. Rejected
because squared is pure NativeActivity with no Java, and because a build-time
index answers without a JNI round trip per directory.

**The asset root, as `assets/.index`.** Works, but sits among the
application's own files and could collide with one of them. The namespaced
directory costs nothing extra.

## How it is read

**Per call, never cached.** Each directory question reads the index afresh and
keeps nothing resident afterwards. A large game's index is tens of kilobytes
that would otherwise stay in memory for a question usually asked once at
startup. Android caches asset reads underneath, so a repeated read is cheap.
Anything that lists in a hot path should cache its own answer.

## Without an index

An APK built by hand, or by a Makefile predating the index, has none. Nothing
fails. Every directory question falls back to `AAssetDir`: files still list
and read at any depth, and subdirectories do not appear &mdash; exactly the
behaviour before the index existed. Degraded, not broken.

## Generating it

Packaging writes it. In the template's `mk/squared_android_package.mk`:

```make
SQ_ASSET_INDEX := $(SQ_ASSETS_DIR)/SQ-INF/index

$(SQ_ASSET_INDEX): $(SQ_ASSET_FILES)
	@mkdir -p $(dir $@)
	@cd $(SQ_ASSETS_DIR) && find . -type f ! -path './SQ-INF/*' \
	  | sed 's|^\./||' | LC_ALL=C sort > SQ-INF/index
```

and make the APK depend on `$(SQ_ASSET_INDEX)` so it is regenerated before
`aapt2 link -A $(SQ_ASSETS_DIR)` packages the directory.

The `! -path './SQ-INF/*'` matters: without it the index would list itself,
and any other bundle metadata as though it were an asset.

When there are no real assets the build removes only the index file, and
`SQ-INF/` only if that leaves it empty. It never deletes a file it did not
write &mdash; a hand-written manifest in `SQ-INF/` survives.

## In a generated project

The index is a build product that happens to sit in the source tree, so it
belongs in the project's `.gitignore`:

```
sq_android/assets/SQ-INF/
```

It is regenerated on every packaging run; committing it would only produce
noise when assets change, and a stale committed copy would be overwritten
anyway. Nobody should edit it by hand.

## On the desktop

`PosixFileSystem` serves `Internal` from a real directory, where `SQ-INF/`
is an ordinary folder and will appear in a listing of the root if it has been
generated there. That is deliberate and harmless: the desktop and Termux build
environments are not where assets are consumed, the finished APK is. The
index only ever matters to `AndroidAssetFileSystem`.
