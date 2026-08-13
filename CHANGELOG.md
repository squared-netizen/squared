# Changelog

## Unreleased

- Advance Squared GUI to `0.6.0-dev.3` with a libGDX-inspired table grid,
  chainable cell constraints, draggable table-backed windows, modal dialogs,
  result actions, escape dismissal, dimming overlays, and focus restoration.
- Add the optional portable `squared-gui` module with retained widgets,
  composable containers, reusable skins, handheld-sized basic controls, focus,
  pointer capture, UTF-8 text editing, direct Application event routing, and a
  Graphics2D painter boundary with no SDL dependency.
- Add a CC0 Kenney UI Pack skin fixture that exercises image-backed button,
  checkbox, and slider drawables through portable texture regions.
- Defer GUI's CMake dependency binding until all composed module targets exist,
  keeping optional-module builds independent of directory discovery order.
- Add the independent optional `squared-holoDisk` module with a single
  `HoloDrive` operational boundary, streamed ZIP reads, scratch-backed writes,
  resource limits, safe cartridge paths, and explicit ZIP materialization.
- Vendor miniz inside HoloDisk so the extension has no dependency on another
  Squared module.
- Split SDL2/OpenGL implementation code from portable Graphics and Graphics2D
  contracts into the new `squared-backend-sdl2-opengl` link-time backend.
- Advance Graphics, Graphics2D, and Scene2D to `0.6.0-dev.2` while preserving
  exact immutable dependency coordinates.
- Add a host test proving that the portable graphics headers and camera build
  without SDL or OpenGL development headers.

- Establish the independent Squared framework repository from the frozen
  Squared Project Generator 0.6.0-dev.6 package snapshot.
- Preserve the existing module package IDs, versions, dependency coordinates,
  source contents, documentation, third-party sources, and licenses.
- Add an independent host build for the existing Application, Data, Graphics,
  Math, Messaging, Scene2D, and Time tests.
