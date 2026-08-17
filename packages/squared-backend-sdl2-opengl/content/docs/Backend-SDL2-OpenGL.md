# Squared SDL2/OpenGL backend

This module supplies the link-time implementation of Squared Graphics and
Graphics2D for SDL2 with OpenGL ES 2. It owns window/context creation, texture
uploads, atlas file loading, shaders, buffers, drawing, and presentation.

Applications select the backend by including this module in their exact SQ
dependency graph. There is no runtime backend registry, backend lookup, or
virtual dispatch in the frame loop. A platform template must link exactly one
graphics backend.

## Android context recovery

The backend can reactivate SDL's retained GL context or create a replacement
for the existing window. Each activation records a new portable context
generation and reports whether native resources were preserved. Graphics2D
textures, atlas pages, programs, and buffers validate preserved OpenGL names
and rebuild invalid ones from retained recipes.

Graphics2D recovery policies are implemented behind this same boundary.
Asset reload, retained-pixel upload, synchronous application regeneration,
and discard all use portable public types; SDL image decoding and OpenGL
uploads remain implementation details in this backend.

Android background handling must not issue GL calls after
`SDL_APP_WILLENTERBACKGROUND`. Applications mark resources stale at that
boundary. Restoration occurs only after `SDL_APP_DIDENTERFOREGROUND`, when SDL
has made rendering available again.

Portable application and framework code should include only headers from
Squared Graphics, Graphics2D, and Scene2D. SDL and OpenGL headers belong in
this backend package and in the platform template.
