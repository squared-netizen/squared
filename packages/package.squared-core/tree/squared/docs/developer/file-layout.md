# File layout

Where files go inside a module, what each one is to the compiler, and the
rules that keep each one small.

Programmer counterpart: [../programmer/include-layout.md](../programmer/include-layout.md).
How the build makes and uses the modules: [modules.md](modules.md).

## A module directory

Since 7b.4 each module is one named module, `sq.<module>`, and each of
its types is a partition of it.

```
squared/<module>/
  module.cppm      the primary interface: `export module sq.<module>;`
  <name>.cppm      a partition per everyday type: `sq.<module>:<name>`
  Makefile
  src/
    <name>.cppm    a partition per supporting type: `sq.<module>:src.<name>`
    <name>.cpp     implementation units: `module sq.<module>;`
    detail/        the framework's own types: `sq.<module>:src.detail.<name>`
    <backend>/     backend sources (null/ and gles/ in graphics)
    common/        code shared by the backends (graphics)
  test/
```

`squared/third_party/`, `squared/docs/`, `squared/tools/` and
`squared/build/` sit beside the modules and follow none of this.
`squared/squared.cppm` is the one import for an app (`export module squared;`,
re-exporting `std`, app, gui, messaging and stage), and
`squared/third_party/libcxx/` holds the pinned sources of `std`
([modules.md](modules.md#std)).

**A partition is named for its path**, the slashes as dots:
`gui/src/detail/ui.cppm` is `sq.gui:src.detail.ui`. `tools/module_deps.sh`
refuses a file that declares another name, as it builds, and `make headers`
(`tools/check_modules.sh`) for the whole tree.

**The primary interface re-exports every partition** (`export import :label;`
for each), and the modules its partitions import (`export import
sq.graphics;` in sq.gui's): code was written against headers, whose includes
reached everyone who included them, and an importer of sq.gui still gets the
graphics types a widget's interface names. The detail partitions are exported
too: stage uses gui's `Ui` and `PaneFrame`, and a partition is private to its
module. They stay out of the programmer pages.

**A partition re-exports the partitions it imports** (`export import
:widget;` in `label.cppm`), for the same reason: a header's includes reached
whoever included it.

**Workhorse or supporting** was decided once per type, in Sprint 1, and the
root/`src/` split keeps it. A workhorse type is one people write in their own
code: `Button`, `TextureAtlas`, `FileHandle`. A supporting type is what those
are made of or report: styles, error codes, options structs, small value
types. Both come with `import sq.<module>;`.

**`app` is a built module** since 7b.4 (`libsquared_app.a`): it was headers
only, but a module's interfaces have objects (each module's initializer, and
member functions defined in a class, which are not implicitly inline in a
module).

## One type, one file

One public type, one partition, named for the type in `snake_case`. One
implementation unit per type that has out-of-line definitions.

```
gui/scroll_pane.cppm        class ScrollPane       (sq.gui:scroll_pane)
gui/src/scroll_pane.cpp     its definitions        (module sq.gui;)
```

Enums, aliases and plain value structs follow the same rule:
`squared/gui/src/pointer_action.cppm` holds `enum class PointerAction` and
nothing else.

The user chose a partition per type (2026-10-27) over one interface unit per
module with the type files included into it, and over one file per module,
on the numbers in [modules.md](modules.md#a-partition-per-type).

## Why one type per file

The priority order (RAM, ergonomics, speed, extensibility) does not make this
a priority-1 change and it should not be sold as one: moving text between
files changes no runtime allocation. What it buys:

- `squared-core` vendors into the project tree, so editing the source is the
  supported way to extend the framework. That only works if a file is small
  enough to read in full before changing it. See
  [extension-policy.md](extension-policy.md).
- Per-file measurement: the 39 errors blocking `-fno-exceptions -fno-rtti`
  became attributable to 17 named files instead of being a property of "the
  GUI".
- The directory listing a user sees is the short list of types they will use
  (ergonomics, priority 2).

Until 7b.4 it also bounded the include weight of each translation unit; a
module's BMI is read lazily, so that weight is gone either way.

## What a partition may declare

**Another module's types are imported, never declared ahead.** A module owns
what it declares: `namespace sq::files { class FileSystem; }` in sq.assets
declares a second `FileSystem`, attached to sq.assets, and clang refuses the
pair. `app/runtime.cppm` imports sq.graphics, sq.files and sq.assets for the
references it holds, so app comes after them in the build order.

**Declaring ahead inside the module is fine**: `class Painter;` in a gui
widget's partition names the same module's `Painter`.

**What code outside the modules declares or defines belongs to the global
module**, written `extern "C++"`:

| What | Where | Why |
|---|---|---|
| the replacement `operator new` and `delete` | `dev/src/frame_allocations.cpp` | they replace the global ones |
| `AAssetManager` | `files/android_asset_file_system.cppm` | the NDK's type, declared ahead; exported |
| `g_null_draw_calls` and the null backend's other counters | `graphics/src/null/sprite_batch.cpp` | tests declare them `extern` |
| `StageProbe`, `NameTableProbe` | `stage/stage.cppm`, `messaging/name_table.cppm` | befriended, defined by tests |
| `squared_main` | `stage/app.cppm` | the app defines it; exported |
| `layouts::all()`, `layouts::screens()` | `stage/src/detail/catalog.cppm`, `stage/src/catalog.cpp` | sqgen defines them in the app; the weak defaults with them |

A name attached to a module is mangled with it (`sq::App@sq.stage`), so the
same declaration outside the module would name another function, and the link
fails.

**Includes go in the global module fragment**, before the module
declaration: the C headers whose macros a unit uses (`<assert.h>`,
`<errno.h>`, `<stdarg.h>`, `<stdint.h>`, `<stddef.h>`, `<stdio.h>`,
`<signal.h>`, `<stdlib.h>`, `<time.h>`, `<ctype.h>`), the POSIX and NDK
headers, and third_party (`<yyjson.h>`, `<stb_image.h>`). An `#if` block of
includes goes there whole. Every standard C++ header is `import std;`.

**Configuration macros** (the Telegraph's and the Console's capacities,
`-DSQUARED_TELEGRAPH_RING=512`) are read where the module is compiled, in its
partition, and become constants there (`telegraph::ring`). An importer sees the
constants, never the macros; a project passes the same values to the framework
and its own code (`mk/pkg_squared_core.mk`), as before.

## Where a type is

A test, a tool or an app reaches a type by importing its module. Inside a
module, a unit that names a type of another partition imports that partition
(`import :size;`), and the module's implementation units see every partition
through the primary interface they implicitly import.
