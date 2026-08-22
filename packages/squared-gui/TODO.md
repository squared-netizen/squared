# Squared GUI TODO

Only unfinished work belongs here. Move completed work into `history.md`.

## Next

- [ ] Decompose the monolithic `gui.hpp` and `gui.cpp` implementation into
  cohesive component-focused headers and translation units, following
  `list_view.cpp` as the extraction precedent.
- [ ] Keep `squared/gui/gui.hpp` as a compatibility umbrella while introducing
  focused public headers for core geometry and style contracts, widget
  foundations, layouts, primitive controls, overlays and windows, and composite
  controls.
- [ ] Split implementation by ownership and behavior rather than creating one
  file for every trivial class; avoid cyclic includes, duplicate widget
  hierarchies, and backend or platform leakage.
- [ ] Preserve behavior with component-level tests, run the mandatory Clang
  workflow for each slice, and update CMake and programmer/developer
  documentation as each extraction lands.
- [ ] Continue rounding out the primitive widget set with popup overlays,
  select boxes, and menus.
- [ ] Add a canvas widget using the existing `Widget`, input propagation,
  clipping, and `Painter` contracts for Paint-like drawing.
- [ ] Add a multiline text editor composed from the canvas/text model,
  `ScrollPane`, existing focus/IME routing, selection, and primitive controls;
  do not introduce a separate UI hierarchy.
- [ ] Add a console composed from the text editor/read-only text view,
  `TextField`, `ScrollPane`, `Table`, and buttons with injected command and
  output models; do not create a terminal-specific widget hierarchy.

## Later

- [ ] Connect editor documents and canvas saves to the HoloDisk-backed
  AssetManager after their composite components stabilize.
- [ ] Polish scrolling and disabled-state behavior in the showcase.
