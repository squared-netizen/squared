# Squared Graphics2D History

## 0.6.0-dev.7

- Dependency alignment only: advanced the exact Graphics and Math dependency
  coordinates to `0.6.0-dev.4` and `0.6.0-dev.2`. No ABI or behavior change.

## 0.6.0-dev.6

- Expanded Doxygen coverage on texture, region, atlas, sprite, batch, and camera headers.
- Documentation-only release: no ABI or behavior change.

## 0.6.0-dev.5

- Added portable `ReloadFromAsset`, `RetainPixels`, `Regenerate`, and `Discard`
  texture recovery policies.
- Added synchronous callback regeneration without retaining procedural RGBA
  pixels or exposing backend APIs.
- Added recovery-policy and retained-byte introspection plus selectable atlas
  page retention.

## 0.6.0-dev.4

- Added no-GL invalidation and context-aware restoration for textures, atlas pages, shaders, and buffers.
- Retained asset paths and RGBA source pixels as texture restoration recipes.
- Preserved texture and atlas-region object addresses across context replacement.

## 0.6.0-dev.3

- Added logical texture subregions, rotated atlas regions, and libGDX nine-patch metadata.
- Added portable atlas and texture-region coverage without SDL/OpenGL headers.

## 0.6.0-dev.2

- Split portable Graphics2D contracts from the SDL2/OpenGL backend.

## 0.6.0-dev.1

- Added orthographic camera, textures, sprites, and sprite batching contracts.
