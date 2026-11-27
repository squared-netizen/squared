# package.squared-tui

squared-tui: character-cell programs on squared (`import sq.tui;`), the
terminal programs a person runs in sqterm or any VT terminal: tools, games,
editors, and the exercises a course in programming starts with. The design
is the squared repository's `docs/design/squared-tui.md`; how it works,
`tree/sq_pkg/squared_tui/docs/tui.md`.

| Layer | Types |
|---|---|
| the terminal | `Terminal` (raw mode, put back however the program ends), `Event`, `Key`, `InputDecoder` |
| the screen | `Screen`, `View`, `Style`, `Rect`, `Renderer` (only what changed is written) |
| testing | `Test`: a Screen rendered into sq.term's emulator, read back as a person sees it |
| teaching | `Simple`: `clear`, `put`, `show`, `key()` |

## In a workspace

Everything under `tree/` is copied into a workspace made with the package,
at the same path from its root:

- `sq_pkg/squared_tui/`: sq.tui's interfaces (`*.cppm`), sources (`src/`)
  and its test (`test/`, not built into the app);
- `sq/build/package_squared_tui.lua`, which the template's build includes
  after the kits' and which returns `{id, srcs}`: sq.tui compiled with the
  app.

sq.tui imports squared-core's `sq.term` (the cell, UTF-8, the emulator the
test bench uses) and `sq.keys` (`Key`, `KeyModifiers`); the template links
squared's `term` library, which brings `keys`, and nothing of squared's
graphics. `template.sq_tui.cpp` and `template.sq_tui.cpp.edu` are made for
it.

This file is beside `tree/`, not in it: it documents the package without
being copied. The ownership is `shared`: a workspace may change the
package's code, and an update says where it and the package differ rather
than overwriting it.

## Tests

`tree/sq_pkg/squared_tui/test/tui_test.cpp`, built and run by squared-core's
`sqmake test` (its build.lua names the package's folder), every layer
checked through sq.term's emulator.
