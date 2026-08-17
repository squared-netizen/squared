# Squared Graphics — Programmer Guide

Squared Graphics defines the portable low-level graphics contracts of the
framework: the `squared::graphics::Color` value type shared by every
higher-level rendering module, and `squared::graphics::Context`, the
window/rendering-context contract that a selected link-time backend
implements. The package contains no SDL or OpenGL headers, sources, or link
requirements; the graphics implementation is chosen by the platform template
at link time, never registered at runtime.

## Package availability

| Module | Version | Requires |
| --- | --- | --- |
| `dev.squarednetizen.squared.graphics` | `0.6.0-dev.4` | (none) |

The CMake target is `squared_graphics`, an `INTERFACE` target exporting the
`include/` directory and the C++20 requirement.

## Public API overview

| Type | Header | Purpose |
| --- | --- | --- |
| `squared::graphics::Color` | `squared/graphics/color.hpp` | Normalized four-component color value. |
| `squared::graphics::Context` | `squared/graphics/context.hpp` | Owns the native window and rendering context selected at link time. |

Higher-level rendering types — texture, atlas, sprite, batch, and camera —
live in Squared Graphics2D, not here.

## Color

`Color` is a plain struct of four `float` components `red`, `green`, `blue`,
and `alpha`, each in the closed range `[0, 1]`. Components are linear
(non-gamma-corrected) and map directly to the units of the rendering API.

```cpp
#include <squared/graphics/color.hpp>

const auto opaque_white = squared::graphics::Color::white();
const auto clear_black = squared::graphics::Color::transparent();
const auto orange_50 =
    squared::graphics::Color::from_rgba8(255, 128, 0, 50);
const auto safe = squared::graphics::Color{
    1.25F, -0.5F, 0.5F, 2.0F
}.clamped();
```

- `white()` and `transparent()` return opaque white and clear black
  respectively. Both are `constexpr`.
- `from_rgba8(r, g, b, a = 255)` converts byte components in the inclusive
  range `[0, 255]` by dividing each by `255.0F`. It never fails; any value
  is representable.
- `clamped()` returns a copy with every component clamped to `[0, 1]`. Squared
  rendering code applies this before a color reaches a backend, so passing
  out-of-range components never produces undefined GL behavior.

`Color` is a value type: copy freely, aggregate-initialize directly, and
multiply the tint into sampled texture colors by passing it to the drawing
APIs.

## Context

`Context` owns one backend window and a replaceable rendering context. The
generated platform layer owns the lifetime of this object: developer
applications receive it through the Squared Application rendering boundary
(`Application::create`, `Application::surface_created`, `Application::render`)
and never create or present windows themselves. The class is non-copyable
and non-movable.

The lifecycle is `create()` → frames → `suspend()`/`resume()` on context loss
→ `destroy()`.

```cpp
#include <squared/graphics/context.hpp>
#include <squared/graphics/color.hpp>

squared::graphics::Context graphics;
if (!graphics.create("Example", 960, 540)) {
    // create() already logged the platform error; abort startup.
    return 1;
}

graphics.clear(squared::graphics::Color::from_rgba8(24, 32, 40));
// ... draw with Graphics2D objects ...
graphics.present();

graphics.destroy();
```

- `create(title, logical_width, logical_height)` creates the window and makes
  its rendering context current. It returns `true` only when both exist and
  the drawable is usable; on failure it returns `false` after logging the
  platform error. Call `destroy()` first if re-creating.
- `clear(color)` clears the active color buffer; the color is clamped first.
  `present()` swaps the completed frame. Both are `void`.
- `suspend()` releases the native rendering context while retaining the
  window. `resume()` reactivates the retained context when possible or
  creates a replacement. After a successful `resume()`, `generation()` has
  advanced and `resources_preserved()` reports whether the previous context's
  GPU objects are still valid.
- `refresh_viewport()` re-reads drawable dimensions and reapplies the
  backend viewport, for example after a window resize.
- `valid()` is `true` after `create()` or a successful `resume()`.
  `pixel_width()` and `pixel_height()` return the current framebuffer size in
  physical pixels.

### Context loss and restoration

Android lifecycle and equivalent situations can destroy the graphics context.
Squared's protocol for recovering GPU resources:

1. When rendering becomes unavailable, stop issuing graphics calls and call
   `Context::suspend()` (or, on Android, let SDL suspend EGL and only mark
   resources stale). Portable resource owners (Graphics2D textures and
   batches) expose `invalidate()` for the no-call path.
2. When rendering is available again, call `resume()`. Read
   `generation()` and `resources_preserved()`; then recreate or restore every
   GPU resource that the context backs.
3. Textures and sprite batches built for recovery hold the recipe needed to
   rebuild themselves (see their own guides), so the application typically
   forwards `resources_preserved()` into their `restore()` calls.

## Errors and failure behavior

`create()` and `resume()` return `false` on failure and log the platform
error message; there is no exception channel. `clear()` and `present()` are
`noexcept void` and do not report failure. Portable resource restorers report
`bool` success so the application can rebuild or abandon resources.

## Threading

All `Context` methods must be called from the application's main thread, the
same thread that calls `Application::render()` and owns the event loop. Non-
current-context GL calls are undefined; the backend does no cross-thread
synchronization.

## Ownership and lifetime

`Context` owns the native window handle and the native context; only its
`destroy()` releases them. It is non-copyable and non-movable, so video state
cannot escape the owning adapter. The platform adapter owns the `Context`
and the application borrows it for the duration of each lifecycle callback.

## Lua bindings

None of the types in this package have a Lua 5.4 binding.

## Related documentation

- Package payload: [Graphics.md](../../../packages/squared-graphics/content/docs/Graphics.md)
- Implementation details: [Squared Graphics — Developer Guide](../developer/squared-graphics/README.md)
- Squared SDL2/OpenGL backend (the link-time implementation):
  [Programmer Guide](../squared-backend-sdl2-opengl/README.md)
- Documentation index: [Programmer documentation](../README.md)