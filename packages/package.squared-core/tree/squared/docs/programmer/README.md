# squared &mdash; programmer documentation

For people building something with squared. Nothing here describes an
internal: for that, see [../developer/README.md](../developer/README.md).

- [Getting started](getting-started.md) &mdash; a window, a skin and a widget
- [Include layout](include-layout.md) &mdash; which header to reach for
- [Building](building.md) &mdash; g++ and make, on desktop Linux and Termux
- [Tools](tools.md) &mdash; the scripts in `squared/tools/`, and the workspace wrappers that run them

## Modules

- [app](app.md) &mdash; the platform-neutral lifecycle, event and text-input boundary
- [assets](assets.md) &mdash; typed asset cache with dependency tracking
- [console](console.md) &mdash; the Console, compiled in with `CONSOLE=app`: commands, input, history, scrollback (drawing comes next)
- [data](data.md) &mdash; strict RFC 8259 JSON parsing and writing
- [dev](dev.md) &mdash; development aids: a loopback log sink, crash reporter and frame-allocation counter (not in release builds)
- [files](files.md) &mdash; virtual filesystem and libGDX-style file handles
- [gles](gles.md) &mdash; the GL object layer: buffers, shaders, programs, textures
- [graphics](graphics.md) &mdash; the rendering context and colour, then textures, atlases, sprites, the sprite batch, the 2D camera and bitmap fonts
- [gui](gui.md) &mdash; the libGDX-flavoured widget set, skinning and the Ui host, on the Actor/Group/Stage scene graph and its input routing
- [messaging](messaging.md) &mdash; addressed, queued message passing between decoupled parts of a program
- [math](math.md) &mdash; small value types used across the framework
- [time](time.md) &mdash; clock domains and a bounded deadline queue

| Module | Headers at the root | Headers in `src/` | Aggregate header |
|---|---:|---:|---|
| `app` | 4 | 7 | `squared/app/application.hpp` |
| `assets` | 1 | 8 | `squared/assets/assets.hpp` |
| `console` | 4 | 4 | `squared/console/console.hpp` |
| `data` | 2 | 6 | `squared/data/json.hpp` |
| `dev` | 2 | 0 | none: `dev_log.hpp` and `frame_allocations.hpp` are independent |
| `files` | 4 | 5 | `squared/files/files.hpp` |
| `gles` | 5 | 11 | `squared/gles/gles.hpp` |
| `graphics` | 11 | 19 | `squared/graphics/graphics.hpp` |
| `gui` | 29 | 43 | `squared/gui/gui.hpp` |
| `messaging` | 4 | 20 | `squared/messaging/messaging.hpp` |
| `math` | 2 | 0 | `squared/math/math.hpp` |
| `time` | 5 | 1 | `squared/time/time.hpp` |

The root counts leave out the aggregate header itself, where a module has a
separate one. Private headers under `src/detail/` are not counted: they are
not for you to include.

## Reading the tables

Each module page lists every public type, the header that defines it, and
what it is for. One type, one header: `squared/gui/button.hpp` defines
`sq::gui::Button` and nothing else.

A header at the module root is one you will reach for. A header under
`src/` is a supporting type &mdash; a style, an error code, an options
struct &mdash; that the root headers already include. Both are written the
same way: `#include <squared/gui/button.hpp>`,
`#include <squared/gui/src/button_style.hpp>`. See
[include-layout.md](include-layout.md).
