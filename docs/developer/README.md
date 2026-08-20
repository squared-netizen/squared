# Squared Developer Documentation

Implementation-oriented documentation for maintainers and integrators of the
Squared framework. Each page documents architecture and dependency
boundaries, ownership and threading models, invariants and failure behavior,
data structures with complexity, algorithms and execution order, the design
patterns actually used (why they fit, alternatives rejected, deviations), and
known limitations and technical debt.

Graphical overviews are stored as editable Graphviz sources (`*.dot`) beside
these pages and rendered beneath `build/docs/`. Validate changes with
`dot -Tsvg`.

## Package pages

| Package | Developer page | Counterpart programmer page |
| --- | --- | --- |
| application | [squared-application/](squared-application/README.md) | [programmer](../programmer/squared-application/README.md) |
| backend-sdl2-opengl | [squared-backend-sdl2-opengl/](squared-backend-sdl2-opengl/README.md) | [programmer](../programmer/squared-backend-sdl2-opengl/README.md) |
| data | [squared-data/](squared-data/README.md) | [programmer](../programmer/squared-data/README.md) |
| graphics | [squared-graphics/](squared-graphics/README.md) | [programmer](../programmer/squared-graphics/README.md) |
| graphics2d | [squared-graphics2d/](squared-graphics2d/README.md) | [programmer](../programmer/squared-graphics2d/README.md) |
| gui | [squared-gui/](squared-gui/README.md) | [programmer](../programmer/squared-gui/README.md) |
| holoDisk | [squared-holoDisk/](squared-holoDisk/README.md) | [programmer](../programmer/squared-holoDisk/README.md) |
| math | [squared-math/](squared-math/README.md) | [programmer](../programmer/squared-math/README.md) |
| messaging | [squared-messaging/](squared-messaging/README.md) | [programmer](../programmer/squared-messaging/README.md) |
| scene2d | [squared-scene2d/](squared-scene2d/README.md) | [programmer](../programmer/squared-scene2d/README.md) |
| time | [squared-time/](squared-time/README.md) | [programmer](../programmer/squared-time/README.md) |

## Architecture diagrams

- [Framework package dependencies](architecture/package-dependencies.dot)
- [GUI implementation dependencies](squared-gui/GUI-dependencies.dot)
- [GUI input/focus event flow](squared-gui/GUI-input-focus.dot)
- [Skin-loading transaction](squared-gui/GUI-skin-loading.dot)
- [HoloDisk typed asset loading](squared-holoDisk/asset-loading-flow.dot)
- [Graphics2D bitmap-font layout](squared-graphics2d/bitmap-font-layout-flow.dot)

## Package payloads

Package documentation shipped inside generated projects lives in
`packages/*/content/docs/` and is preserved as distributed content; the pages
above synchronize with it without duplicating implementation detail.

## Not covered here

End-user usage guidance: see the counterpart programmer pages, or the Doxygen
generation for the authoritative symbol-level reference.
