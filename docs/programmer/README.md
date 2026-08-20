# Squared Programmer Documentation

Guides for application and game programmers consuming the Squared framework.
Each page documents a package's public API: purpose, parameters and units,
return values, ownership and lifetime, preconditions and postconditions,
error results, threading expectations, a minimal C++20 example, and the Lua
5.4 equivalent when a binding exists.

Public headers carry full Doxygen comments; this tree mirroring them is the
human-readable companion. Where a topic is about *how* Squared works instead
of *how to use* it, see the counterpart developer page.

## Package pages

| Package | Programmer page | Counterpart developer page |
| --- | --- | --- |
| application | [squared-application/](squared-application/README.md) | [developer](../developer/squared-application/README.md) |
| backend-sdl2-opengl | [squared-backend-sdl2-opengl/](squared-backend-sdl2-opengl/README.md) | [developer](../developer/squared-backend-sdl2-opengl/README.md) |
| data | [squared-data/](squared-data/README.md) | [developer](../developer/squared-data/README.md) |
| graphics | [squared-graphics/](squared-graphics/README.md) | [developer](../developer/squared-graphics/README.md) |
| graphics2d | [squared-graphics2d/](squared-graphics2d/README.md) | [developer](../developer/squared-graphics2d/README.md) |
| gui | [squared-gui/](squared-gui/README.md) | [developer](../developer/squared-gui/README.md) |
| gui-file-picker | [squared-gui-file-picker/](squared-gui-file-picker/README.md) | [developer](../developer/squared-gui-file-picker/README.md) |
| holoDisk | [squared-holoDisk/](squared-holoDisk/README.md) | [developer](../developer/squared-holoDisk/README.md) |
| math | [squared-math/](squared-math/README.md) | [developer](../developer/squared-math/README.md) |
| messaging | [squared-messaging/](squared-messaging/README.md) | [developer](../developer/squared-messaging/README.md) |
| scene2d | [squared-scene2d/](squared-scene2d/README.md) | [developer](../developer/squared-scene2d/README.md) |
| time | [squared-time/](squared-time/README.md) | [developer](../developer/squared-time/README.md) |

Package payload documentation distributed inside generated projects lives in
`packages/*/content/docs/` and is cross-linked from each page above.

## Not covered here

- Internal implementation, structures, and algorithms: see the counterpart
  developer pages.
- Building the framework, package lifecycle, and repository conventions:
  see the repository `README.md` and `AGENTS.md`.
