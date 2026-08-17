# Squared SDL2/OpenGL Backend — Programmer Guide

Squared SDL2/OpenGL Backend is the link-time implementation of the Squared
Graphics and Graphics2D contracts for SDL2 with OpenGL ES 2. It owns window
and context creation, texture uploads, atlas file loading, shaders, buffers,
drawing, and presentation, plus Android-style context loss recovery. You
select it by making the platform template depend on this exact module;
there is no runtime backend registry, lookup, or virtual dispatch in the
frame loop, so a platform links exactly one graphics backend.

The package defines no new public C++ types: the rendering classes you use
are the portable contracts from Squared Graphics and Graphics2D, and this
package provides their implementations.

## Package availability

| Module | Version | Requires |
| --- | --- | --- |
| `dev.squarednetizen.squared.backend.sdl2-opengl` | `0.6.0-dev.5` | `dev.squarednetizen.squared.graphics2d` `0.6.0-dev.7` |

The CMake target is `squared_backend_sdl2_opengl`, a `STATIC` library that
links `squared_graphics2d`, `SDL2`, `SDL2_image`, and the configured OpenGL
ES 2 library. Build integration must already provide the `SDL2` and
`SDL2_image` targets, `SDL2_INCLUDE_DIR`, and `SQUARED_GLES2_LIBRARY`; the
module aborts configuration if any is missing.

## Public API overview

| Type (declared elsewhere) | Header | Relationship to this backend |
| --- | --- | --- |
| `squared::graphics::Context` | `squared/graphics/context.hpp` | Window/context creation, viewport, clear, present, resume, and generation counters are implemented here. |
| `squared::graphics2d::Texture` | `squared/graphics2d/texture.hpp` | Asset loading, RGBA upload, and GPU-object recovery policies are implemented here. |
| `squared::graphics2d::TextureRegion` | `squared/graphics2d/texture_region.hpp` | Non-owning texture view consumed by the batch here. |
| `squared::graphics2d::TextureAtlas`, `squared::graphics2d::AtlasRegion` | `squared/graphics2d/texture_atlas.hpp` | libGDX `.atlas` parsing, page loading, and region construction are implemented here. |
| `squared::graphics2d::Sprite`, `squared::graphics2d::SpriteBatch` | `squared/graphics2d/sprite.hpp`, `squared/graphics2d/sprite_batch.hpp` | Sprite transform and quad batching with the built-in shader are implemented here. |
| `squared::graphics2d::OrthographicCamera` | `squared/graphics2d/orthographic_camera.hpp` | Portable projection; its `combined()` matrix drives the batch. |

For the full contract reference of these types, see the
[Squared Graphics2D programmer guide](../squared-graphics2d/README.md).

## Backend selection

Applications select the backend by including this module in their exact
dependency graph. The example platform template
(`examples/gui-showcase/android/CMakeLists.txt`) links
`squared_backend_sdl2_opengl` next to the portable modules. Portable
application and framework code includes only Squared Graphics, Graphics2D,
and Scene2D headers; SDL and OpenGL headers stay in this package and in the
platform template.

## Context capabilities

A successful `graphics.create(...)` requests, through SDL attributes:

- OpenGL ES profile, major version 2, minor version 0;
- double-buffered swap;
- RGBA red/green/blue/alpha each 8 bits;
- depth size 0 and stencil size 0.

Vertical synchronization is requested with `SDL_GL_SetSwapInterval(1)` on a
best-effort basis; if unsupported, it is reported in the log and rendering
continues without it. The drawable may be high-DPI, so applications must use
`pixel_width()`/`pixel_height()` rather than logical sizes for framebuffer
work. `clear()` clamps the color and applies `glClearColor`/`glClear`;
`present()` swaps the window.

## Loading textures and atlases

`Texture` and `TextureAtlas` load assets through SDL2_image and upload as
tightly packed RGBA8888 data. All loaders return `bool`; on failure they log
the error and (for the atlas) leave any prior successful load untouched, so a
`load()` in hot paths can safely fall back to a previous atlas.

```cpp
#include <squared/graphics/color.hpp>
#include <squared/graphics2d/texture.hpp>

squared::graphics2d::Texture texture;
if (!texture.load("images/tree.png")) {
    // SDL2_image logged IMG_GetError(); abort or degrade.
    return;
}
texture.set_filter(
    squared::graphics2d::TextureFilter::Linear,
    squared::graphics2d::TextureFilter::Linear
);
```

Create surfaces from memory, not just files: `create_rgba(width, height,
pixels)` takes tightly packed RGBA8888 bytes, and `create_solid(color)` fills
a one-pixel texture. Recovery options are chosen at creation time via
`TextureRecoveryOptions`; see the Graphics2D guide for each policy's recipe
semantics.

Texture atlases use the libGDX text `.atlas` format. Page image paths are
resolved relative to the atlas file; page `filter` values are limited to
`Nearest` and `Linear`, and `repeat` to `none`, `x`, `y`, or `xy`. Entries
with `rotate: true` or `rotate: 90` store rotated storage; the logical
dimensions are preserved. Nine-patch `split` and `pad` metadata is retained
for GUI consumption.

```cpp
#include <squared/graphics2d/texture_atlas.hpp>

squared::graphics2d::TextureAtlas atlas;
if (!atlas.load("ui/theme.atlas")) return;
if (const auto* region = atlas.find_region("button_normal", -1)) {
    // region->region() is a TextureRegion; pointers are valid while
    // the atlas lives and become invalid after release()/restore()
    // cycling that reloads pages.
}
```

## Sprite batching

`SpriteBatch` renders ordered textured quads with one GPU upload per flush.
Construct it, call `initialize(maximum_sprites)` once, then repeat the
`begin(camera)` → `draw(...)`* → `end()` cycle each frame. Sprites are
buffered as raw quads, so drawing thousands of quads is a single draw call.

```cpp
#include <squared/graphics2d/orthographic_camera.hpp>
#include <squared/graphics2d/sprite_batch.hpp>

squared::graphics2d::OrthographicCamera camera(960.0F, 540.0F);
camera.update();

squared::graphics2d::SpriteBatch batch;
if (!batch.initialize(2048)) return;

// Each frame:
if (batch.begin(camera)) {
    batch.draw(region, 100.0F, 120.0F, 128.0F, 128.0F);
    batch.draw(sprite);
    batch.end();
}
```

An automatic flush happens when the texture changes or the capacity is
reached; `flush()` pushes partial batches within a frame when needed. Every
drawn color is multiplied with the sampled texture, so opaque white tint
preserves the source.

## Context loss and restore

Android and equivalent platforms can lose the OpenGL context. After the
At the background boundary, stop rendering and do not issue GL calls; mark resources
stale with `invalidate()`. On foreground recovery, reactivate the context
through `graphics.resume()` and then restore each resource in dependency
order: textures, then atlases, then the batch:

```cpp
graphics.resume();
const bool preserved = graphics.resources_preserved();
texture.restore(preserved);
atlas.restore(preserved);
batch.restore(preserved);
```

`Texture::restore()` and `SpriteBatch::restore()` validate preserved native
handles with `glIsTexture`/`glIsProgram`/`glIsBuffer` and rebuild invalid
objects from the retained recipe, so a driver-reported preserved context
cannot silently retain stale objects.

## Errors

Every creation and restoration entrypoint returns `bool`. Network-free error
reporting is done through `SDL_Log` plus `SDL_SetError`/`SDL_GetError`
(for SDL errors) and `IMG_GetError` (for image decode failures). No
exceptions cross the backend boundary. Check the return values of
`Context::create`, `Texture::load/create_*`, `TextureAtlas::load`,
`SpriteBatch::initialize/restore`, and treat `false` as "resource creation
failed; recover or abort".

## Threading

All backend entrypoints must run on the thread that owns the OpenGL context
— the application's main thread. Textures, their regions, atlases, and the
batch are not thread-safe and never migrate GL work off that thread.

## Ownership and lifetime

- `Context` owns the window and context; only create/destroy touch them.
- `TextureRegion` and `AtlasRegion::region()` are non-owning views: the
  referenced `Texture` (and its owning atlas) must outlive every draw that
  uses them. `AtlasRegion` pointers become invalid when the atlas is
  destroyed or successfully reloaded.
- `Texture`, `TextureAtlas`, and `SpriteBatch` own their backend objects.
  Call `release()` to free GPU objects while keeping the recovery recipe,
  `invalidate()` during a context-loss background transition, and
  `restore()` after `resume()`.
- `SpriteBatch` owns shader programs, a vertex buffer, and an index buffer;
  you must call `initialize()` before `begin(camera)` returns `true`.

## Lua bindings

None of the types implemented or consumed by this backend have a Lua 5.4
binding.

## Related documentation

- Package payload: [Backend-SDL2-OpenGL.md](../../../packages/squared-backend-sdl2-opengl/content/docs/Backend-SDL2-OpenGL.md)
- Implementation details: [Squared SDL2/OpenGL Backend — Developer Guide](../developer/squared-backend-sdl2-opengl/README.md)
- Portable low-level contracts: [Squared Graphics — Programmer Guide](../squared-graphics/README.md)
- Portable 2D rendering types: [Squared Graphics2D — Programmer Guide](../squared-graphics2d/README.md)
- Documentation index: [Programmer documentation](../README.md)