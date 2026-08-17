---
title: Squared Graphics2D
tags:
  - cpp
  - graphics
  - opengl-es
---

# Graphics2D

Applications render through portable `squared::graphics2d` contracts.
Platform templates select one implementation backend at link time; ordinary
application drawing does not include SDL, Android, or raw OpenGL headers.

## Minimum API

- The independent Squared Graphics module provides
  `squared::graphics::Context`, which owns the window, context, viewport,
  clear, and presentation operations.
- `squared::graphics2d::Texture` owns a backend texture.
- `squared::graphics2d::TextureRegion` references a rectangle within a texture.
- `squared::graphics2d::TextureAtlas` owns one or more page textures and
  exposes named regions.
- `squared::graphics2d::Sprite` stores a region, transform, and color.
- `squared::graphics2d::SpriteBatch` batches ordered textured quads.
- `squared::graphics2d::OrthographicCamera` maps logical 2D coordinates to the
  framebuffer.

The default camera uses a top-left origin: positive X points right and positive
Y points down. Texture regions also use top-left image coordinates.
`TextureRegion::subregion` creates another non-owning logical view and maps
coordinates correctly even when the containing atlas region is stored with a
clockwise rotation. Invalid subregion bounds return an empty region.

## Texture atlases

`TextureAtlas` reads the libGDX text `.atlas` format. Page image paths are
relative to the atlas file. The loader supports multiple pages, indexed
duplicate region names, 90-degree rotation, trimmed-image original sizes and
offsets, nine-patch `split` and `pad` metadata, nearest or linear filtering,
and `none`, `x`, `y`, or `xy` repeat modes.

The atlas owns every page texture. Returned `AtlasRegion` pointers and their
`TextureRegion` views remain valid until the atlas is destroyed or a later
load succeeds. Loading is transactional: malformed metadata or a missing page
leaves an existing valid atlas untouched.

Mipmapped filter names and arbitrary rotation angles are not implemented yet.
The loader reports those unsupported values instead of silently changing
their meaning. Texture packing is also intentionally absent from the project
generator; it belongs to the future framework asset-tools API.

The generated sample includes `graphics/lifecycle-status.atlas`. Its square is
blue at process start and toggles green when touched. After backgrounding and
returning, green means the same process state survived; blue means the
activity/process was recreated. This is a diagnostic, not persisted
application state.

If the atlas cannot load, the application stays open and draws a red square
from its fallback texture. The bundled atlas uses a standard PNG page, matching
normal libGDX TexturePacker output and the explicitly initialized SDL_image PNG
decoder.

Public headers live beneath `include/squared/`. Doxygen discovers new
subdirectories recursively, so developers may organize additional framework
or application code without editing the documentation configuration.

## Graphics-context recovery

`Texture`, `TextureAtlas`, and `SpriteBatch` support a three-step recovery
cycle. `invalidate()` marks native handles stale without issuing graphics
commands, which is safe after Android reports that rendering is unavailable.
Once a context is current, `restore(context.resources_preserved())` either
validates preserved handles or recreates missing textures, shaders, and
buffers. `release()` remains available when a context is known to be current.

Texture recovery is selected when the texture is created and remains entirely
inside the portable Graphics2D API:

| Policy | CPU recovery storage | Context-loss behavior |
| --- | --- | --- |
| `ReloadFromAsset` | Asset path | Decode and upload the asset again. |
| `RetainPixels` | One RGBA8888 copy | Upload the retained pixels. |
| `Regenerate` | Callback and caller-owned pointer | Ask application code to synchronously provide new pixels. |
| `Discard` | None | Remain invalid; the owner may recreate or remove the resource. |

`load(path)` defaults to `ReloadFromAsset`; `create_rgba(...)` and
`create_solid(...)` default to `RetainPixels`, preserving source compatibility.
Pass `TextureRecoveryOptions` to select another policy. A regeneration
callback receives `TextureRecoveryTarget`, whose `upload_rgba()` consumes the
pixels synchronously. Neither the callback nor public header refers to SDL,
OpenGL, Android, or another backend API. The caller owns `user_data` and must
keep it alive while restoration is possible.

`recovery_policy()` reports the selection and `retained_recovery_bytes()`
reports only the RGBA bytes held for `RetainPixels`. Asset paths, callback user
state, backend allocations, and ordinary object bookkeeping are not included.
Atlas loading accepts a page policy of `ReloadFromAsset`, `RetainPixels`, or
`Discard`; its default remains asset reload. `Regenerate` is intentionally
rejected for an atlas-wide policy because different pages require distinct
recipes.

Atlas restoration preserves page object addresses and region objects, so
existing sprites and GUI drawables do not acquire dangling texture references.
Sprite batches retain their configured capacity and rebuild the built-in
shader and buffers when required.

## Scene and UI boundary

`Actor`, `Group`, `Stage`, actions, widgets, layout, focus, and scene input
routing are Phase 6 APIs. Phase 5 intentionally exposes only the lower-level
graphics foundation they will use.
