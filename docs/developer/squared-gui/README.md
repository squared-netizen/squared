# Squared GUI — Developer Guide

Squared GUI is the portable retained-mode widget layer of the framework. It
owns one widget tree per `Ui`, implements measure/layout and painting over the
application-supplied `Painter`, routes pointer and key input through Scene2D
capture-target-bubble dispatch, and imports a supported libGDX skin subset
transactionally. The implementation lives in three translation units plus the
public headers: the widget/layout/input engine (`gui.cpp`), the strict
transactional skin importer (`skin_loader.cpp`), and the atlas-to-drawable
converter (`skin_atlas_resolver.cpp`).

- Programmer counterpart: [Squared GUI — Programmer Guide](../programmer/squared-gui/README.md)
- Package payload: [Gui.md](../../packages/squared-gui/content/docs/Gui.md),
  [Pinned-Gdx-Skins.md](../../packages/squared-gui/content/docs/Pinned-Gdx-Skins.md),
  [Skin-Loading.md](../../packages/squared-gui/content/docs/Skin-Loading.md)
- Documentation index: [Developer documentation](../README.md)
- Diagrams:
  - [GUI package dependencies](GUI-dependencies.dot)
  - [Widget ownership and layout](GUI-widget-ownership.dot)
  - [Input/focus/capture event flow](GUI-input-focus.dot)
  - [Transactional skin loading and rollback](GUI-skin-loading.dot)
  - [Window/dialog modal flow](GUI-modal-flow.dot)
  - Shared framework graph: [Framework package dependencies](../architecture/package-dependencies.dot)

## Dependency boundary

The manifest declares `module.requires` as exactly:

| Module | Version |
| --- | --- |
| `dev.squarednetizen.squared.data` | `0.6.0-dev.2` |
| `dev.squarednetizen.squared.application` | `0.6.0-dev.5` |
| `dev.squarednetizen.squared.scene2d` | `0.6.0-dev.7` |
| `dev.squarednetizen.squared.graphics2d` | `0.6.0-dev.7` |

The CMake target `squared_gui` links exactly those four targets
(`content/modules/squared-gui/CMakeLists.txt` defers the edges to the top-level
directory because generated projects register module targets alphabetically).
The public headers include Application `event.hpp`/`text_input.hpp`, Scene2D
`group.hpp`/`stage.hpp`, and Graphics2D `texture_atlas.hpp`/`texture_region.hpp`
plus `graphics/color.hpp`, which arrives transitively through Graphics2D. No
SDL, Android, OpenGL, or HoloDisk header or link requirement appears anywhere;
`history.md` records that runtime JSON skin loading through a
HoloDisk-backed AssetManager remains future work, and HoloDisk must stay absent
from the GUI dependency graph until that integration exists. The GUI depends on
portable Scene2D input propagation and the portable Application event type; it
never names a backend or another widget hierarchy.

## Architecture

| Component | Source | Responsibility |
| --- | --- | --- |
| `Painter`, `Drawable` + concrete drawables, `Skin`, styles | `src/gui.cpp`, `include/…/gui.hpp` | Drawing boundary, image abstractions, named resource and style tables. |
| `Widget` + all concrete widgets, `Cell`, `Table`, `LinearLayout`, `Stack`, `MarginContainer`, `ScrollPane` | `src/gui.cpp` | Measure/layout, painting, pointer/key/text handling. |
| `Window`, `Dialog` | `src/gui.cpp` | Floating panels, dragging, resizing, modal blocking, focus restoration. |
| `Ui` | `src/gui.cpp` | Facade owning stage, skin, content, windows, captures, and focus. |
| `SkinLoadSeverity/Issue/Limits/Report`, `load_libgdx_skin`, `SkinDrawableResolver` | `src/skin_loader.cpp` | Normalization, strict parsing, validation, staged import, commit. |
| `resolve_atlas_drawable` | `src/skin_atlas_resolver.cpp` | Converts atlas regions to region or nine-patch drawables. |

`Ui` is the facade over the `scene2d::Stage`, the by-value `Skin`, the
`SkinLoader` entry point, pointer capture bookkeeping, and the focus list. The
retained widget component list is: `Label`, `Image`, `Panel`, `Separator`,
`Button`, `ToggleButton`, `CheckBox`, `TextField`, `Slider`, `Table`,
`LinearLayout`, `Stack`, `MarginContainer`, `ScrollPane`, `Window`, and
`Dialog`. Every one derives from `Widget`, which derives from `scene2d::Group`.

## Ownership and threading

- **Widget tree ownership (Composite).** `Group` owns children through
  `std::vector<std::unique_ptr<Actor>>`; `add_actor` adopts a moved
  `unique_ptr`. Containers add their widgets through this mechanism and keep a
  side table of metadata (`Table` holds `Cell`s in a `std::deque`, `Cell`
  stores a raw `Widget*`; `LinearLayout` holds `Slot{Widget*, float grow}` in a
  `std::vector`; `Stack`, `MarginContainer`, `ScrollPane`, and `Window` keep
  raw child pointers alongside `Group` ownership). A failed side-table push
  removes the already-adopted actor and rethrows, so `add()` is exception-safe.
- **`Ui` ownership.** `Ui` owns `stage_` and `skin_` by value; the content
  widget and every shown window are owned by the stage root through
  `add_actor`. `content_` and the `overlays_` vector (`Overlay{Window*
  window; Widget* previous_focus; bool center_pending}`) hold non-owning views
  into that tree. `captures_` is a non-owning `unordered_map<int64_t,
  Widget*>` keyed by framework pointer id.
- **Window lifetimes.** `show_window`/`show_dialog` push an `Overlay`, then
  adopt the window; if adoption throws the overlay is popped. `close_window`
  clears captured pointers inside the window, removes the actor (returning
  ownership to a dropped `unique_ptr` that destroys it), erases the overlay,
  and restores focus. `prune_closed_windows` iterates the overlays from the
  back and closes any window with `close_requested()` set; this runs after
  input dispatch and after `update`, so removal never happens mid-handler.
- **Drawable lifetimes.** `Skin` stores `shared_ptr<const Drawable>`
  (`DrawablePtr`); the drawable objects are shared and immutable. The
  `RegionDrawable`/`NinePatchDrawable` classes hold `const TextureRegion*`
  views, so the backing texture/atlas must outlive every drawable — a hard
  constraint on Android graphics recovery, where textures are recreated and
  restored by the application while the widget tree and skin survive.
- **Threading.** One thread drives the `Ui`. All state — stage, skin, focus,
  captures, overlays, widget `layout_valid_` flags — is unsynchronized and must
  not be mutated concurrently. The single thread model matches the
  application's event loop and graphics context ownership.

## Measure/layout

Widgets report `minimum_size`, `preferred_size`, and `maximum_size`; the
`size_hints` template gathers all three and clamps the preferred size between
the minimum and maximum. `validate_layout` recomputes layout only when
`layout_valid_` is false, and `invalidate_layout` marks the widget and its
nearest `Widget` ancestor stale, so a child text change invalidates the whole
path to the root.

`Table` measurement (`Table::measure`) builds `GridMetrics` (column and row
size vectors) by taking the maximum padded cell extent per column and row,
distributing the deficit of spanning cells back into their columns, and adding
padding and row/column gaps. `Table::layout` then:

1. Measures preferred and minimum metrics.
2. Accumulates per-column/per-row grow weights (a growing cell shares its
   weight across its span).
3. Fits column and row sizes to the table box: extra space is distributed by
   weight; when the box is too small, sizes shrink down toward minimums.
4. Places each cell by its column/row origin plus padding, honoring
   `fill_x`/`grow_x` vs. preferred width, and `align` offset for leftover room.
5. Invalidates and validates each child so nested tables lay out depth-first.

`LinearLayout` sizes along one axis: preferred sizes are summed, shrink is
distributed down to each child's minimum, and surplus space is granted by
`grow` weight up to each child's maximum. `Stack` expands every child to the
stack box. `MarginContainer` insets its single content. `ScrollPane` lays out
content at `(0, -scroll_y)` with the max of pane and content size and clamps
`scroll_y` to `content_height - pane_height`.

`Ui::layout` validates the content tree, then every overlay: a zero-sized
window is given its preferred size, `center_pending` windows are centered over
the stage, windows are constrained to the stage bounds, and the window (and
hence its content table) is validated. Finally, when a `TextField` is focused
and a text-input service is active, the field's stage rectangle is recomputed
and pushed through `update_area`. Layout runs before paint on every frame.

## Input and focus

`Ui::event` translates the framework `application::Event` (pointer down/move/
up, resize, key down/up, text input, IME editing, navigation) into the
portable boundary calls. The dispatch order:

**Pointer** (`Ui::pointer`):

1. Look up `captures_` for the pointer id; a captured widget wins over hit
   testing. Otherwise `widget_at(x, y)` runs `stage_.hit` (top-most actor
   bounds first) and walks ancestors until it finds a `Widget`. If a modal
   window is on top and the hit is not one of its descendants, the modal
   window itself is the target — this is the modal trap.
2. On `down`, focus walks up from the target to the first focusable ancestor
   and assigns focus (skipped when a modal already owns the scope).
3. The event is routed through `stage_.dispatch_input` as an
   `InputEvent` in capture/target/bubble phases; `Widget::input_event` adapts
   Scene2D pointer and key events into `PointerEvent`/`key_down` and calls
   `handle()`/`stop()` when handled.
4. On `down`, the widget that handled the event becomes the capture for that
   pointer id; on `up`/`cancel` the capture is erased.
5. `prune_closed_windows` removes any window closed during dispatch.

**Keys** (`Ui::key_down`): dispatch to the focused widget first; if unhandled,
Tab falls back to `focus_next(shift)`, arrow keys to `focus_direction`, and
Escape to the top overlay's `escape_closes()` (true for `Dialog` by default).
`key_up` dispatches to the focused widget with modifier state.

**Focus.** `focusable_widgets` collects focusable widgets via a depth-first
`collect_focusable` walk, scoped to the top modal window's subtree when one
exists and to the stage root otherwise. `focus_next` traverses that ordered
list with wraparound (Tab order is therefore insertion/actor order within the
modal scope). `focus_direction` scores each eligible candidate by
`primary + 2 * perpendicular` distance and picks the nearest widget in the
requested direction. `set_focus` fires `focus_changed(false)` on the old
widget and `focus_changed(true)` on the new one, refuses widgets outside the
modal scope, and drives the `TextInputService`: `start` when a `TextField`
gains focus (with its stage rectangle), `update_area` on layout changes, and
`stop` when focus leaves the field.

**Modal windows.** `show_window` records the previous focus, clears captures,
and for a modal window clears focus and focuses its first control. Painting
dims the stage behind the top modal using its `WindowStyle::modal_overlay`.
Closing a modal restores the recorded previous focus; overlays record the focus
at their own open time and rewrite `previous_focus` when a window they point
into closes.

## Skin loading transaction

`load_libgdx_skin` (in `skin_loader.cpp`) is `noexcept` and follows a strict
pipeline; failure at any point leaves `destination` untouched:

1. **Byte limit** — `json.size() > maximum_json_bytes` (default 1 MiB) fails.
2. **Normalization** — `normalize_libgdx_json` converts libGDX's relaxed
   dialect (bare unquoted keys, `//` and `/* */` comments, bare tokens) into
   strict JSON, quoting tokens unless they are `true`/`false`/`null`/finite
   numbers; unterminated strings and comments fail.
3. **Strict parse** — Squared Data's owned parser runs with
   `maximum_bytes`, `maximum_depth` (default 64), and
   `reject_duplicate_keys = true`; parse errors are reported with
   line:column.
4. **Importer validation** — the resource-class count, total declared
   resources, and every resource name are bounded (`maximum_resources` default
   4096, `maximum_name_bytes` default 128; class names capped at 240 bytes).
   Any error-severity issue aborts before any resource is loaded.
5. **Staged load** — an `Importer` fills a private `Skin candidate_`, phase by
   phase: `load_colors` (named colors plus the `default`/`grey`/`ui`/`white`
   palette hooks), `validate_fonts` (bitmap-font `file` paths must be relative
   contained asset paths), `load_tinted_colors`, `load_buttons` (button and
   text-button classes), `load_text_fields`, `load_check_boxes`,
   `load_sliders`, `load_windows`, then `report_unsupported` warnings for any
   other libGDX class. Drawables resolve through the caller's resolver
   callback and are cached and counted; an unresolved drawable reference is an
   error. Colors accept inline objects or references to named colors and
   reject non-finite or out-of-range components.
6. **Commit** — if no error-severity issue was recorded, `destination_ =
   std::move(candidate_)` swaps the finished skin into place. Otherwise the
   candidate is dropped and `destination` keeps its prior bytes.

The transaction is therefore ACID-like: the work happens on a copy, the
precondition is a clean report, and the commit is a value move. All JSON and
name processing is byte-oriented and UTF-8-safe; names are bounded in bytes,
and text payloads are copied without re-encoding.

## Data structures and complexity

- `Skin` is `unordered_map<std::string, DrawablePtr>` plus six
  `unordered_map<std::string, Style>` tables — average O(1) lookup and insert,
  worst case O(n) on hash collision. Drawable/style lookups construct a
  temporary `std::string` from the `string_view` key.
- `Ui::captures_` is an `unordered_map<int64_t, Widget*>`: O(1) average
  capture lookup and erase.
- `Ui::overlays_` is a `std::vector<Overlay>`; push is amortized O(1),
  `close_window` is O(n) linear find, and `top_modal` scans from the back in
  O(n) worst case (O(1) when only the top overlay is modal).
- `Table` cells are a `std::deque<Cell>`: amortized O(1) `push_back` and stable
  Cell references across growth. `add` performs a linear scan of existing
  cells to find the next free column (O(cells)). `measure`/`layout` are
  O(cells + columns + rows) with two extra passes over cells for spans and
  grow weights; the `fit` pass is O(columns/rows).
- `LinearLayout` keeps a `std::vector<Slot>`; measured size and layout are
  O(slots).
- Hit testing walks the actor tree from the top-most actor down (`stage_.hit`)
  and then a short ancestor chain to the nearest `Widget`: O(depth + subtree
  visited).
- Focus traversal rebuilds the focusable list with a full tree walk each time
  (O(nodes)); `focus_next` is O(nodes) find, and `focus_direction` is
  O(nodes²) worst case for the pairwise geometric scoring.

## Design patterns

- **Composite** — `Widget` derives from `scene2d::Group`, so any widget
  subtree is a tree of uniform nodes owning their children, and layout/paint
  recurse over it. Alternatives rejected: a flat list of absolutely positioned
  elements (would lose retained hierarchy and reuse) and a facade-only scene
  graph without composition (would duplicate Scene2D). Deviation: GUI nodes
  are a *rich* composite — the same class participates in input, layout, and
  painting, rather than separating structure from strategy.
- **Strategy** — `Painter` is an abstract interface (measure, fill, stroke,
  region, text, clip) implemented by the application (e.g. the showcase's
  SDL2/OpenGL `SpriteGuiPainter`, the tests' `RecordingPainter`). Widgets and
  drawables depend only on the interface. Alternatives rejected: direct
  backend calls in widgets (ties GUI to a backend) and a global function
  table (no place for state such as the text cache). Deviation: the strategy
  is supplied *by the caller* rather than selected by the framework, keeping
  the package free of any backend.
- **Facade** — `Ui` hides the `Stage`, the by-value `Skin`, the skin-loader
  entry point, capture bookkeeping, and focus management behind a compact
  surface (`event`, `pointer`, `key_down`, `layout`, `paint`, `show_window`,
  `show_dialog`). Alternatives rejected: exposing the stage and capture map
  to applications (would leak invariants such as the modal trap).
- **Template Method** — the measure/layout life cycle is fixed by `Widget`
  (`size_hints` gathers the three size hooks; `validate_layout` calls the
  virtual `layout` when stale), and the skin-load pipeline is a fixed sequence
  of `load_*` steps inside `Importer::import`. Alternatives rejected: a
  callback-driven loader (loses deterministic ordering of colors before
  styles).
- **Observer** — change notification is expressed through `std::function`
  callbacks: button click, toggle/check change, slider change, dialog result,
  plus the Scene2D input-listener interface for actors. Alternatives rejected:
  a framework-wide event bus (unneeded global state; callbacks suffice).
- **Factory (simple, data-driven)** — `resolve_atlas_drawable` (and the
  `Skin::add_*_drawable` overloads) select a concrete drawable subclass from
  resource metadata: a region becomes a `RegionDrawable`, and one carrying
  nine-patch splits becomes a `NinePatchDrawable`. This is not the classic
  Factory Method (no virtual creation hook per subclass); the choice is a
  single function branching on data.
- **Transactional commit** — the skin import stages work into a candidate and
  commits with one move, giving a strong all-or-nothing guarantee enforced by
  the `noexcept` boundary and the report's error aggregation.

## Algorithms and execution order

- **Layout pass order**: `Ui::layout` → content `validate_layout` (recursing
  parent-first, children depth-first via each container's own `layout`) →
  each overlay: size-if-zero → center if pending → `constrain_to_parent` →
  window `validate_layout` → text-input area refresh. Painting then walks the
  stage children (content first, then overlays in open order, top-most last),
  pushing a clip per widget, painting it, then its children, then popping.
- **Input dispatch order**: pointer → capture lookup → hit test (with modal
  trap) → focus assignment → Scene2D capture/target/bubble dispatch →
  capture bookkeeping → prune closed windows. Keys → focused widget → Tab →
  arrows → Escape; then prune.
- **Skin-load pipeline order**: byte limit → normalize → strict bounded parse
  → importer limit checks → colors → fonts → tinted drawables → buttons →
  text fields → check boxes → sliders → windows → unsupported-class warnings →
  commit by swap.

## Limitations and technical debt

- **Text**: single-line `TextField` only; no rich text, selection, or
  clipboard. Cursor navigation is byte-offset based on UTF-8 codepoint
  boundaries.
- **Window chrome**: one title bar with a single close glyph; no
  minimize/maximize; resizing works only through border/corner grabs.
- **Focus**: Tab order is insertion/actor order, and directional focus is a
  geometric heuristic (no focus groups or explicit focus order API). `TODO.md`
  tracks list/select-box popups and keyboard-accessible popup behavior.
- **Touch**: pointer capture is per pointer id; there are no pinch, pan
  (beyond `ScrollPane` drag), or multi-touch gesture handlers.
- **Skin model**: one `Skin` per `Ui`, styles fall back to `"default"`, and
  only a subset of libGDX resource classes is supported; unknown classes only
  warn. Font rasterization and typed style inheritance are recorded as the
  next unfinished work in `TODO.md`. Text rendering remains the application's
  responsibility through `Painter`.
- **Atlas resolution**: drawables hold raw region views, so texture lifetimes
  bind to application assets; resolution must be provided by the application
  (the HoloDisk-backed AssetManager is future work per `TODO.md`).
- **Layout**: `ScrollPane` is vertical-only with no rendered scrollbar; every
  layout pass re-measures from scratch (no incremental layout); cell `grow`
  weights are binary rather than proportional shares.
- **Nine-patch**: content insets are clamped to non-negative, and destination
  boxes smaller than the fixed borders scale borders proportionally, so the
  center can shrink to nothing.
- **Modal model**: only the top modal dims and traps; `Overlay` bookkeeping
  keeps focus restoration best-effort across nested window closes.
- **Header/implementation agreement**: the `Skin` style-getter Doxygen claims
  `@throws std::out_of_range` when a name is absent, but the implementation
  falls back to `"default"` and throws only when `"default"` is also missing;
  the header comment is the only mismatch and no code change was made.
