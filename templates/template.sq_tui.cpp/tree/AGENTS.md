# AGENTS.md — {{project_name}}

Conventions for this workspace. Seeded by squared-pg; yours to change.

## Ground rules

- **Write code in `sq_app/`.** That is the user working directory.
- **Never edit `sq/build/`.** It is generator-owned and rewritten on update.
  Project-wide build settings go in `build.lua` (`sq/build/template.lua`
  lists every setting).
- **`sq_pkg/squared_tui/` is package.squared-tui's, shared**: change it if you
  must, and an update says where it and the package differ.
- **The build globs.** Every `.cppm` and `.cpp` under `sq_app/src/` is
  compiled; there is no list to keep.

## Language

C++23 with `import std;` and squared's modules (`import sq.tui;`). The build
sets `-Wall -Wextra -Wpedantic`; keep it warning-free.

## The shape of the program

- `main.cpp` runs the loop and nothing else: open the terminal, draw, show,
  wait for an event.
- What is drawn and how input changes it lives in modules (`board.cppm`), so
  a test draws it into `sq::tui::Test` and reads back the text with no
  terminal.
- Draw the whole screen from the state each time; `Terminal::show` sends only
  what changed.

## Before committing

```sh
sqmake          # warning-free
sqmake test     # the tests pass
square verify   # nothing generated changed by hand
```
