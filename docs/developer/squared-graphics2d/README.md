# Squared Graphics2D — Developer Guide

Squared Graphics2D is the portable 2D rendering contract layer. It ships the
camera implementation and the pure data types (texture region views, sprite
state), while texture, atlas, and sprite-batch bodies are implemented by
exactly one link-time backend — the shipped backend is
squared-backend-sdl2-opengl. The portable headers contain no SDL, OpenGL, or
Android symbols; portable tests (`graphics2d-portable-test.cpp`) compile
against only the include paths of Graphics2D, Graphics, and Math.

- Programmer counterpart: [Squared Graphics2D — Programmer Guide](../programmer/squared-graphics2d/README.md)
- Package payload: [Graphics2D.md](../../packages/squared-graphics2d/content/docs/Graphics2D.md)
- Documentation index: [Developer documentation](../README.md)
- Diagrams:
  - [Texture-atlas page/region ownership](atlas-page-region-ownership.dot)
  - [SpriteBatch begin/draw/end sequence](sprite-batch-sequence.dot)
  - [Camera-to-batch transform flow](camera-batch-transform.dot)
  - [Framework package dependencies](../architecture/package-dependencies.dot)

## Dependency boundary

The manifest declares exactly two `module.requires`: `squared.graphics`
`0.6.0-dev.4` and `squared.math` `0.6.0-dev.2`. The CMake target
`squared_graphics2d` is a `STATIC` library that compiles
`src/orthographic_camera.cpp` and links `squared_graphics` and `squared_math`
(`content/modules/squared-graphics2d/CMakeLists.txt`).

The portable/backend boundary is strict: every public header includes only
Graphics (`color.hpp`), Math (`matrix4.hpp`, `vector2.hpp`), and the standard
library. `Texture`, `TextureAtlas`, and `SpriteBatch` declare their members in
`include/` but define the backend-dependent operations in the backend
package's `src/texture.cpp`, `src/texture_atlas.cpp`, and `src/sprite_batch.cpp`.
No backend header is included by the portable headers; the boundary is the
concrete class interface, not a virtual one.

## Architecture

| Component | Source | Responsibility |
| --- | --- | --- |
| `squared::graphics2d::Texture` | `include/.../texture.hpp`; backend `src/texture.cpp` | Move-only GPU texture: opaque `uint32` handle, dimensions, filter/wrap state, and a CPU recovery recipe selected by policy. |
| `TextureRecoveryOptions`/`TextureRecoveryTarget`/callback | `include/.../texture.hpp`; `texture.cpp` | Portable recovery selection and the synchronous upload destination for `Regenerate`. |
| `squared::graphics2d::TextureRegion` | `include/.../texture_region.hpp` | Non-owning rectangle view: pixel rect, rotation flag, derived UVs, `subregion`. |
| `squared::graphics2d::TextureAtlas` | `include/.../texture_atlas.hpp`; backend `src/texture_atlas.cpp` | Transactional libGDX `.atlas` parser; owns page textures and region values. |
| `squared::graphics2d::AtlasRegion` | `include/.../texture_atlas.hpp`; `texture_atlas.cpp` | One named entry: region view plus packed/original sizes, offsets, rotation, splits, pads. |
| `squared::graphics2d::Sprite` | `include/.../sprite.hpp` | Value state over one region: position, size, origin, scale, rotation, color. |
| `squared::graphics2d::SpriteBatch` | `include/.../sprite_batch.hpp`; backend `src/sprite_batch.cpp` | Ordered textured-quad batching with a built-in GLSL program and GPU buffers. |
| `squared::graphics2d::OrthographicCamera` | `src/orthographic_camera.cpp` | Logical viewport, pan, zoom, origin; computes the combined projection. |

## Ownership and threading

- **Texture is move-only.** It owns an opaque `uint32` GL handle plus a
  recovery recipe: an asset `std::string`, a `std::vector<std::uint8_t>` RGBA
  copy, or a `TextureRecoveryCallback`/`void*` pair. Move transfers all state;
  move-assign destroys the destination's prior backend object first. Only
  `destroy()` releases the GPU object and clears the recipe.
- **Atlas owns pages; regions are values.** `TextureAtlas` holds
  `std::vector<std::unique_ptr<Texture>> textures_` and
  `std::vector<AtlasRegion> regions_`. Each `AtlasRegion` contains a
  `TextureRegion` that points at one page texture; because pages are
  `unique_ptr`-owned and regions are values in a stable vector, object
  addresses survive `release()`/`invalidate()`/`restore()` cycles and change
  only on `destroy()` or a successful reload (which swaps both vectors).
- **Batch owns its staging and GPU state.** `SpriteBatch` holds a CPU
  `std::vector<float> vertices_` reserved to `maximum_sprites_ × 4 vertices ×
  8 floats`, plus `vertex_buffer_`, `index_buffer_`, `program_`, uniform
  locations, and `active_texture_`. `maximum_sprites_` is clamped to
  `[1, 16383]`; the 16-bit index format caps it.
- **Camera owns its matrix.** `OrthographicCamera` keeps viewport, position,
  zoom, origin, and the computed `Matrix4 combined_`; it is a plain value.
- **Main-thread only.** Every GPU-touching operation requires the current
  graphics context; the package does no cross-thread synchronization, and all
  types are confined to the main game-loop thread.

## Invariants and failure behavior

- **Recovery-recipe invariant.** After a successful `load`/`create_*`, the
  recipe matches the policy: `ReloadFromAsset` keeps a non-empty asset path,
  `RetainPixels` keeps a non-empty pixel buffer, `Regenerate` keeps a non-null
  callback, `Discard` keeps nothing. `restorable()` checks exactly this;
  `restore()` for `Discard` always returns `false`.
- **Context-loss contract.** `invalidate()` marks handles stale without GL
  calls (safe while rendering is unavailable). `release()` deletes GPU objects
  while retaining the recipe (context must be current). `restore(context_preserved)`
  first validates preserved handles via `glIsTexture`/`glIsProgram`/`glIsBuffer`
  when `context_preserved` is true; otherwise it rebuilds from the recipe.
  Successful restore keeps object addresses stable. `Texture::upload_rgba`
  restores the previous dimensions if the GL upload fails, so a failed restore
  does not corrupt reported size.
- **Atlas transactionality.** `TextureAtlas::load` never mutates existing
  state until a fully valid replacement exists: the text is parsed into local
  `ParsedPage`/`ParsedRegion` records, then page textures are loaded and
  validated into local `textures_`/`regions_` vectors, and only on complete
  success are the members replaced with `swap`. Parse validation rejects
  unsafe page paths, empty pages, non-positive region bounds, regions that
  exceed their page, duplicate name/index pairs, and unsupported filter/repeat
  values. A shared `Regenerate` page policy is rejected up front.
- **Batch invariants.** `begin` requires `valid() && !drawing_`.
  `append_quad` skips invalid regions silently and flushes when the active
  texture changes or the batch reaches capacity. `flush()` submits only when
  `drawing_`, non-zero sprite count, and a bound texture are all true; it
  uploads `vertices_` with `glBufferSubData` and resets the CPU count. `end()`
  flushes and clears GL state. Color components are clamped before upload so
  out-of-range values cannot reach GL.

## Data structures and complexity

- **Texture** — O(1) object: one handle, two `int` dimensions, two filter and
  two wrap enums, a policy, an asset path, an optional retained RGBA buffer,
  and a callback pointer. `retained_recovery_bytes()` is O(1), returning the
  pixel-buffer byte count.
- **TextureAtlas** — `regions_` is a contiguous `std::vector<AtlasRegion>`
  (good cache locality for lookups and iteration); `find_region` is a linear
  scan O(R) over all regions. Page load is O(P) textures plus O(R) region
  construction. `valid()` is O(P).
- **SpriteBatch** — CPU staging `vertices_` is reserved at initialization to
  `maximum_sprites × 32` floats, so `append_quad` is amortized O(1) with no
  mid-draw allocation; index generation at `allocate_gpu_objects` is O(S × 6)
  once. `flush` is O(buffered sprites) for the buffer upload plus O(1) draw
  call.
- **OrthographicCamera** — O(1) float arithmetic in `update()`; the
  `Matrix4::orthographic` computation is constant-time value math with no
  allocation.

## Algorithms and execution order

**Atlas load: parse → validate → commit.**

1. Read the atlas text; parse line-by-line into `ParsedPage`/`ParsedRegion`
   (page image line, then `filter`/`repeat` page properties, then per-region
   `rotate`, `xy`/`bounds`, `size`, `orig`, `offset`/`offsets`, `index`,
   `split`, `pad`). Blank lines close the current page/region. Unsupported or
   malformed values fail with a line-numbered error.
2. Validate the parse: at least one page, each page has at least one region,
   every region has non-negative bounds.
3. Load each page image relative to the atlas directory, apply the page's
   filter/wrap, and build `TextureRegion` views (swapping packed extents for
   rotated entries). Validate each region fits its page and no name/index pair
   duplicates.
4. Commit: `textures_.swap(...)` and `regions_.swap(...)` only after all pages
   and regions succeed.

**SpriteBatch sequence.** `initialize` allocates GPU objects (program, vertex
buffer, static 16-bit index buffer). `begin(camera)` uploads
`camera.combined().data()` to `u_projection` and enables blending. Each
`draw` appends 4 vertices of 8 floats (`x,y,r,g,b,a,u,v`) to the CPU staging
vector, flushing on texture switch or capacity. `flush` binds the active
texture, uploads the staging buffer, sets the three vertex attributes
(position/color/tex-coord), and issues a single `glDrawElements`. `end`
flushes and unbinds.

**Camera-to-batch transform flow.** `OrthographicCamera::update()` derives
half-extents from viewport and zoom, then calls `Matrix4::orthographic` with
`bottom`/`top` ordered so `TopLeft` maps increasing world Y downward and
`BottomLeft` upward. The column-major result is stored in `combined_`;
`SpriteBatch::begin` feeds `combined().data()` straight into the `u_projection`
uniform (`glUniformMatrix4fv` with `GL_FALSE`), so the vertex shader projects
each quad position with `u_projection * vec4(a_position, 0, 1)`.

## Design patterns

- **Strategy** — texture recovery: `TextureRecoveryPolicy` selects a recovery
  strategy (`ReloadFromAsset`, `RetainPixels`, `Regenerate`, `Discard`) at
  creation time, and `restore()` dispatches to the matching algorithm
  (`restore_asset`, retained-pixel upload, `restore_callback`, or none). Why
  it fits: recovery is a genuinely swappable behavior selected once per
  resource without per-restore conditionals in callers. Alternatives rejected:
  hard-coding asset reload for all textures (wastes memory or loses procedural
  content) and a callback-everywhere model (complicates simple asset use).
  Deviation: the strategy is an enum plus member-held data, not a polymorphic
  strategy object; there is no runtime strategy registry.
- **Template Method (resource restore chain)** — `Texture`, `TextureAtlas`,
  and `SpriteBatch` each expose the same `invalidate`/`release`/`restore`
  skeleton with the fixed order: mark-stale-or-release, validate preserved
  handles, rebuild from the retained recipe. The base step order is shared
  across the resource family (and mirrors `Context`'s generation lifecycle);
  each resource fills in its concrete rebuild step. This is what lets an
  application restore every resource with a single `resources_preserved()`
  value.
- **Non-owning view** — `TextureRegion` references a `Texture` without owning
  it and derives normalized UVs from the pixel rect. `Sprite` and `AtlasRegion`
  build on the same view. Why it fits: one GPU texture is shared by many
  logical views with zero GPU cost. Alternatives rejected: copying texture
  sub-images into per-region textures (wastes VRAM and complicates atlases).
  Deviation: the view is mutable only in the sense that `subregion` derives a
  new view; the underlying pointer is set once at construction.
- **Object pool / buffer reuse** — `SpriteBatch` reserves a fixed-capacity CPU
  vertex staging vector and a static GPU index buffer once, then re-fills and
  re-uploads the same GPU vertex buffer every flush. Why it fits: it removes
  per-frame allocation and driver object churn. Alternatives rejected: a new
  buffer per frame (driver overhead) or immediate-mode one quad at a time
  (many draw calls). Deviation: this is a per-object reuse buffer rather than
  a cross-object pool, and the index buffer is a static 16-bit identity
  pattern rather than a dynamic index generator.

## Limitations and technical debt

- **16-bit index cap.** `kMaximumIndexableSprites = 16383` bounds the batch
  capacity; larger scenes need multiple batches or a 32-bit index path (not
  implemented).
- **No mipmaps.** Filtering supports only `Nearest`/`Linear`; mipmapped
  filter names in atlas files are reported as unsupported rather than
  silently redefined.
- **Filter/repeat subset.** Only one min and one mag filter, and three wrap
  modes, exist; atlas `repeat` accepts only `none`/`x`/`y`/`xy` (no
  `MirroredRepeat` in the text format and no combined forms beyond those).
- **Rotation support is clockwise-only.** Atlas rotation is a single 90-degree
  clockwise flag; arbitrary angles are rejected during parse.
- **No camera frustum culling.** Every batched sprite is submitted regardless
  of visibility against the camera bounds.
- **No atlas overlap validation.** `TODO.md` records validating that page
  metadata rectangles do not overlap as the next unfinished item; only
  out-of-page and duplicate-name checks exist today.
- **No recovery-budget telemetry.** `TODO.md` records aggregate
  recovery-memory reporting across a texture collection as unfinished;
  `retained_recovery_bytes()` is per-texture only.
- **`Regenerate` is rejected as an atlas-wide page policy**, because pages
  need distinct recipes; per-page regeneration is not implemented.
- **No texture packing.** Generating atlases from loose images is a future
  asset-tools concern; `TextureAtlas` only consumes existing `.atlas` files.
