# Squared Graphics2D — Programmer Guide

Squared Graphics2D defines the portable 2D rendering contracts of the
framework: portable bitmap fonts and UTF-8 glyph layout, textures with
selectable context-loss recovery, non-owning texture subregions,
transactionally loaded libGDX texture atlases, sprites, an ordered sprite
batch, and orthographic cameras. The package contains no SDL,
OpenGL, or Android headers; an application draws through these portable types
and the platform template selects the backend implementation at link time.

## Package availability

| Module | Version | Requires |
| --- | --- | --- |
| `dev.squarednetizen.squared.graphics2d` | `0.6.0-dev.8` | `dev.squarednetizen.squared.graphics` `0.6.0-dev.4`, `dev.squarednetizen.squared.math` `0.6.0-dev.2` |

The CMake target is `squared_graphics2d`, a `STATIC` library. Camera math
lives in this package; texture, atlas, and batch bodies are provided by the
selected link-time backend.

## Public API overview

| Type | Header | Purpose |
| --- | --- | --- |
| `squared::graphics2d::Texture` | `squared/graphics2d/texture.hpp` | Move-only texture owning a backend GPU object, with a selected recovery policy. |
| `squared::graphics2d::TextureFilter` | `squared/graphics2d/texture.hpp` | Minification/magnification filter: `Nearest`, `Linear`. |
| `squared::graphics2d::TextureWrap` | `squared/graphics2d/texture.hpp` | Wrap mode: `ClampToEdge`, `Repeat`, `MirroredRepeat`. |
| `squared::graphics2d::TextureRecoveryPolicy` | `squared/graphics2d/texture.hpp` | Recovery after context loss: `ReloadFromAsset`, `RetainPixels`, `Regenerate`, `Discard`. |
| `squared::graphics2d::TextureRecoveryOptions` | `squared/graphics2d/texture.hpp` | Recovery selection (policy, callback, user data) with factory helpers. |
| `squared::graphics2d::TextureRecoveryTarget` | `squared/graphics2d/texture.hpp` | Synchronous upload destination given to a regeneration callback. |
| `squared::graphics2d::TextureRecoveryCallback` | `squared/graphics2d/texture.hpp` | Function-pointer signature for `Regenerate`. |
| `squared::graphics2d::TextureRegion` | `squared/graphics2d/texture_region.hpp` | Non-owning rectangular view into a `Texture` with UV coordinates. |
| `squared::graphics2d::AtlasRegion` | `squared/graphics2d/texture_atlas.hpp` | One named atlas entry: region view plus libGDX metadata. |
| `squared::graphics2d::TextureAtlas` | `squared/graphics2d/texture_atlas.hpp` | Owning, transactionally loaded libGDX text atlas. |
| `squared::graphics2d::BitmapFont` | `squared/graphics2d/bitmap_font.hpp` | Value-semantic, transactionally parsed text BMFont metrics and page references. |
| `squared::graphics2d::GlyphLayout` | `squared/graphics2d/bitmap_font.hpp` | Reusable UTF-8 layout producing page-indexed glyph placements. |
| `squared::graphics2d::Sprite` | `squared/graphics2d/sprite.hpp` | Lightweight mutable draw state over one region. |
| `squared::graphics2d::SpriteBatch` | `squared/graphics2d/sprite_batch.hpp` | Ordered textured-quad batching with an internal shader. |
| `squared::graphics2d::CoordinateOrigin` | `squared/graphics2d/orthographic_camera.hpp` | Camera orientation: `BottomLeft`, `TopLeft`. |
| `squared::graphics2d::OrthographicCamera` | `squared/graphics2d/orthographic_camera.hpp` | 2D pan/zoom camera producing a projection matrix for the batch. |

## Creating and loading a Texture

`Texture` is move-only and owns one backend GPU texture. Creation requires a
current graphics context. `load` decodes an image asset; `create_rgba` uploads
tightly packed RGBA8888 pixels; `create_solid` builds a one-pixel texture.

```cpp
#include <squared/graphics2d/texture.hpp>

squared::graphics2d::Texture hero;
if (!hero.load("assets/hero.png")) {
    // Decode or upload failed; the backend logged the reason.
    return;
}

squared::graphics2d::Texture mask;
const unsigned char four_pixels[] = {
    0xFF, 0x00, 0x00, 0xFF,    // one red texel
    0x00, 0xFF, 0x00, 0xFF,    // one green texel
    0x00, 0x00, 0xFF, 0xFF,    // one blue texel
    0xFF, 0xFF, 0xFF, 0xFF     // one white texel
};
if (!mask.create_rgba(2, 2, four_pixels)) {
    return;
}

squared::graphics2d::Texture white;
white.create_solid(squared::graphics::Color::white());
```

`load(path)` defaults to `ReloadFromAsset` recovery; `create_rgba` and
`create_solid` default to `RetainPixels`. `width()` and `height()` return the
texture dimensions in pixels; `valid()` reports whether a backend object
exists. Filtering and wrapping are per-texture:

```cpp
hero.set_filter(
    squared::graphics2d::TextureFilter::Linear,
    squared::graphics2d::TextureFilter::Linear
);
hero.set_wrap(
    squared::graphics2d::TextureWrap::Repeat,
    squared::graphics2d::TextureWrap::ClampToEdge
);
```

## Regions and UVs

`TextureRegion` is a non-owning view: it references a `Texture` (which must
outlive every use), a pixel rectangle in top-left image coordinates, an
optional 90-degree clockwise rotation flag, and the derived normalized UV
coordinates `u1`/`v1`/`u2`/`v2` in `[0, 1]`. The default camera orientation is
also top-left, so image and draw coordinates agree.

```cpp
#include <squared/graphics2d/texture_region.hpp>

squared::graphics2d::TextureRegion full(hero);              // entire texture
squared::graphics2d::TextureRegion icon(hero, 0, 0, 32, 32); // 32x32 corner

const float left = icon.u1();
const float top = icon.v1();
const float right = icon.u2();
const float bottom = icon.v2();

// A smaller logical view inside the region; empty when bounds are invalid.
const squared::graphics2d::TextureRegion sub = icon.subregion(4, 4, 16, 16);
if (sub.valid()) {
    // draw sub
}
```

`subregion` maps coordinates correctly even when the containing region is
rotated. Invalid bounds (negative, non-positive size, or beyond the region)
return an empty region. There is also a constructor that builds an unbacked
logical region (dimensions and UVs, no texture) for recording and validation;
it is not renderable.

## Atlas loading and find_region

`TextureAtlas` reads the libGDX text `.atlas` format. Page image paths are
relative to the atlas file. The loader supports multiple pages, indexed
duplicate names, 90-degree rotation, trimming (orig/offset), nine-patch
`split`/`pad`, nearest or linear filtering, and `none`/`x`/`y`/`xy` repeat.

```cpp
#include <squared/graphics2d/texture_atlas.hpp>

squared::graphics2d::TextureAtlas atlas;
if (!atlas.load("assets/ui.atlas")) {
    // Malformed metadata or a missing page; any prior valid atlas is intact.
    return;
}

const squared::graphics2d::AtlasRegion* button =
    atlas.find_region("button");
if (button) {
    const squared::graphics2d::TextureRegion& region = button->region();
    const int original_width = button->original_width();
    const bool rotated = button->rotated_clockwise();
    // optional nine-patch metadata:
    const auto& splits = button->splits();
    const auto& pads = button->pads();
}
```

`find_region(name, index)` matches an exact name/index pair (`index = -1` for
the default entry) and returns `nullptr` on a miss. Indexed duplicates are
retrieved by index. The atlas owns every page `Texture`; returned
`AtlasRegion` pointers and their `TextureRegion` views stay valid until the
atlas is destroyed or a later load succeeds.

## Bitmap fonts and glyph runs

`BitmapFont` parses text BMFont descriptors without loading a texture or
depending on a filesystem. Supply bytes from HoloDisk, an embedded resource,
or another application-owned source. A successful load owns the face,
line-height, baseline, page size, safe relative page filenames, glyph metrics,
and kerning pairs. A failed load reports a stable `BitmapFontErrorCode` plus a
line when applicable and leaves the previous font unchanged.

```cpp
#include <squared/graphics2d/bitmap_font.hpp>

squared::graphics2d::BitmapFont font;
squared::graphics2d::BitmapFontError error;
if (!font.load(font_descriptor_bytes, error)) {
    // error.code, error.line, error.message
    return;
}

squared::graphics2d::GlyphLayoutOptions options;
options.scale = 1.5F;
options.target_width = 320.0F;
options.alignment = squared::graphics2d::GlyphAlignment::center;

squared::graphics2d::GlyphLayout title;
if (!title.set_text(font, "Squared \xE2\x96\xA1", options, error)) {
    return;
}

for (const auto& glyph : title.glyphs()) {
    // Resolve font.pages()[glyph.page] to a Texture, create a TextureRegion
    // from glyph.source_*, then batch.draw(region, glyph.x, glyph.y,
    //                                      glyph.width, glyph.height).
}
```

Layout is strict UTF-8 by default. Set `reject_invalid_utf8 = false` to emit
the configured replacement glyph instead. Tabs expand to `tab_spaces` spaces;
CR, LF, and CRLF create explicit lines. Kerning is applied before placement,
and start/center/end alignment shifts each line inside `target_width`.

The page resolver owns textures and must keep them alive through submission;
`BitmapFont`, `GlyphLayout`, and `GlyphPlacement` own no GPU resources or
borrowed texture pointers. Word wrapping, complex-script shaping,
bidirectional layout, markup, and font fallback are not implemented.

## Sprite construction

`Sprite` holds a reference to a region plus position, size, transform origin,
scale, clockwise rotation, and a tint color. It is lightweight mutable state;
the region must outlive the sprite.

```cpp
#include <squared/graphics2d/sprite.hpp>

squared::graphics2d::Sprite card(icon);
card.set_position(40.0F, 20.0F);      // top-left in logical pixels
card.set_size(64.0F, 64.0F);          // unscaled logical size
card.set_origin(32.0F, 32.0F);        // transform origin relative to top-left
card.set_scale(2.0F, 2.0F);
card.set_rotation(45.0F);             // clockwise degrees
card.set_color(squared::graphics::Color::white());  // tint
```

## SpriteBatch begin/draw/end with an OrthographicCamera

`SpriteBatch` renders ordered textured quads efficiently. The lifecycle is
`initialize(capacity)` → `begin(camera)` → `draw(...)` → `end()`.

```cpp
#include <squared/graphics2d/sprite_batch.hpp>
#include <squared/graphics2d/orthographic_camera.hpp>

squared::graphics2d::OrthographicCamera camera(960.0F, 540.0F);
squared::graphics2d::SpriteBatch batch;
if (!batch.initialize(2048)) {
    return;  // shader/buffer allocation failed; logged by the backend
}

camera.set_position(480.0F, 270.0F);
camera.set_zoom(1.0F);
camera.update();   // recompute the projection matrix

if (batch.begin(camera)) {
    batch.draw(icon, 100.0F, 80.0F, 64.0F, 64.0F);
    batch.draw(card);
    batch.end();   // flushes the queued quads
}
```

- `initialize(maximum_sprites = 2048)` allocates the GPU buffers and compiles
  the built-in sprite shader. The capacity is clamped to `[1, 16383]`; the
  upper bound comes from the 16-bit index buffer. Pass the same capacity on
  restore so the batch can rebuild exactly what it had.
- `begin(camera)` uploads `camera.combined()` as the shader's projection
  uniform and returns `false` when the batch is not initialized or already
  drawing.
- `draw(region, x, y, w, h, color)` draws an untransformed quad; `draw(sprite)`
  draws a transformed sprite. Colors are clamped before upload; opaque white
  preserves the texture unchanged.
- `flush()` submits queued quads without ending the batch. A flush happens
  automatically when the texture changes and when the batch reaches capacity.
  `end()` flushes and ends.
- Draw calls targeting an invalid region are skipped silently.

## Coordinate conventions

`OrthographicCamera` has a logical viewport and a coordinate origin. The
default `CoordinateOrigin::TopLeft` matches image coordinates: positive X
points right and positive Y points down. `BottomLeft` keeps X rightward but
makes Y point up, mirroring classic math/GL orthographic conventions. The
projection is recomputed by `update()` after changing viewport, position, or
zoom.

```cpp
squared::graphics2d::OrthographicCamera top_left(960.0F, 540.0F);
squared::graphics2d::OrthographicCamera bottom_left(
    960.0F, 540.0F,
    squared::graphics2d::CoordinateOrigin::BottomLeft
);

top_left.set_position(480.0F, 270.0F);  // default is the viewport center
top_left.set_zoom(2.0F);                // magnify; clamped at a safe minimum
top_left.update();
```

The viewport is floored at 1.0 logical unit and the zoom at a positive
minimum, so a valid projection is always producible. `position()` and
`zoom()` report the active values.

## Recovery policy selection

Each texture chooses one recovery policy at creation time, and the choice
stays entirely inside the portable API. The recovery options carry no backend
types.

| Policy | CPU recovery storage | Context-loss behavior |
| --- | --- | --- |
| `ReloadFromAsset` | Asset path | Decode and upload the asset again. |
| `RetainPixels` | One RGBA8888 copy | Upload the retained pixels. |
| `Regenerate` | Callback and caller-owned pointer | Ask application code to synchronously provide new pixels. |
| `Discard` | None | Remain invalid; the owner recreates or removes the resource. |

```cpp
#include <squared/graphics2d/texture.hpp>

bool regenerate(void* user_data, squared::graphics2d::TextureRecoveryTarget& target) noexcept
{
    const auto* pixels = static_cast<const std::uint8_t*>(user_data);
    return target.upload_rgba(64, 64, pixels);
}

std::uint8_t owned_pixels[64 * 64 * 4] = {};

squared::graphics2d::Texture animated;
const auto options = squared::graphics2d::TextureRecoveryOptions::regenerate(
    &regenerate, owned_pixels
);
animated.create_rgba(64, 64, owned_pixels, options);
```

`load(path)` defaults to `ReloadFromAsset`; `create_rgba` and `create_solid`
default to `RetainPixels`. `Regenerate` requires a non-null callback and a
caller-owned `user_data` that must stay alive while restoration is possible;
the callback receives a `TextureRecoveryTarget` whose `upload_rgba` consumes
the pixels synchronously for that call only. The callback and public headers
never refer to SDL, OpenGL, or Android.

`recovery_policy()` reports the selection and `retained_recovery_bytes()`
reports only the RGBA bytes held for `RetainPixels`; asset paths, callback
user state, backend allocations, and bookkeeping are not counted.
`restorable()` reports whether the stored recipe can rebuild the texture.

Atlas-wide page recovery accepts `ReloadFromAsset`, `RetainPixels`, or
`Discard`; `Regenerate` is rejected because different pages require distinct
application recipes. The default is asset reload.

## Context restore obligations

Textures, atlases, and batches participate in a three-step recovery cycle used
when the graphics context is lost (for example Android lifecycle events):

1. While rendering is unavailable, call `invalidate()` on each resource. It
   marks backend handles stale without issuing graphics calls.
2. When rendering is available again, restore each resource with the context's
   preservation status:
   ```cpp
   hero.invalidate();
   atlas.invalidate();
   batch.invalidate();

   // ... context resume; read graphics.resources_preserved() ...
   const bool preserved = graphics.resources_preserved();
   hero.restore(preserved);
   atlas.restore(preserved);
   batch.restore(preserved);
   ```
3. If restoration fails (`restore` returns `false`), the resource remains
   unusable and the application should rebuild or remove it. `release()` is
   available instead when the context is known to be current and the recipe
   should be kept.

`TextureAtlas::restore` preserves page-object and region-object addresses, so
existing sprites and GUI drawables do not acquire dangling texture references.
`SpriteBatch::restore` rebuilds the shader and buffers at the configured
capacity when required.

## Errors and failure behavior

- Every creation, load, restore, and begin operation returns `bool`.
  Failures return `false` and, where a backend implements the SDL error
  channel, the reason is available through `SDL_GetError()` and `SDL_Log`.
- `Texture::load`/`create_rgba`/`create_solid` return `false` for null or
  non-positive arguments, decode/upload failures, overflow-sized buffers, and
  invalid recovery selections (for example `ReloadFromAsset` on an RGBA
  texture, or `Regenerate` without a callback).
- `TextureAtlas::load` is transactional: a parse error, an unsafe or missing
  page path, a region exceeding its page, or a duplicate name/index pair
  leaves any existing valid atlas untouched and returns `false`.
- `find_region` returns `nullptr` on a miss; `subregion` returns an empty
  region for invalid bounds; `SpriteBatch::draw` skips invalid regions
  silently; `TextureRegion::valid()` distinguishes renderable views.
- `OrthographicCamera` operations are `noexcept`; viewport and zoom are
  clamped rather than failing.

## Ownership and lifetime

- `Texture` is move-only and owns the backend GPU object and its recovery
  recipe (path, retained pixels, or callback); only `destroy()` releases both.
- `TextureRegion` is non-owning: it references a `Texture` that must outlive
  every region and every draw that uses it. Copy freely.
- `TextureAtlas` owns every page `Texture`; region objects are values inside
  the atlas and are invalidated when the atlas is destroyed or successfully
  reloaded.
- `BitmapFont` and `GlyphLayout` are independent values. Page filenames and
  metrics are owned strings/values; a later renderer owns resolved textures.
- `Sprite` references a region non-owningly and must not outlive it.
- `SpriteBatch` owns its CPU vertex staging buffer, GPU buffers, and compiled
  shader; it is non-copyable and non-movable.
- `OrthographicCamera` is an independent value holding its own projection
  matrix; it must simply outlive the `begin(camera)` calls that reference it.

## Threading

All types must be used from the main thread — the thread that owns the
graphics context and the game loop. GL objects are thread-bound, and the
package performs no cross-thread synchronization.

## Lua bindings

None of the types in this package have a Lua 5.4 binding.

## Related documentation

- Package payload: [Graphics2D.md](../../../packages/squared-graphics2d/content/docs/Graphics2D.md)
- Implementation details: [Squared Graphics2D — Developer Guide](../developer/squared-graphics2d/README.md)
- The link-time implementation backend: [Squared SDL2/OpenGL — Programmer Guide](../squared-backend-sdl2-opengl/README.md)
- Documentation index: [Programmer documentation](../README.md)
