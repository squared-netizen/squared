# Changelog

## Unreleased

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
