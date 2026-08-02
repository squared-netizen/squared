# Squared Graphics

Squared Graphics owns the SDL window and OpenGL ES context boundary used by
the current Android profile. It also provides the platform-neutral `Color`
value shared by higher-level rendering modules.

## Color

`squared::graphics::Color` stores normalized red, green, blue, and alpha
components. It provides opaque white and transparent-black defaults, converts
eight-bit RGBA values, and clamps unsafe components before they reach a
graphics backend.

## Context

`squared::graphics::Context` owns one SDL window and its OpenGL ES 2 context.
It creates and destroys both resources together, keeps the drawable viewport
current, clears the active color buffer, and presents completed frames.

The generated platform adapter owns the Context lifetime. Developer
applications receive it through the Squared Application rendering boundary
instead of creating or presenting platform windows themselves.

## Template-provided platform targets

The package contains no SDL or OpenGL implementation. Its CMake target expects
the selected template to provide a validated `SDL2` target, SDL include path,
and OpenGL ES 2 linker library. Missing platform requirements fail during
configuration with an explicit diagnostic.

Textures, sprites, atlases, cameras, and batching remain in Squared Graphics2D
and are intentionally outside this low-level context module.
