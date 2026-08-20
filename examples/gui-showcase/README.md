# Squared GUI Showcase

This is a visual Android test application for the complete portable Squared
GUI surface. It starts with three movable, closable, resizable windows and a
persistent launcher beneath them.

The showcase exercises:

- labels, images, panels, separators, and skins;
- buttons, toggle buttons, check boxes, text fields, and sliders;
- tables, linear layouts, stacks, margin containers, and scroll panes;
- multiple floating windows and two modal dialog workflows;
- region and nine-patch drawables loaded from the pinned gdx-holo atlas;
- nested clipping through a SpriteBatch/SDL_ttf painter;
- soft-keyboard text entry and IME composition;
- background/foreground context recovery without rebuilding the widget tree;
- Tab/Shift+Tab traversal, modal focus trapping, arrow-key navigation, and
  Enter/Space/Escape activation;
- semantic D-pad, shoulder-button, A, and B controller navigation through the
  portable Application event boundary.
- hover, long-press, and focus tooltips on launcher controls.

`src/showcase.cpp` is platform-neutral and is covered by the host test suite.
`android/src/application.cpp` implements the generated Android application's
factory and painter boundary. A generated project must enable both
`dev.squarednetizen.squared.gui@0.6.0-dev.15` and
`dev.squarednetizen.squared.backend.sdl2-opengl@0.6.0-dev.5` through
`squared-pg project module add`. The GUI coordinate composes its portable
dependencies and assets; the backend coordinate supplies the link-time
Graphics and Graphics2D implementations. Build the result with
`squared-pg project build`.

The GUI package installs the complete pinned gdx-skins archive and a portable
projection of the selected gdx-holo runtime files. The showcase loads
gdx-holo through the memory-based transactional GUI loader; HoloDisk is not
required for this slice.

## Context-recovery check

Move a window, edit the player name, and open a dialog. Send the application
to the background and return to it several times. The logical UI state must
remain unchanged, texture-backed controls must still render, and the status
line must report the new graphics generation as either `preserved` or
`rebuilt`. Repeat once after an orientation or multi-window transition when
the device permits it.

Cached text uses the portable `Regenerate` recovery policy. Glyph textures are
rerendered only if the context is replaced, avoiding permanent CPU copies of
their RGBA pixels. The atlas uses `ReloadFromAsset`, while the one-pixel white
texture uses the default four-byte `RetainPixels` recipe.
