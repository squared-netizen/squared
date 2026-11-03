# squared &mdash; programmer documentation

For people building something with squared. Nothing here describes an
internal: for that, see [../developer/README.md](../developer/README.md).

- [Getting started](getting-started.md) &mdash; a window, a skin and a widget
- [Imports and file layout](include-layout.md) &mdash; `import squared;`, the modules, and where each type is written
- [Building](building.md) &mdash; g++ and make, on desktop Linux and Termux
- [Tools](tools.md) &mdash; the scripts in `squared/tools/`, and the workspace wrappers that run them
- [API reference](../reference/README.md) &mdash; every public type and function with its comment, made from the module interfaces by `make docs`

## Modules

- [app](app.md) &mdash; the platform-neutral lifecycle, event and text-input boundary
- [assets](assets.md) &mdash; typed asset cache with dependency tracking
- [console](console.md) &mdash; the Console, compiled in with `CONSOLE=app`: commands and the built-ins, input, history, scrollback, crash reports, and drawing it
- [data](data.md) &mdash; strict RFC 8259 JSON parsing and writing
- [dev](dev.md) &mdash; development aids: a loopback log sink and its listener, crash reporter and frame-allocation counter (not in release builds)
- [files](files.md) &mdash; virtual filesystem and libGDX-style file handles; files compiled into the program (`EmbeddedFileSystem`, `tools/embed_files.sh`)
- [font](font.md) &mdash; font faces read and rendered at run time (FreeType): cell metrics, glyphs as 8-bit coverage, bold and italic made; fallback faces; a glyph atlas for one texture
- [gles](gles.md) &mdash; the GL object layer: buffers, shaders, programs, textures
- [graphics](graphics.md) &mdash; the rendering context and colour, then textures, atlases, sprites, the sprite batch, the 2D camera and bitmap fonts
- [gui](gui.md) &mdash; the libGDX-flavoured widget set and skinning, on the Actor/Group/Widget tree and its input routing (the host is internal since 7.7)
  - [layouts](layouts.md) &mdash; screens and panes as data: the node table, writing one from C++, layout files, `names.json`, the `sqgen` build step, taps, and building widgets from it
- [stage](stage.md) &mdash; the App and the Stage: `squared_main`, the model at the Telegraph's far end, the stack of screens, C++ screens and custom nodes, panes, taps, the window, and what is reported
- [term](term.md) &mdash; a terminal emulator's core: bytes from a program in, a screen of 12-byte cells out (ECMA-48 over UTF-8), and a program in a pseudo-terminal (`Pty`); sqterm's, Sprint 12
- [messaging](messaging.md) &mdash; the Telegraph: names, addresses, keys, the 32-byte telegram and its text form, the rings, the cache, lists, both ends, and the model's thread
- [math](math.md) &mdash; small value types used across the framework
- [time](time.md) &mdash; the time between ticks (`Timepiece`), the clocks, and a model's timers (`DeadlineQueue`)

| Module | Partitions at the root | Partitions in `src/` | Import |
|---|---:|---:|---|
| `app` | 4 | 9 | `import sq.app;` |
| `assets` | 1 | 8 | `import sq.assets;` |
| `console` | 16 | 10 | `import sq.console;` |
| `data` | 2 | 6 | `import sq.data;` |
| `dev` | 3 | 1 | `import sq.dev;` |
| `files` | 5 | 5 | `import sq.files;` |
| `font` | 8 | 0 | `import sq.font;` |
| `gles` | 5 | 9 | `import sq.gles;` |
| `graphics` | 11 | 19 | `import sq.graphics;` |
| `gui` | 30 | 38 | `import sq.gui;` |
| `messaging` | 15 | 7 | `import sq.messaging;` |
| `math` | 2 | 0 | `import sq.math;` |
| `stage` | 3 | 0 | `import sq.stage;` |
| `term` | 15 | 9 | `import sq.term;` |
| `time` | 5 | 0 | `import sq.time;` |

`import squared;` is everything an app is written against: the standard
library (`std`), app, gui, messaging and stage. Private partitions under
`src/detail/` are not counted: they are the framework's own.

## Reading the tables

Each module page lists every public type, the file that defines it, and
what it is for. One type, one file: `squared/gui/button.cppm` defines
`sq::gui::Button` and nothing else.

A type at the module root is one you will reach for. A type under `src/` is
a supporting type &mdash; a style, an error code, an options struct &mdash;
that comes with it. One import gives both: `import sq.gui;`. See
[include-layout.md](include-layout.md).
