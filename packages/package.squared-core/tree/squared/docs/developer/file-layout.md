# File layout

## The rule

One public type, one header, named for the type in `snake_case`. One
translation unit per type that has out-of-line definitions.

```
gui/include/squared/gui/scroll_pane.hpp   class ScrollPane
gui/src/scroll_pane.cpp                   its definitions
```

Enums, aliases and plain value structs follow the same rule:
`squared/gui/pointer_action.hpp` holds `enum class PointerAction` and nothing
else.

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
`-fno-exceptions -fno-rtti` are now attributable to 17 named files instead of
being a property of "the GUI".

## Self-containment invariant

**Every header compiles on its own.** No header requires another to be
included first.

This is checked, not asserted: `tools/check_headers.sh` compiles each of the
113 public headers in isolation with `-std=c++20 -Wall -Wextra -Wpedantic`,
and `make headers` runs it. A header that regresses fails there.

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
despite naming the type only by reference, and are recorded in
`tools/forbidden_demotions.json`:

| Header | Needs the definition of | Because |
|---|---|---|
| `BitmapFont` | `BitmapFontParseLimits` | defaulted argument `= {}` |
| `GlyphLayout` | `GlyphLayoutOptions` | defaulted argument `= {}` |
| `Sprite` | `TextureRegion` | inline constructor reads the region |
| `TextureRegion` | `Texture` | inline constructor calls `texture.width()` |

The compiler found all four. The generator does not guess; it records what the
compiler rejected and re-runs.

## Aggregate headers

`squared/gui/gui.hpp` and `squared/gui/skin_loader.hpp` still exist as pure
include lists, so no existing consumer had to change. Each module also gains
`squared/<module>/<module>.hpp`. Do not use any of them inside a header.

## Private headers

`src/detail/*.hpp` is internal. It holds helpers that were per-translation-
unit anonymous-namespace entities in the monolith and now need to be shared
across the split translation units. They are `inline` in a `detail` namespace
and are not installed.
