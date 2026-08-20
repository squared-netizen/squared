# Changelog

## GUI HoloDisk file picker 0.6.0-dev.1

- Added an optional composite file-picker package requiring exact GUI dev.15
  and HoloDisk dev.3 coordinates.
- Added transactional HoloDrive navigation, typed AssetManager selection
  loading, and AssetManager-backed gdx-holo skin JSON loading.
- Kept core GUI independent from HoloDisk and platform filesystem APIs.
- Added host tests, package documentation, and dependency diagrams.

## Unreleased

- Advance GUI to `0.6.0-dev.15` with determinate progress bars, lifetime-safe
  button groups and radio behavior, drawable-or-glyph button content, imported
  libGDX progress styles, portable tests, documentation, and showcase coverage.

- Advance GUI to `0.6.0-dev.14` with tooltip factories on every Widget,
  plain-text tooltips composed from existing containers and labels, configurable
  hover/touch/focus timing, movement cancellation, modal scoping, and
  viewport-aware placement through the existing Stage root.

- Advance GUI to `0.6.0-dev.13` and its exact Graphics2D requirement to
  `0.6.0-dev.8`. Add immutable bitmap-font resources, font-aware Painter and
  widget measurement/drawing, imported Label styles, and order-independent
  same-type style inheritance with cycle detection and transactional rollback.

- Advance Graphics2D to `0.6.0-dev.8` with bounded, transactional text
  BMFont resources and UTF-8 glyph layout. The portable result carries page,
  source-rectangle, placement, and advance values ready for GUI page
  resolution and SpriteBatch submission without adding a backend or HoloDisk
  dependency.

- Advance HoloDisk to `0.6.0-dev.3` with a synchronous application-owned,
  typed AssetManager: registered loader strategies, immutable shared handles,
  bounded reads, cache/dependency tracking, cycle detection, transactional
  reload, explicit unload, and manager-owned nested in-memory ZIP mounts.
- Record the next GUI foundation order: Graphics2D bitmap fonts and glyph
  layout, GUI font resources/style inheritance, tooltips, and the remaining
  primitive widgets before File Picker, Canvas, Text Editor, and Console.

- Advance Graphics2D to `0.6.0-dev.7`, Scene2D to `0.6.0-dev.7`, the
  SDL2/OpenGL backend to `0.6.0-dev.5`, Messaging to `0.6.0-dev.3`, and GUI
  to `0.6.0-dev.12` as dependency-alignment-only releases. The exact
  dependency coordinates now reference the published leaf versions
  (Application `0.6.0-dev.5`, Data `0.6.0-dev.2`, Graphics `0.6.0-dev.4`,
  Math `0.6.0-dev.2`, Time `0.6.0-dev.2`, and HoloDisk `0.6.0-dev.3`); no
  ABI or behavior changes.
- Add repository documentation trees `docs/programmer` and `docs/developer`
  with paired package guides, a Doxygen-integrated programmer index, and
  Graphviz diagrams for the framework package graph plus GUI, backend,
  Scene2D, and subsystem flows.
- Publish documentation-only releases with expanded Doxygen coverage and
  unchanged ABI: Application `0.6.0-dev.5`, Data `0.6.0-dev.2`, Graphics
  `0.6.0-dev.4`, Graphics2D `0.6.0-dev.6`, GUI `0.6.0-dev.11`, HoloDisk
  `0.6.0-dev.2`, Math `0.6.0-dev.2`, Messaging `0.6.0-dev.2`, Scene2D
  `0.6.0-dev.6`, and Time `0.6.0-dev.2`.
- Resolve GUI header Doxygen reference warnings by naming `preferred_size`
  painter parameters and documenting the `selected` return value.
- Fix `application-boundary-test` designated-initializer warnings by naming
  the `modifiers` and `text` members.
- Add bounded, transactional libGDX skin loading through Squared Data and
  select gdx-holo for the Android GUI showcase.
- Track File Picker, Canvas, Text Editor, and Console as composite GUI
  components built from the existing widget, layout, focus, input, and painter
  primitives rather than alternative class hierarchies.
- Advance GUI to `0.6.0-dev.10` with an exact Squared Data dependency.
- Add portable modifier-aware keyboard and semantic navigation events without
  exposing SDL or controller constants.
- Add Scene2D capture-target-bubble input propagation and route GUI pointer
  and key delivery through it.
- Add Tab/Shift+Tab traversal, modal trapping, directional focus,
  controller navigation, Enter/Space activation, Escape cancellation,
  keyboard sliders, and focus outlines.
- Add reproducible SHA-256-pinned gdx-skins vendoring into GUI package assets;
  retain the complete archive for the future HoloDisk-backed AssetManager.
- Advance Application to `0.6.0-dev.4`, Scene2D to `0.6.0-dev.5`, and GUI to
  `0.6.0-dev.9` with exact dependencies.
- Add backend-neutral texture recovery policies for asset reload, retained
  pixels, application regeneration, and discard, plus retained-byte
  introspection.
- Move showcase text textures to synchronous callback regeneration so their
  RGBA pixels are not retained between context-loss events.
- Advance Graphics2D to `0.6.0-dev.5`, Scene2D and the SDL2/OpenGL backend to
  `0.6.0-dev.4`, and GUI to `0.6.0-dev.8` with exact dependencies.
- Add Android graphics-context recovery across Application, Graphics,
  Graphics2D, and the SDL2/OpenGL backend. Backgrounding now marks GPU objects
  stale without unsafe GL calls; foregrounding reactivates or replaces the
  context and validates or reconstructs textures, atlas pages, shaders, and
  buffers while preserving GUI state and texture references.
- Advance Application and Graphics to `0.6.0-dev.3`, Graphics2D to
  `0.6.0-dev.4`, the SDL2/OpenGL backend to `0.6.0-dev.3`, and GUI to
  `0.6.0-dev.7` with consistent exact dependency coordinates.
- Add package-owned `history.md` and `TODO.md` documents to all eleven Squared
  packages, seed each from its actual released work and next priorities, and
  add a host test that enforces their presence, current-version history entry,
  active `Next` work, and completed-work handoff rule.
- Advance Squared Application to `0.6.0-dev.2` and GUI to `0.6.0-dev.6`
  with portable key, committed-text, and IME-composition events plus an
  injected soft-keyboard service. Text fields now start and stop platform text
  input from focus, maintain the IME target rectangle, and render pre-edit text
  without coupling GUI to SDL or Android.
- Link the GUI showcase against the selected SDL2/OpenGL implementation target
  so `TextureAtlas`, `Texture`, `SpriteBatch`, and `Context` resolve in
  `libmain.so` before Android starts the application.
- Fix the GUI showcase's SDL OpenGL ES include order by loading the Khronos
  and GLES2 platform definitions before the GLES2 API declarations.
- Add a complete Android GUI showcase application with every current widget,
  three initial floating windows, repeatable window launchers, two modal dialog
  workflows, SDL_ttf text caching, SpriteBatch drawing, and nested scissoring.
- Add an independent host showcase test covering construction, painting,
  multiple-window ownership, modal launch, and safe dismissal.
- Advance Squared GUI to `0.6.0-dev.5` with styled window close controls,
  edge and corner resizing, measured and application minimum-size enforcement,
  viewport constraints, and safe removal after pointer dispatch.
- Add portable logical texture subregions and Squared GUI nine-patch
  drawables, including clockwise atlas rotation, libGDX `split` and `pad`
  metadata binding, proportional undersize behavior, and nine-slice tests.
- Advance Graphics2D and Scene2D to `0.6.0-dev.3`, the SDL2/OpenGL backend to
  `0.6.0-dev.2`, and GUI to `0.6.0-dev.4` with consistent exact dependency
  coordinates.
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
