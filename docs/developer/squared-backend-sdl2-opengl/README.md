# Squared SDL2/OpenGL Backend — Developer Guide

This package is the link-time realization of the portable Squared Graphics
and Graphics2D contracts on SDL2 with OpenGL ES 2. Its four translation units
implement all backend-facing methods of the portable classes and never add a
public API of their own. Selecting the package is a dependency decision made
by the platform template; there is no runtime registry and no virtual
dispatch in the frame loop.

- Programmer counterpart: [Squared SDL2/OpenGL Backend — Programmer Guide](../programmer/squared-backend-sdl2-opengl/README.md)
- Package payload: [Backend-SDL2-OpenGL.md](../../packages/squared-backend-sdl2-opengl/content/docs/Backend-SDL2-OpenGL.md)
- Documentation index: [Developer documentation](../README.md)
- Diagrams:
  - [Backend initialization and shutdown](init-shutdown.dot)
  - [Texture and atlas loading and GPU resource lifetime](texture-atlas-lifecycle.dot)
  - [SpriteBatch submission path](sprite-batch-submission.dot)

## Dependency boundary

The manifest declares exactly one requirement: `squared.graphics2d` version
`0.6.0-dev.8`, which itself requires Graphics `0.6.0-dev.4` and Math
`0.6.0-dev.2`. The CMake module `squared_backend_sdl2_opengl` is a `STATIC`
library, `POSITION_INDEPENDENT_CODE ON`, compiled with `-Wall -Wextra
-Wpedantic`, and it links `squared_graphics2d`, `SDL2_image`, `SDL2`, and the
configured `SQUARED_GLES2_LIBRARY`. Configuration fails fast unless the
`SDL2` and `SDL2_image` targets exist, `SDL2_INCLUDE_DIR/SDL.h` resolves, and
`SQUARED_GLES2_LIBRARY` is set.

Direction of dependence matters: portable packages never include SDL or GL
headers; only this backend and the platform template do. The SDL include
directory is added `PRIVATE`, and the portable headers `color.hpp`,
`context.hpp`, and the graphics2d headers contain only standard-library
includes. `tests/graphics2d-portable-test.cpp` proves the portable boundary
compiles without SDL or GL.

## Architecture

| Class (declared in portable package) | Source file | Responsibilities implemented here |
| --- | --- | --- |
| `squared::graphics::Context` | `src/context.cpp` | Window creation, GL attributes, create/suspend/resume, viewport, clear, present, generation counter, preserved flag. |
| `squared::graphics2d::Texture`, `TextureRecoveryTarget` | `src/texture.cpp` | SDL2_image decode, RGBA conversion, generate/upload, filters/wrap, retention recipes, release/invalidate/restore. |
| `squared::graphics2d::TextureAtlas`, `AtlasRegion` | `src/texture_atlas.cpp` | libGDX `.atlas` parser, page loads, rotation/trimming metadata, transactional commit, restore fan-out. |
| `squared::graphics2d::SpriteBatch`, `SpriteBatch::begin/draw/end/flush` | `src/sprite_batch.cpp` | Built-in GLSL ES shader, buffers, quad append, flush, sprite transform math, recovery. |

`OrthographicCamera` and `Sprite` are fully implemented in the portable
graphics2d package; the backend only consumes their data.

## Ownership and threading

- `Context` owns the `SDL_Window*` and `SDL_GLContext` (as `void*` in the
  portable header). `destroy()` tears down both; `suspend()` releases only
  the context.
- Every `Texture` owns a GL texture object plus the recovery recipe it was
  created with: an asset path (`ReloadFromAsset`), a retained pixel buffer
  (`RetainPixels`), or a callback and user-data pointer (`Regenerate`,
  non-owning). `Texture::release()` deletes the GPU object and keeps the
  recipe; `invalidate()` marks it stale without GL calls.
- `TextureAtlas` owns `textures_` (`std::vector<std::unique_ptr<Texture>>`)
  and `regions_` (`std::vector<AtlasRegion>`); `AtlasRegion` holds a
  non-owning `TextureRegion` into the owning page texture.
- `SpriteBatch` owns the compiled program, the `GL_DYNAMIC_DRAW` vertex
  buffer, the `GL_STATIC_DRAW` index buffer, and the CPU vertex vector.
- All GL use happens on one thread (the application main thread). There is
  no GL context sharing and no cross-thread synchronization.

## Invariants and failure behavior

- A valid context must be current for every texture create, release,
  restore, delete, and for all batch/atlas GPU work. `Context::valid()`
  reflects `window_ && native_context_`.
- Context loss invalidates GL names but leaves CPU recipes intact.
  `resources_preserved()` is only `true` on the fast-path `SDL_GL_MakeCurrent`
  reactivation of a retained context; a fresh replacement context reports
  `false`. Even on `true`, the backend revalidates with `glIsTexture`,
  `glIsProgram`, and `glIsBuffer` before reuse, so stale objects are never
  silently retained.
- After `SDL_APP_WILLENTERBACKGROUND`, the backend issues no GL calls. The
  adapter calls `invalidate()` on resources and only `resume()`s after
  `SDL_APP_DIDENTERFOREGROUND`.
- Texture allocation checks `width > 0`, `height > 0`, non-null pixels, and
  overflow of the `width * height * 4` byte count before reserving memory.
- `upload_rgba()` is transactional at the GPU level: it clears GL errors,
  generates a text object, uploads, applies filters and wraps, and on any
  recorded error deletes the object and restores the previous dimensions.
- `TextureAtlas::load()` is transactional: any failure anywhere in asset
  read, parse, page load, bounds check, or duplicate check returns `false`
  and leaves the previously loaded atlas untouched (`textures_.swap`/
  `regions_.swap` happen only at the end).
- Atlas page recovery policy may not be `Regenerate`; that is rejected
  explicitly because pages would need distinct application recipes.

## Data structures and complexity

- `Texture` members: `handle_` (GLuint), `width_`/`height_` (int),
  `recovery_policy_`, `asset_path_` (std::string), `pixels_`
  (`std::vector<std::uint8_t>` sized `width * height * 4` under
  `RetainPixels`), callback pointers, filter/wrap state, and `invalidated_`
  flag. `retained_recovery_bytes()` reports `pixels_.size()`.
- `TextureAtlas` holds `textures_` (page-count) and `regions_`
  (total-region-count). `find_region(name, index)` is a linear `find_if`
  over `regions_`, so lookup is O(n) in region count; n is typically tens to
  hundreds for a UI atlas. Page/region vectors are reserved up front after
  parsing, so no reallocation churn during loading.
- `SpriteBatch` CPU side is one `std::vector<float> vertices_` reserved to
  `maximum_sprites_ * 4 vertices * 8 floats` (32 floats per sprite). GPU
  objects: a vertex buffer sized the same in bytes with `GL_DYNAMIC_DRAW`
  and a static index buffer of `maximum_sprites_ * 6` `std::uint16_t`
  entries. Maximum indexable sprites is `16383` because the index type is
  `GL_UNSIGNED_SHORT` (16383 * 4 = 65532 vertices stays within 16-bit
  indices). `maximum_sprites_` is clamped into `[1, 16383]`.

## Algorithms and execution order

### Initialization and shutdown

`SDL_Init`/subsystem initialization belongs to the platform template
(`sdl_main.cpp`), not this package. This package's lifecycle begins at
`Context::create()`: set the ES 2.0 + double-buffer + RGBA 8/8/8/8 + depth 0
+ stencil 0 attributes, create the window (`SDL_WINDOW_OPENGL | SHOWN |
ALLOW_HIGHDPI`, centered), then `resume()`. Shutdown is the reverse:
`Context::suspend()` drops the context, `destroy()` drops the window.

`resume()` orders: try to reactivate the retained context (fast path); on
`MakeCurrent` failure delete it and rebuild; set best-effort swap interval
and log `GL_VERSION`; refresh the drawable; verify positive pixel
dimensions; bump `generation_` (skipping zero) and set
`resources_preserved_`.

### Texture loading and recovery

`Texture::load`: `IMG_Load` → `SDL_ConvertSurfaceFormat` to
`SDL_PIXELFORMAT_ABGR8888` → build the retention recipe according to the
policy (keep path, copy pixels, or record the callback) → `upload_rgba` →
record policy/recipe only if upload succeeded. `restore(context_preserved)`:
when `invalidated_` and `context_preserved` and
`glIsTexture(handle_) == GL_TRUE`, clear the flag and return; otherwise drop
the handle and dispatch on the policy — `ReloadFromAsset` reloads via
`restore_asset`, `RetainPixels` re-uploads `pixels_`, `Regenerate` calls the
callback into a fresh `TextureRecoveryTarget` (which must call
`upload_rgba` synchronously, exactly once), and `Discard` fails.

### Atlas transactional pipeline

`TextureAtlas::load(path, policy)`: reject `Regenerate`; `read_asset`
(SDL_RWops) → `parse_atlas` into local `ParsedPage`/`ParsedRegion` vectors —
line-oriented, `key: value` after a page image line; strict integer parsing
with `std::from_chars`; filter limited to `Nearest`/`Linear`, repeat to
`none`/`x`/`y`/`xy`, rotate to `false`/`true`/`0`/`90`; page images must be
safe relative paths (no absolute path, no `..` segment) — then for each
parsed page load the page texture, apply its filter and wrap, build every
`AtlasRegion` (rotated entries swap storage extents so `packed_width` stays
logical), validate each region lies inside its page and that no name/index
pair is duplicated, and only then `textures_.swap`/`regions_.swap`. Any
exception or failure aborts with the prior state intact.

`release()`/`invalidate()` fan out to every page texture; `restore()` loops
pages in order and calls `release()` when one page cannot recover, returning
`false`.

### SpriteBatch submission

`begin(camera)`: guard `valid() && !drawing_`; reset `active_texture_`,
`sprite_count_`, and `vertices_`; `glUseProgram`, upload
`camera.combined()` to `u_projection`, bind `u_texture = 0`, enable blend
`SRC_ALPHA`/`ONE_MINUS_SRC_ALPHA`, disable depth test.

`draw(region, ...)`/`draw(sprite)` call `append_quad`, which flushes when the
texture changes or capacity is reached, appends 32 floats per quad
(x,y,rgba,u,v), and increments `sprite_count_`. Sprite draws compute the
transformed quad corners: position plus origin, local extents scaled by
`scale_x`/`scale_y`, rotated clockwise by `rotation()` degrees around the
origin.

`flush()`: bind the active texture and buffers, `glBufferSubData` the CPU
vertex range (dynamic buffer, no full reallocation), set the three vertex
attribute pointers (a_position at location 0 = 2 floats, a_color at 1 = 4
floats, a_tex_coord at 2 = 2 floats; stride 32 bytes), `glDrawElements`
`GL_TRIANGLES` with `sprite_count_ * 6` `GL_UNSIGNED_SHORT` indices, then
clear counts. `end()` flushes and unbinds. `initialize()` compiles the
built-in GLSL ES 1.00 shaders (attributes bound with `glBindAttribLocation`
before link), builds the static index pattern (0,1,2, 2,3,0), creates the
buffers, and verifies uniform locations and a clean error state.

## Design patterns

- **Facade** — `Context` (and to a lesser degree `Texture`/`SpriteBatch`)
  hide SDL2 windowing, EGL, shader, and buffer detail behind the portable
  contracts. Portable code depends on the facade, never on SDL/GL.
- **Strategy** — `TextureRecoveryPolicy` selects the post-context-loss
  algorithm (`ReloadFromAsset`, `RetainPixels`, `Regenerate`, `Discard`) via
  a switch in `restore()`. Deviation: the textbook
  Strategy uses virtual strategy objects; the enum-plus-switch is a
  degenerate, allocation-free form chosen so recipes stay copyable and
  serializable in the portable header.
- **Template Method (resource-generation lifecycle)** — each recoverable
  resource follows the same `create/upload` → `release`/`invalidate` →
  `restore(context_preserved)` skeleton, with the steps filled by the
  policy or the caller; `Context::resume` is the fixed outer skeleton
  (`MakeCurrent`/rebuild → viewport → generation).
- **Swing (transactional commit)** — `TextureAtlas::load` and (in Squared
  GUI) skin loading build an entire new state in local buffers and commit
  with a single `swap`; failure in the prepare phase leaves the committed
  state untouched. Deviation: the pattern here is two-phase (prepare +
  swap) with no rollback journal, which is correct because the prepare phase
  owns all freshly created objects until commit.
- **Object Pool** — `SpriteBatch` reuses a heap of preallocated vertex slots
  (and a fixed index buffer) across all draw calls and frames. Deviation:
  pool members are raw float slots rather than discrete objects, and
  "release" is the per-flush reset of `sprite_count_`/`vertices_`, not an
  explicit per-object return.

Alternatives rejected: a runtime backend registry (startup cost, failure
surface, and a second backend could be linked accidentally); per-frame
`glBufferData` reallocation (rejected in favor of `GL_DYNAMIC_DRAW` +
`glBufferSubData`); dynamic per-render shader generation (rejected — the
built-in program covers all batch drawing).

## Limitations and technical debt

- **Atlas format limits**: filters are restricted to `Nearest` and `Linear`;
  `cube` and `MipMap*` values are rejected; `repeat` is limited to
  `none`/`x`/`y`/`xy` (no mirror modes in the text format even though
  `TextureWrap::MirroredRepeat` exists in GL). There is no mipmap
  generation.
- **16-bit index cap**: the batch cannot draw more than 16383 sprites in one
  buffer because indices are `GL_UNSIGNED_SHORT`; exceeding the cap silently
  leaves extra sprites unbuffered.
- **GLES 2 only**: the shipped backend targets ES 2.0; `TODO.md` records
  OpenGL ES 3 evaluation as later work.
- **Depth and stencil are disabled** (depth 0, stencil 0), appropriate for
  the 2D profile but a constraint for layered effects.
- **Textures do not generate mipmaps** and `TextureFilter` has no mipmap
  modes, which limits minification quality at distance.
- **Flush-on-texture-switch** forces callers to batch by texture to keep draw
  calls low; interleaved geometry across many textures loses batching.
- **Backend is SDL2-specific**: windowing, events, and asset reads are tied
  to SDL/RWops; a future backend would need equivalent services, and an
  SDL-less platform cannot use this package without changes.
- **No audio, no font rasterization, no file-format policy beyond
  SDL2_image**: those live outside this package's scope (the showcase
  template adds SDL_mixer/SDL_net/SDL_ttf separately).
- **Regenerate is rejected at the atlas level**, so atlas pages cannot use
  application callbacks; only `ReloadFromAsset`, `RetainPixels`, and
  `Discard` are available.
- `TODO.md` records unfinished repeatable on-device stress coverage for
  background/foreground/orientation recovery and diagnostics for unsupported
  page formats.