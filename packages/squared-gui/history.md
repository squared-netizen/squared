# Squared GUI History

## 0.6.0-dev.15

- Added determinate progress bars with named programmatic and imported libGDX
  styles, clamped range values, normalized completion, and read-only input.
- Added non-owning, mutually detaching button groups with configurable checked
  limits plus radio-button semantics built on the existing toggle controls.
- Added drawable-or-UTF-8-glyph content to Button and its derived controls,
  retaining the existing font, focus, input, and callback paths.
- Expanded portable tests, paired documentation, and the complete showcase
  without adding dependencies or another widget hierarchy.

## 0.6.0-dev.14

- Added plain-text and custom-factory tooltips composed from ordinary Widget
  subtrees and temporarily owned by the existing Stage root.
- Added configurable hover, primary-contact long-press, and keyboard/controller
  focus delays, pointer-movement cancellation, modal scoping, and viewport-aware
  below/above placement.
- Added tooltip coverage to the portable GUI tests and launcher controls in the
  complete showcase without adding dependencies or another overlay hierarchy.

## 0.6.0-dev.13

- Added immutable descriptor-only or resolved bitmap-font resources with
  Graphics2D metrics, page regions, glyph-region derivation, and font-aware
  Painter measurement/drawing fallbacks.
- Added named fonts and label styles to Skin; text-bearing imported styles now
  retain typed font handles through measurement and painting.
- Added order-independent same-type style inheritance through `parent` or
  `extends`, with bounded parent names, missing-parent diagnostics, cycle
  detection, and transactional rollback.
- Advanced the exact Graphics2D dependency to `0.6.0-dev.8`.

## 0.6.0-dev.12

- Dependency alignment only: advanced the exact Application, Data, Scene2D,
  and Graphics2D dependency coordinates to `0.6.0-dev.5`, `0.6.0-dev.2`,
  `0.6.0-dev.7`, and `0.6.0-dev.7`. No ABI or behavior change.

## 0.6.0-dev.11

- Expanded Doxygen coverage across widget, skin, loader, window, and dialog headers.
- Named `preferred_size` painter parameters and documented the `selected` return value to resolve reference warnings.
- Documentation-only release: no ABI or behavior change.

## 0.6.0-dev.10

- Added bounded normalization and strict parsing for libGDX's relaxed skin
  JSON dialect through Squared Data.
- Added transactional loading for Squared button, text-field, check-box,
  slider, and window styles with portable drawable resolution.
- Selected `gdx-holo` as the showcase theme while preserving the programmatic
  fallback skin and reporting unsupported libGDX resource classes.

## 0.6.0-dev.9

- Routed widget pointer, key-down, and key-up delivery through Scene2D
  capture-target-bubble propagation.
- Added Tab/Shift+Tab traversal, modal focus trapping, directional focus,
  semantic controller navigation, and focus restoration.
- Added Enter and Space button activation, Escape cancellation, keyboard
  sliders, and visible focus outlines.
- Added repeatable SHA-256-pinned gdx-skins archive vendoring into
  generated-project assets while leaving runtime JSON skin loading through a
  HoloDisk-backed AssetManager as explicit future work.

## 0.6.0-dev.8

- Advanced the exact Graphics2D dependency for selectable texture recovery.
- Changed showcase text-cache recovery to regenerate glyph textures instead
  of retaining a second CPU-side RGBA copy.

## 0.6.0-dev.7

- Advanced exact Application and Graphics2D dependencies for Android context recovery.
- Preserved widget trees and texture-backed skin references across graphics restoration.

## 0.6.0-dev.6

- Routed portable keyboard, committed-text, and IME-composition events.
- Added focus-driven soft-keyboard start, target-area updates, and stop behavior.
- Added separate pre-edit composition rendering in `TextField`.

## 0.6.0-dev.5

- Added styled close controls, edge/corner resizing, minimum sizes, and viewport constraints.
- Added the complete multi-window Android GUI showcase and host showcase test.

## 0.6.0-dev.4

- Added nine-patch drawables and libGDX atlas split/padding integration.

## 0.6.0-dev.3

- Added table/grid layout, draggable windows, modal dialogs, focus restoration, and dimming.

## 0.6.0-dev.2

- Added the first retained controls, containers, skins, focus, pointer capture, and UTF-8 editing.
