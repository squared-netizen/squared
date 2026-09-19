# squared &mdash; programmer documentation

For people building something with squared. Nothing here describes an
internal: for that, see [../developer/README.md](../developer/README.md).

- [Getting started](getting-started.md) &mdash; a window, a skin and a widget
- [Include layout](include-layout.md) &mdash; which header to reach for
- [Building](building.md) &mdash; g++ and make, on desktop Linux and Termux

## Modules

- [app](app.md) &mdash; the platform-neutral lifecycle, event and text-input boundary
- [assets](assets.md) &mdash; typed asset cache with dependency tracking
- [data](data.md) &mdash; strict RFC 8259 JSON parsing and writing
- [files](files.md) &mdash; virtual filesystem and libGDX-style file handles
- [gles](gles.md) &mdash; the GL object layer: buffers, shaders, programs, textures
- [graphics](graphics.md) &mdash; Backend-neutral colour and context types.
- [graphics2d](graphics2d.md) &mdash; Textures, atlases, regions, sprites, bitmap fonts and the 2D camera.
- [scene2d](scene2d.md) &mdash; Actor/Group/Stage composition and the input event contract.
- [messaging](messaging.md) &mdash; addressed, queued message passing between decoupled parts of a program
- [gui](gui.md) &mdash; The libGDX-flavoured widget set, skinning, and the Ui host.
- [math](math.md) &mdash; Small value types used across the framework.
- [time](time.md) &mdash; Clock domains and a bounded deadline queue.

| Module | Public types | Aggregate header |
|---|---:|---|
| `app` | 8 | `squared/app/application.hpp` |
| `assets` | 9 | `squared/assets/assets.hpp` |
| `data` | 7 | `squared/data/json.hpp` |
| `files` | 9 | `squared/files/files.hpp` |
| `gles` | 16 | `squared/gles/gles.hpp` |
| `graphics` | 2 | `squared/graphics/graphics.hpp` |
| `graphics2d` | 27 | `squared/graphics2d/graphics2d.hpp` |
| `scene2d` | 11 | `squared/scene2d/scene2d.hpp` |
| `messaging` | 23 | `squared/messaging/messaging.hpp` |
| `gui` | 57 | `squared/gui/gui.hpp` |
| `math` | 2 | `squared/math/math.hpp` |
| `time` | 6 | `squared/time/time.hpp` |

## Reading the tables

Each module page lists every public type, the header that defines it, and
what it is for. One type, one header: `squared/gui/button.hpp` defines
`sq::gui::Button` and nothing else.
