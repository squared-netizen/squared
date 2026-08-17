# Squared GUI — Programmer Guide

Squared GUI is the framework's portable retained-mode widget library. It
provides a widget tree with measured layouts, a named drawable and style
`Skin`, pointer/keyboard/focus input routed through Scene2D propagation,
floating draggable and resizable `Window`s, modal `Dialog`s, and a
transactional loader for a supported subset of libGDX skin JSON. All widgets
are C++ objects owned by the widget tree; nothing here touches SDL, OpenGL,
Android, or HoloDisk, and there is no separate second widget hierarchy.

## Package availability

| Module | Version | Requires |
| --- | --- | --- |
| `dev.squarednetizen.squared.gui` | `0.6.0-dev.12` | `dev.squarednetizen.squared.data@0.6.0-dev.2`, `dev.squarednetizen.squared.application@0.6.0-dev.5`, `dev.squarednetizen.squared.scene2d@0.6.0-dev.7`, `dev.squarednetizen.squared.graphics2d@0.6.0-dev.7` |

The CMake target is `squared_gui`, a static library exporting the `include/`
directory and the C++20 requirement. It links Application, Data, Scene2D, and
Graphics2D. The `graphics::Color` value used by the API arrives transitively
through Graphics2D; the GUI package never depends on a rendering backend or on
HoloDisk.

## Public API overview

All types live in namespace `squared::gui` and in the two headers listed.

| Type | Header | Purpose |
| --- | --- | --- |
| `Size`, `Rectangle`, `Insets` | `squared/gui/gui.hpp` | Geometry values in logical units. |
| `SizeHints` | `squared/gui/gui.hpp` | Minimum, preferred, and maximum size of one widget. |
| `Direction`, `Alignment` | `squared/gui/gui.hpp` | Layout axis and per-cell alignment enums. |
| `Painter` | `squared/gui/gui.hpp` | Application-implemented drawing boundary used for measure and paint. |
| `Drawable`, `DrawablePtr` | `squared/gui/gui.hpp` | Immutable image abstraction; `shared_ptr<const Drawable>`. |
| `ColorDrawable`, `RegionDrawable`, `NinePatchDrawable`, `NinePatchSplits` | `squared/gui/gui.hpp` | Concrete drawables: flat color, one texture region, scalable nine-piece region. |
| `Skin` | `squared/gui/gui.hpp` | Named drawable table plus named styles for panel, button, text field, check box, slider, window. |
| `PanelStyle`, `ButtonStyle`, `TextFieldStyle`, `CheckBoxStyle`, `SliderStyle`, `WindowStyle` | `squared/gui/gui.hpp` | Style value types referencing drawables and colors. |
| `PointerAction`, `PointerEvent`, `Key`, `KeyModifiers` | `squared/gui/gui.hpp` | Portable input payloads at the GUI boundary. |
| `Widget` | `squared/gui/gui.hpp` | Base class of every GUI node; derives from `scene2d::Group`. |
| `Label`, `Image`, `Panel`, `Separator` | `squared/gui/gui.hpp` | Content and structure widgets. |
| `Button`, `ToggleButton`, `CheckBox` | `squared/gui/gui.hpp` | Clickable and selectable controls. |
| `TextField` | `squared/gui/gui.hpp` | Single-line UTF-8 text entry with cursor and composition. |
| `Slider` | `squared/gui/gui.hpp` | Draggable one-axis value selector. |
| `Cell`, `Table` | `squared/gui/gui.hpp` | Grid layout with chainable per-cell constraints. |
| `LinearLayout`, `Stack`, `MarginContainer`, `ScrollPane` | `squared/gui/gui.hpp` | Additional compositional containers. |
| `Window`, `Dialog` | `squared/gui/gui.hpp` | Floating table-backed panels; `Dialog` adds modal blocking and results. |
| `Ui` | `squared/gui/gui.hpp` | Owns one widget tree, the stage, the skin, and all open windows. |
| `SkinLoadSeverity`, `SkinLoadIssue`, `SkinLoadLimits`, `SkinLoadReport` | `squared/gui/skin_loader.hpp` | Diagnostics, limits, and counts for transactional skin loading. |
| `SkinDrawableResolver`, `load_libgdx_skin`, `resolve_atlas_drawable` | `squared/gui/skin_loader.hpp` | Portable libGDX skin import entry points. |

## Creating a Ui over an Application

`Ui` is constructed with a virtual size in logical pixels and a `Skin` (the
default skin is used when none is supplied). All layout and painting happens
through a `Painter` that your application implements over its graphics
stack. The application's event and lifecycle callbacks forward into the `Ui`.

```cpp
#include <squared/gui/gui.hpp>
#include <squared/gui/skin_loader.hpp>
#include <squared/application/application.hpp>

// Application-owned painter translating gui::Painter into frame drawing.
struct GuiPainter final : squared::gui::Painter {
    squared::gui::Size measure_text(std::string_view) override { return {8.0F, 16.0F}; }
    void fill_rectangle(const squared::gui::Rectangle&, squared::graphics::Color) override {}
    void stroke_rectangle(const squared::gui::Rectangle&, squared::graphics::Color, float) override {}
    void draw_region(const squared::graphics2d::TextureRegion&, const squared::gui::Rectangle&, squared::graphics::Color) override {}
    void draw_text(std::string_view, float, float, squared::graphics::Color) override {}
    void push_clip(const squared::gui::Rectangle&) override {}
    void pop_clip() override {}
};

class GuiApplication final : public squared::application::Application {
public:
    GuiApplication()
        : ui_(960.0F, 540.0F)
    {
    }

    bool create(squared::graphics::Context&) override { return true; }
    void surface_created(squared::graphics::Context&) override {}

    void handle_event(const squared::application::Event& event) override
    {
        ui_.event(event); // pointer, resize, key, text, IME, navigation
    }

    void update(std::chrono::nanoseconds delta) override
    {
        ui_.update(std::chrono::duration<double>(delta).count());
    }

    void render(squared::graphics::Context&) override
    {
        ui_.layout(painter_);
        ui_.paint(painter_);
    }

    void set_text_input_service(squared::application::TextInputService* service) noexcept override
    {
        ui_.set_text_input_service(service);
    }

    void resize(int width, int height) override { /* painter DPI setup */ }
    void surface_destroyed() override {}
    void dispose() override {}
    [[nodiscard]] bool quit_requested() const noexcept override { return false; }

private:
    GuiPainter painter_;
    squared::gui::Ui ui_;
};
```

- `Ui(width, height, skin)` stores the skin by value; pass a moved `Skin` to
  avoid a copy.
- `event()` accepts the framework `application::Event` directly and returns
  `true` when the widget tree consumed it. Pointer, key, committed text,
  composition, navigation, and resize events all enter through this one call,
  or through the narrower `pointer()`, `key_down()`, `key_up()`,
  `navigation()`, `text_input()`, and `text_editing()` entry points.
- `update(delta_seconds)` advances time-based state and removes
  close-requested windows. `layout(painter)` validates layout of every visible
  widget; `paint(painter)` draws the content and all overlays with per-widget
  clipping. Call both once per frame.
- `set_text_input_service(service)` installs the platform-neutral soft-keyboard
  boundary. The pointer is non-owning and must outlive the `Ui`.

## Loading a skin transactionally

`load_libgdx_skin` imports a supported subset of the libGDX skin JSON dialect
from memory into a `Skin`. The input is treated as untrusted: it is normalized
under explicit limits, parsed with Squared Data's strict parser, and validated
resource by resource. The destination skin is replaced only when the entire
supported subset succeeds, so a rejected document leaves it byte-for-byte
untouched.

```cpp
#include <squared/gui/skin_loader.hpp>
#include <iostream>

squared::gui::Skin load_theme(
    std::string_view json,
    const squared::graphics2d::TextureAtlas& atlas
)
{
    squared::gui::Skin skin;
    squared::gui::SkinLoadReport report;
    const bool loaded = squared::gui::load_libgdx_skin(
        skin,
        json,
        [&atlas](std::string_view name) {
            return squared::gui::resolve_atlas_drawable(atlas, name);
        },
        report
    );
    if (!loaded) {
        for (const squared::gui::SkinLoadIssue& issue : report.issues) {
            if (issue.severity == squared::gui::SkinLoadSeverity::error) {
                std::cerr << "skin " << issue.path << ": " << issue.message << '\n';
            }
        }
        return squared::gui::Skin{}; // programmatic fallback
    }
    return skin;
}
```

- The resolver callback is invoked for every referenced drawable name.
  `resolve_atlas_drawable(atlas, name)` is the ready-made helper: it produces a
  `RegionDrawable`, or a `NinePatchDrawable` when the atlas region carries
  libGDX `split`/`pad` metadata.
- On success `report` counts `colors_loaded`, `drawables_loaded`, and
  `styles_loaded`; `report.success()` is `true` when no error-severity issue
  was recorded.
- The default `SkinLoadLimits` bound the document to 1 MiB of JSON, 64 levels
  of nesting, 4096 loaded resources, and 128 bytes per resource name. Override
  the limits by passing a modified copy:

```cpp
squared::gui::SkinLoadLimits limits;
limits.maximum_json_bytes = 512U * 1024U;
limits.maximum_depth = 32;
limits.maximum_resources = 1024;
limits.maximum_name_bytes = 64;
squared::gui::SkinLoadReport report;
// ... load_libgdx_skin(skin, json, resolver, report, limits);
```

## Skins and drawables

A `Skin` is a named resource table. Drawables are immutable and shared through
`DrawablePtr` (`std::shared_ptr<const Drawable>`); style values are plain
structs that reference drawables.

```cpp
squared::gui::Skin skin;
skin.add_region_drawable(
    "button.normal",
    atlas.find_region("button.normal")->region(),
    {12.0F, 8.0F, 12.0F, 8.0F}
);
skin.add_nine_patch_drawable("window.background", *atlas.find_region("window.background"));

squared::gui::ButtonStyle action = skin.button_style("default");
action.normal = skin.drawable("button.normal");
skin.add_button_style("action", std::move(action));
```

- `add_region_drawable`, `add_nine_patch_drawable`, and the style setters
  register or replace a named resource. `drawable(name)` returns an empty
  `DrawablePtr` when the name is absent.
- The atlas overload of `add_nine_patch_drawable` reads libGDX `split` and
  optional `pad` metadata directly; the explicit overload takes
  `NinePatchSplits` when the metadata comes from another source.
- Style getters (`button_style`, `text_field_style`, ...) fall back to the
  `"default"` style of that kind when the requested name is absent; a missing
  `"default"` entry raises `std::out_of_range`. The default constructor of
  `Skin` installs a programmatic fallback skin: flat `ColorDrawable`s plus
  `"default"` styles for every widget kind, with 44 logical-unit touch targets.
- The referenced `TextureRegion`s must outlive the drawables (see Ownership
  and lifetime below).

## Building a window with a table

`Table` adds children as grid cells and returns a chainable `Cell` that
controls alignment, fill, growth, spans, and padding. Ownership transfers to
the table on `add()`, so pass `std::unique_ptr<Widget>`. The tree — table,
window, and everything inside — is destroyed with the widget that owns it.

```cpp
squared::gui::Ui ui(960.0F, 540.0F);

auto window = std::make_unique<squared::gui::Window>("Player");
window->set_closable(true);
window->set_resizable(true);
window->set_minimum_window_size({280.0F, 160.0F});

squared::gui::Table& table = window->content_table();
table.add(std::make_unique<squared::gui::Label>("Name"))
    .align(squared::gui::Alignment::end);
table.add(std::make_unique<squared::gui::TextField>()).grow_x().fill_x();
table.row();
table.add(std::make_unique<squared::gui::Separator>())
    .column_span(2).grow_x().fill_x();
table.row();
table.add(std::make_unique<squared::gui::Button>("Save", save_and_close))
    .grow_x().fill_x();

squared::gui::Window& shown = ui.show_window(std::move(window));
```

- `Table::add` adopts the child and returns a reference to its `Cell`. The
  `Cell` reference remains valid until the next `add()`/`row()` mutation of the
  table; do not retain it.
- `row()` advances to the next grid row. `column_span(n)` covers `n` columns.
  `grow()`/`grow_x()`/`grow_y()` distribute extra space among growing cells;
  `fill()`/`fill_x()`/`fill_y()` expand the child inside its cell.
  `pad(value)` or `pad(Insets)` sets cell padding; `align(horizontal, vertical)`
  places the child when space remains.
- `Button(text, callback)` copies the callback into the button;
  `set_on_click(callback)` replaces it later. `ToggleButton` and `CheckBox`
  report new states through `set_on_change(std::function<void(bool)>`;
  `Slider` through `set_on_change(std::function<void(float)>`.
- `show_window` transfers ownership of the window to the `Ui`; the returned
  reference is stable until the window closes.

## Focus and keyboard navigation

One widget holds keyboard focus at a time; `ui.focused()` reads it. A pointer
press focuses the deepest focusable ancestor of the pressed widget, Tab and
Shift+Tab traverse the focusable widgets in deterministic actor order and wrap
at the ends, and the arrow keys move focus to the geometrically nearest
eligible widget when the focused widget does not consume them. A modal window
scopes the focus list to its own descendants.

```cpp
ui.key_down(squared::gui::Key::tab);                    // next focusable
ui.key_down(squared::gui::Key::tab, {.shift = true});   // previous focusable
ui.key_down(squared::gui::Key::up);                     // nearest above
ui.key_down(squared::gui::Key::enter);                  // activate focused button
ui.key_down(squared::gui::Key::escape);                 // dismiss cancellable dialog
```

- Buttons, check boxes, and toggle buttons activate on Enter or Space; a
  focused slider adjusts with Left and Right; Escape closes dialogs that have
  `cancel_on_escape` enabled. Focusable controls paint a visible accent
  outline while focused.
- `ui.clear_focus()` clears focus to no widget. When focus is on a
  `TextField`, focusing drives the soft keyboard through the installed
  `TextInputService`: `start` on focus, `update_area` when layout moves the
  field, `stop` when focus leaves.
- Semantic controller navigation arrives through `ui.navigation(action, id)`
  and maps to the same directional, next/previous, activate, and cancel
  behavior without exposing a controller API in the GUI.

## Pointer input coordinates and coordinate spaces

The GUI input boundary works in stage coordinates: the `Ui::pointer` and
`application::Event` pointer payloads use logical pixels measured from the
top-left of the virtual viewport. Inside a widget, `PointerEvent::x` and `y`
are in that widget's local coordinates (offset from its top-left corner).
Scene2D converts stage coordinates to actor-local coordinates during dispatch,
so a child of a window always sees positions relative to itself.

```cpp
// Stage coordinates: (120, 80) in the 960x540 virtual viewport.
ui.pointer(squared::gui::PointerAction::down, 120.0F, 80.0F, 0, pointer_id);
ui.pointer(squared::gui::PointerAction::up, 120.0F, 80.0F, 0, pointer_id);
```

- `pointer(action, x, y, button, pointer_id)` reports a pointer contact by
  stable `pointer_id`. On `down`, the hit widget captures that pointer; all
  subsequent moves for that id route to it even when the pointer leaves its
  bounds, and capture is released on `up` or `cancel`. One `pointer_id` per
  contact, independent of button index.
- `PointerEvent` carries `action`, `pointer_id`, local `x`/`y`, and `button`
  (0 is the primary button). Widgets that need stage positions can accumulate
  the actor parent chain, as the showcase does for dialog action buttons.
- Widget-local coordinates make `Button::pointer_event` hit-test against the
  button's own bounds and make `Slider::pointer_event` scale its drag by the
  slider's own width.

## Dialogs and modality

`Dialog` is a modal `Window` with a content table, an action-button table, and
a result callback. It dims the rest of the stage, traps pointer input outside
its bounds, focuses its first control when shown, and restores the previously
focused widget when dismissed.

```cpp
auto dialog = std::make_unique<squared::gui::Dialog>(
    "Quit game",
    [](std::string_view result) {
        if (result == "quit") request_quit();
    }
);
dialog->text("Return to the title screen?")
    .button("Cancel", "cancel")
    .button("Quit", "quit");
ui.show_dialog(std::move(dialog));
```

- `text()` appends a message paragraph; `button(label, result)` appends an
  action button that reports `result` through the dialog's result callback
  when chosen. `set_on_result` replaces the callback later.
- A dialog with zero size is packed and centered over the viewport during the
  next `Ui::layout`. Escape dismisses it by default (a `"cancel"` result);
  `set_cancel_on_escape(false)` disables that.
- The close control and window resizing behave exactly as for a `Window`
  (below). `Dialog` always starts modal.

## Window dragging and resizing

`Window` is a floating, table-backed panel hosted by the `Ui`. The following
behavior is publicly configurable:

- `set_movable(bool)` — dragging by the title bar (on by default). Dragging is
  clamped to the UI bounds.
- `set_closable(bool)` — shows a styled close button in the title bar. Pressing
  it marks the window close-requested; the `Ui` removes it safely after event
  dispatch.
- `set_resizable(bool)` — enables resize grabs on the enabled border edges and
  corners. Resizing never crosses the window's measured or requested minimum
  and is clamped to the viewport.
- `set_minimum_window_size(Size)` — application-requested minimum; the window
  also honors its measured content minimum.
- `request_close()` — asks the `Ui` to remove the window on the next update.

```cpp
squared::gui::Window& window = ui.show_window(std::make_unique<squared::gui::Window>("Settings"));
window.set_movable(true);
window.set_closable(true);
window.set_resizable(true);
window.set_minimum_window_size({320.0F, 240.0F});
```

## Errors

- **Skin loading** returns `false` on any failure and fills `report.issues`
  with `SkinLoadIssue` entries `{ severity, path, message }`. `path` is a
  JSON-pointer-style path such as
  `com.badlogic.gdx.scenes.scene2d.ui.TextButton$TextButtonStyle.default.up` or
  `$` for document-level errors. Warnings are recorded for unsupported libGDX
  resource classes without failing the load; any error-severity issue aborts
  the transaction. `load_libgdx_skin` is `noexcept`; internal failures are
  caught and reported as a `$` issue.
- **Widget tree construction** reports misuse with exceptions: passing a null
  child to `add`/`set_content`/`show_window`/`show_dialog` throws
  `std::invalid_argument`, as does registering an empty-named or null drawable
  on a `Skin` or constructing a `NinePatchDrawable` whose splits exceed the
  source region. Style lookups raise `std::out_of_range` only when the
  `"default"` style of that kind is missing.
- **Input entry points** (`event`, `pointer`, `key_down`, `key_up`,
  `navigation`, `text_input`, `text_editing`) return `true` when the widget
  tree handled the event. There is no exception channel across the input
  boundary; unexpected failures surface as logged errors in the application
  layer.

## Ownership and lifetime

- **Widgets own widgets.** Every container (`Table`, `LinearLayout`, `Stack`,
  `MarginContainer`, `ScrollPane`, `Window`, `Dialog`) adopts
  `std::unique_ptr<Widget>` children and releases them on destruction.
  `Window` owns its content table; `Dialog` adds its message table and button
  table to that content table. References returned by `add`, `set_content`,
  `content_table`, and `button_table` are valid for the child's lifetime.
- **`Ui` owns the stage and the skin.** It adopts the content widget and every
  shown window. A window is removed when it is closed; the `Ui` then drops its
  ownership and the window and its contents are destroyed.
- **Drawables are shared and immutable.** `DrawablePtr` gives shared ownership
  of the drawable object, but a `RegionDrawable`/`NinePatchDrawable` stores a
  raw view of a `TextureRegion`, so the owning `Texture`/`TextureAtlas` must
  outlive every use of the drawable. This matters on Android context loss: the
  application rebuilds or restores its textures before drawing again.
- **Focus and listener references must not outlive their widgets.** Callbacks
  captured into buttons, toggles, sliders, and dialogs must not reference
  widgets that can be closed and destroyed while the callback is installed.
- **The `TextInputService` pointer is non-owning** and must outlive the `Ui`
  while set.

## Threading

A `Ui` is not thread-safe. Create it, feed it events, and run `update`,
`layout`, and `paint` on the application's main thread — the thread that owns
the event loop and the graphics context. `load_libgdx_skin` requires the caller
to own both the destination `Skin` and the resolver; concurrent mutation of a
`Skin` is likewise unsynchronized.

## Lua bindings

None. No type or function in `squared::gui` has a Lua 5.4 binding, and the
package headers contain no Lua interface.

## Related documentation

- Package payload: [Gui.md](../../../packages/squared-gui/content/docs/Gui.md),
  [Pinned-Gdx-Skins.md](../../../packages/squared-gui/content/docs/Pinned-Gdx-Skins.md),
  [Skin-Loading.md](../../../packages/squared-gui/content/docs/Skin-Loading.md)
- Implementation details: [Squared GUI — Developer Guide](../developer/squared-gui/README.md)
- Underlying contracts: [Scene2D Programmer Guide](../squared-scene2d/README.md)
- Documentation index: [Programmer documentation](../README.md)
