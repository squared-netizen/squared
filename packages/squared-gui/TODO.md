# Squared GUI TODO

Only unfinished work belongs here. Move completed work into `history.md`.

## Next

- [ ] Continue rounding out the primitive widget set with scroll bars, list
  views with injected selection models, popup overlays, and select boxes.
- [ ] Add a file picker composed from `Window`/`Dialog`, `Table`, `ScrollPane`,
  `TextField`, `Label`, and `Button`; storage enumeration must be injected.
- [ ] Add a canvas widget using the existing `Widget`, input propagation,
  clipping, and `Painter` contracts for Paint-like drawing.
- [ ] Add a multiline text editor composed from the canvas/text model,
  `ScrollPane`, existing focus/IME routing, selection, and primitive controls;
  do not introduce a separate UI hierarchy.
- [ ] Add a console composed from the text editor/read-only text view,
  `TextField`, `ScrollPane`, `Table`, and buttons with injected command and
  output models; do not create a terminal-specific widget hierarchy.

## Later

- [ ] Connect skin archives, file picking, editor documents, and canvas saves
  to a HoloDisk-backed AssetManager after the primitive components stabilize.
- [ ] Polish scrolling and disabled-state behavior in the showcase.
