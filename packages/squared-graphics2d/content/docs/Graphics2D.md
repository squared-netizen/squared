---
title: Squared Graphics2D
tags:
  - cpp
  - graphics
  - opengl-es
---

# Graphics2D

Applications render through the `squared::` C++ framework over
an OpenGL ES 2 context. SDL2 owns the portable window and lifecycle boundary;
ordinary application drawing does not call Android APIs or raw OpenGL ES.
The Termux build compiles against the Khronos declarations bundled in the SDL2
kit and links Android's system GLES2 library. That system library is not copied
into the APK.

## Minimum API

- The independent Squared Graphics module provides
  `squared::graphics::Context`, which owns the window, context, viewport,
  clear, and presentation operations.
- `squared::graphics2d::Texture` owns a GPU texture.
- `squared::graphics2d::TextureRegion` references a rectangle within a texture.
- `squared::graphics2d::TextureAtlas` owns one or more page textures and
  exposes named regions.
- `squared::graphics2d::Sprite` stores a region, transform, and color.
- `squared::graphics2d::SpriteBatch` batches ordered textured quads.
- `squared::graphics2d::OrthographicCamera` maps logical 2D coordinates to the
  framebuffer.

The default camera uses a top-left origin: positive X points right and positive
Y points down. Texture regions also use top-left image coordinates.

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

## Current lifecycle limit

GPU objects, including atlas page textures, must be created and destroyed
while the OpenGL ES context is current. Automatic restoration after Android
context loss is deferred until the asset manager is introduced. The generated
host refreshes the viewport on resize and foreground events.

## Scene and UI boundary

`Actor`, `Group`, `Stage`, actions, widgets, layout, focus, and scene input
routing are Phase 6 APIs. Phase 5 intentionally exposes only the lower-level
graphics foundation they will use.
