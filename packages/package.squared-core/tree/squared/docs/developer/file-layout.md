# File layout

Where files go inside a module, how they are included, and the rules that
keep each header small and self-contained.

Programmer counterpart: [../programmer/include-layout.md](../programmer/include-layout.md)

## A module directory

A module directory shows what a user needs and hides the rest.

```
squared/<module>/
  <name>.hpp     workhorse headers: the ones users include
  Makefile
  src/
    <name>.hpp   supporting headers: includable, rarely needed
    <name>.cpp   implementation
    detail/      private headers; only the module's own sources include them
    <backend>/   backend sources (null/ and gles/ in graphics)
    common/      code shared by the backends (graphics)
  test/
```

`squared/third_party/`, `squared/docs/`, `squared/tools/` and
`squared/build/` sit beside the modules and follow none of this.

**Workhorse or supporting** was decided once per header, in Sprint 1. A
workhorse header names a type people write in their own code: `Button`,
`TextureAtlas`, `FileHandle`. A supporting header holds what those types are
made of or report: styles, error codes, options structs, small value types.

**Every supporting header is reachable** through the includes of some
workhorse header, so a user never has to know `src/` exists to get a type.
The exception would be deliberate extension surface: a header only someone
extending squared needs, listed in `tools/check_headers.sh`. There is none
today; the old dispatcher's provider interface was the only one, and went with
it (2026-10-20).

This is checked, not asserted. The first step of `tools/check_headers.sh`
follows the `#include <squared/...>` lines out from every workhorse header and
fails for any supporting header it never reaches. A module's aggregate,
`<module>/<module>.hpp`, is not a starting point: it includes the whole module,
so counting it would let anything pass. The three extension-surface headers
are listed in the script, and a listed header that no longer exists fails too,
so the list cannot quietly go stale. The step is one `grep` over the tree and
one `awk`, and takes well under a second.

**`gui/batch_painter.hpp` is a workhorse header.** It was the last header
without a verdict, and Sprint 3 settled it. An application names
`BatchPainter` in its own code: it constructs one, keeps it, and passes it to
`Ui::layout()` and `Ui::paint()`, which take the abstract `Painter`. It is the
only `Painter` that puts widgets on screen. It was already at the root, so no
file moved and no include line changed. Every header now has a verdict: 77 at
module roots (eight of them aggregates) and 120 in `src/`.

## Include paths

| kind | written as | found through |
|---|---|---|
| workhorse | `<squared/gui/button.hpp>` | the include root |
| supporting | `<squared/gui/src/size.hpp>` | the include root |
| private | `"detail/gui_detail.hpp"` | `-I<module>/src`, only while that module builds |

**One include root** &mdash; the directory that contains `squared/`. In the
framework tree that is `package.squared-core/tree`; in a vendored project it
is the parent of `$(SQ_SQUARED_DIR)`, which `mk/pkg_squared_core.mk` sets.
One `-I` replaces the twelve `-I<module>/include` flags of the old layout. The
directory must keep the name `squared`, because every include line names it.

## Two modules merged in Sprint 1

`graphics2d` joined `graphics`, and `scene2d` joined `gui`. Their namespaces
went with them, flat: `sq::graphics2d::Texture` is `sq::graphics::Texture`,
and `sq::scene2d::Actor` is `sq::gui::Actor`. No header name and no `src/` or
`test/` path collided, and their archives became part of `libsquared_graphics.a`
and `libsquared_gui.a`. Link order is gui, graphics, gles, assets, files,
messaging, data, time, math.

Backends live under `graphics/src/<backend>/`. Since the merge a backend is a
directory holding `context.cpp`, `sprite_batch.cpp` and `texture.cpp`; see
[graphics.md](graphics.md#link-time-substitution).

## One type, one header

One public type, one header, named for the type in `snake_case`. One
translation unit per type that has out-of-line definitions.

```
gui/scroll_pane.hpp       class ScrollPane
gui/src/scroll_pane.cpp   its definitions
```

Enums, aliases and plain value structs follow the same rule:
`squared/gui/src/pointer_action.hpp` holds `enum class PointerAction` and
nothing else.

## Why

The priority order (RAM, ergonomics, speed, extensibility) does not make this
a priority-1 change and it should not be sold as one: moving text between
files changes no runtime allocation. What it buys is bounded per-translation-
unit include weight, which is compile-time memory, and on Android that is a
real constraint. Before the split, touching anything in the GUI meant parsing
2,719 lines of `gui.hpp` in every one of its consumers. Now a translation unit
that needs `Label` parses `label.hpp`, `widget.hpp`, `size.hpp` and two
forward declarations.

There is a third reason specific to squared, and it is the one that matters
most: `squared-core` vendors into the project tree, so editing the source is
the supported way to extend the framework. That only works if a file is small
enough to read in full before changing it. See
[extension-policy.md](extension-policy.md).

Second, it makes per-file measurement possible. The 39 errors blocking
`-fno-exceptions -fno-rtti` became attributable to 17 named files instead of
being a property of "the GUI".

The root/`src/` split serves ergonomics (priority 2) the same way: the
directory listing a user sees is the short list of types they will use.

## Self-containment invariant

**Every header compiles on its own.** No header requires another to be
included first.

This is checked, not asserted: the second step of `tools/check_headers.sh`
compiles every header at a module root and under `src/` in isolation, with
`-std=c++20 -Wall -Wextra -Wpedantic`, and `make headers` runs it. It skips
`src/detail/`, which is private. A header that regresses fails there.

## How includes are decided

A header includes a dependency's definition when it needs it, and forward
declares otherwise. The test applied to every header/dependency pair:

> If **every** mention of `T` in this header is `T*` or `T&`, forward declare
> it. Otherwise include it.

That rule alone resolves the cycles: `Cell` names `Table` only as `Table*`
(a `friend class Table;` declaration is not a use), `ButtonGroup` names
`ToggleButton` only as `ToggleButton*` and `ToggleButton&`, `Actor` names
`Group` only as `Group*`. It is also what keeps `Painter` and `Skin` out of
all twenty widget headers.

The rule is not sound on its own: an inline body in the same header can
dereference a reference in its own signature. Four pairs need the definition
despite naming the type only by reference:

| Header | Needs the definition of | Because |
|---|---|---|
| `BitmapFont` | `BitmapFontParseLimits` | defaulted argument `= {}` |
| `GlyphLayout` | `GlyphLayoutOptions` | defaulted argument `= {}` |
| `Sprite` | `TextureRegion` | inline constructor reads the region |
| `TextureRegion` | `Texture` | inline constructor calls `texture.width()` |

The compiler found all four.

Reachability can add include lines of its own: a workhorse header may include
a supporting header it only names by reference, so that the supporting type is
reachable without a `src/` path.

## Aggregate headers

One per module, and none of them is for use inside a header:

| Module | Aggregate | Note |
|---|---|---|
| `assets`, `files`, `gles`, `graphics`, `gui`, `math`, `messaging`, `time` | `squared/<module>/<module>.hpp` | includes the whole module |
| `app` | `squared/app/application.hpp` | `Application` itself; see [app.md](app.md) |
| `data` | `squared/data/json.hpp` | the parse and write functions |
| `dev` | `squared/dev/dev_log.hpp` | the module's one header |

`gui/input.hpp` is a topic header: the input-routing types together, short of
the whole of `gui.hpp`.

## Private headers

`src/detail/*.hpp` is internal. It holds helpers that were per-translation-
unit anonymous-namespace entities in the monolith and now need to be shared
across the split translation units. They are `inline` in a `detail`
namespace, included as `"detail/<name>.hpp"`, and reachable only while their
own module builds.
