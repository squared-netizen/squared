# Squared GUI TODO

Only unfinished work belongs here. Move completed work into `history.md`.

## Next

- [ ] Continue rounding out the primitive widget set with scroll bars, list
  views with injected selection models, popup overlays, and select boxes.
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
