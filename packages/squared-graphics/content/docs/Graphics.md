# Squared Graphics

Squared Graphics defines the portable window/rendering-context contract and
the `Color` value shared by higher-level rendering modules. A platform
template selects exactly one implementation package at link time.

## Color

`squared::graphics::Color` stores normalized red, green, blue, and alpha
components. It provides opaque white and transparent-black defaults, converts
eight-bit RGBA values, and clamps unsafe components before they reach a
graphics backend.

## Context

`squared::graphics::Context` owns one backend window and a replaceable
rendering context. It keeps the drawable viewport current, clears the active
color buffer, and presents completed frames. `resume()` makes SDL's restored
context current or creates a replacement when restoration fails.

Every successful context activation advances `generation()`. The
`resources_preserved()` result tells application resource owners whether the
previous context's GPU objects should still exist. Graphics2D resources also
validate their native handles, so a driver-reported preserved context cannot
silently retain stale objects.

`suspend()` explicitly releases the native rendering context while retaining
the window. Android adapters normally let SDL manage EGL suspension instead:
after `SDL_APP_WILLENTERBACKGROUND`, they stop rendering and mark resources
stale without issuing graphics calls, then call `resume()` after
`SDL_APP_DIDENTERFOREGROUND`.

The generated platform adapter owns the Context lifetime. Developer
applications receive it through the Squared Application rendering boundary
instead of creating or presenting platform windows themselves.

## Link-time backend

The package contains no SDL or OpenGL headers, sources, or link requirements.
The SDL2/OpenGL implementation is distributed separately as
`dev.squarednetizen.squared.backend.sdl2-opengl`. Selecting a backend is an
exact SQ dependency decision made by the platform template, not a runtime
registry or virtual interface.

Textures, sprites, atlases, cameras, and batching remain in Squared Graphics2D
and are intentionally outside this low-level context module.
