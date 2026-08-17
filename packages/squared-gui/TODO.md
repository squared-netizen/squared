# Squared GUI TODO

Only unfinished work belongs here. Move completed work into `history.md`.

## Next

- [ ] Define font resources and typed style inheritance for imported skins.
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
- [ ] Add list/select-box widgets and keyboard-accessible popup behavior.
- [ ] Polish scrolling and disabled-state behavior in the showcase.
