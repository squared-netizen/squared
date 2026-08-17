# Squared Graphics — Developer Guide

Squared Graphics is the portable low-level contract layer of the framework.
It ships a `Color` value type and the `Context` contract; the actual window,
rendering context, clear, and present operations are performed by exactly one
backend selected at link time (the shipped implementation is
squared-backend-sdl2-opengl). The package has no implementation source:
`Context`'s methods are declared here and defined by the linked backend.

- Programmer counterpart: [Squared Graphics — Programmer Guide](../programmer/squared-graphics/README.md)
- Package payload: [Graphics.md](../../packages/squared-graphics/content/docs/Graphics.md)
- Documentation index: [Developer documentation](../README.md)
- Diagrams:
  - [Portable Graphics contract to selected backend](contract-to-backend.dot)
  - [Graphics context lifecycle](context-lifecycle.dot)

## Dependency boundary

The manifest declares `module.requires` as `[]`: no other Squared module or
third-party library is required. The CMake target `squared_graphics` is an
`INTERFACE` library (`content/modules/squared-graphics/CMakeLists.txt`) that
exposes only `cxx_std_20` and the `include/` directory.

The portable/backend boundary is strict: `color.hpp` includes only
`<algorithm>` and `<cstdint>`, and `context.hpp` includes only `color.hpp`
and `<cstdint>`. No SDL, OpenGL, Android, or GUI header appears. The backend
reverse-links the same header set; public headers never reference the
backend. `tests/graphics-test.cpp` compiles against only the graphics include
path and static-asserts the portable contract surface.

Because the backend is selected at link time and the symbols are the concrete
members of `squared::graphics::Context` (not a virtual interface), portable
code cannot accidentally bind to a different backend: the program links
exactly the backend the platform template chose.

## Architecture

| Component | Source | Responsibility |
| --- | --- | --- |
| `squared::graphics::Color` | `include/squared/graphics/color.hpp` | Linear normalized RGBA value with byte conversion and clamping. |
| `squared::graphics::Context` | `include/squared/graphics/context.hpp` | Window/context ownership contract; create, suspend, resume, viewport, clear, present, generation. |
| Backend implementation | (backend package) `src/context.cpp` | SDL2 + OpenGL ES 2 realization of every `Context` method. |

The package deliberately contains no render target, texture, or pipeline
abstraction; those belong to Squared Graphics2D and are intentionally absent
so the low-level contract stays minimal.

## Ownership and threading

- `Context` owns, through two private `void*` opaque handles, the native
  window and the native rendering context. Only `destroy()` releases both;
  `suspend()` releases only the context and keeps the window.
- A single-thread model applies: the owning platform adapter runs the event
  loop and all `Context` calls on one thread. GL context validity is
  thread-bound, so no other thread may touch the context.
- `generation_` is a monotonically increasing `std::uint64_t` bumped by every
  successful activation (`create()` via `resume()`, and each successful
  reactivation). The counter wraps to skip zero, preserving the invariant
  that zero means "no context yet".
- `resources_preserved_` is set `true` only on the fast-path reactivation of
  an existing retained context; a freshly created replacement context reports
  `false`.

## Invariants and failure behavior

- `valid()` is the invariant `window_ && native_context_`; `create()` first
  destroys any prior state, so a failed re-create never leaves a half-open
  window.
- `resume()` on a windowed app either reactivates the SDL-retained context
  (fast path, `resources_preserved_ = true`) or, when `MakeCurrent` fails,
  deletes and rebuilds a replacement context (recovery path,
  `resources_preserved_ = false`).
- A successful `resume()` must observe a positive `pixel_width_`/`pixel_height_`
  drawable, otherwise it suspends and returns `false`; frameworks must not
  present to a zero-sized drawable.
- `Android`: after `SDL_APP_WILLENTERBACKGROUND` the backend never issues GL
  calls; the adapter stops rendering and invalidates resources, and only calls
  `resume()` after `SDL_APP_DIDENTERFOREGROUND`, when SDL has made rendering
  available again.
- `present()` guards on `window_`; `clear()` always runs but clamps the color,
  so out-of-range components cannot reach `glClearColor`.

## Data structures and complexity

- `Color` is O(1): four `float` members; `clamped()` creates a new struct
  with `std::clamp` per component.
- `Context` is O(1): two opaque pointers, two `int` drawable dimensions, one
  `std::uint64_t` generation counter, and one `bool`. `pixel_width()` and
  `pixel_height()` return cached values refreshed by `refresh_viewport()`.

## Algorithms and execution order

`Context::create()` (backend):

1. `SDL_GL_SetAttribute` for the ES profile, version 2.0, double buffer, and
   RGBA 8/8/8/8 with depth 0 and stencil 0.
2. `SDL_CreateWindow` with `SDL_WINDOW_OPENGL`, centered, and high-DPI
   allowed; failure logs `SDL_GetError()` and returns `false`.
3. `resume()` and return its result.

`resume()`:

1. If a retained `native_context_` exists, try `SDL_GL_MakeCurrent` on it;
   on failure delete and null the context (fall through to rebuild), on
   success refresh the viewport, verify a positive drawable, set
   `resources_preserved_ = true`, and bump `generation_` (skipping zero).
2. Otherwise create a fresh context with `SDL_GL_CreateContext`, `MakeCurrent`,
   best-effort `SDL_GL_SetSwapInterval(1)` (logged when unsupported), refresh
   the viewport and log `GL_VERSION`. Zero drawable suspends and returns
   `false`. Bump `generation_`.

`suspend()`: `MakeCurrent(nullptr)`, delete the context, null it, zero the
dimensions and `resources_preserved_`. `destroy()`: `suspend()` then destroy
the window and reset everything, including `generation_ = 0`.

## Design patterns

- **Facade** — `Context` wraps SDL2 windowing plus OpenGL ES calls behind a
  small portable surface. Its clear/present/suspend/resume hide
  `glViewport`, `SDL_GL_SwapWindow`, and EGL-lifetime details.
- **Strategy (link-time)** — the contract is implemented by exactly one
  backend with no virtual dispatch and no runtime registry. Unlike the
  textbook dynamic Strategy, the decision is enforced at link time by the
  platform template, which is cheaper and eliminates any registration
  failure mode.
- **Template Method (resource-generation lifecycle)** — `create`,
  `suspend`, `resume`, and `destroy` define the fixed skeleton of window
  and context generation that portable resource owners (textures, batches)
  rely on; the backend fills the concrete steps. This same skeleton is
  mirrored by every recoverable Graphics2D resource.

Alternatives rejected: a virtual `IGraphics` interface plus runtime backend
registry (rejected — adds startup cost, a registry error surface, and allows
portable code to bind a wrong backend); per-render capability queries
(rejected — `TODO.md` keeps portable capability reporting as unfinished work
because the current single backend makes it premature).

## Limitations and technical debt

- **No portable capability reporting**: `TODO.md` records reporting context
  capabilities as the next unfinished item; until then applications assume
  the ES2/RGBA8/2D profile of the shipped backend.
- **Single window, single drawable**: the lifecycle models one context; rapid
  create/destroy cycles are supported but multi-window use is not.
- **Generation arithmetic**: `generation()` wraps by skipping zero, so a
  counter value cannot identify a specific context epoch; applications should
  compare values for change, not equality with a particular epoch.
- **Opaque handles hide backend detail**: the two `void*` members keep SDL
  out of public headers but also force every backend to interpret the same
  slots; a future second backend must keep that contract.
- The clear color is clamped in the portable call rather than the backend,
  so `Context::clear` never sees raw values; this is intentional but means
  the portable contract silently normalizes extreme inputs.