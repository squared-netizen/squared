# Squared GUI

Squared GUI is an optional portable retained-mode interface layer. It uses
Scene2D for ownership and hierarchy, consumes the framework's existing
`application::Event` values, and draws through Graphics and Graphics2D. It has
no dependency on SDL, OpenGL, Android, HoloDisk, or a particular font system.

## Small, composable boundary

`squared::gui::Ui` owns one widget tree. Its public input boundary accepts
Application pointer, resize, key, committed-text, and IME-composition events
directly. GUI converts pointer and key input at that boundary and dispatches it
through Scene2D capture, target, and bubble phases. Pointer capture remains
tracked by the framework pointer ID; GUI does not create another general event
bus or expose platform constants.

Keyboard press and release events carry portable modifier state through the
focused actor path. Tab and Shift+Tab traverse focus in deterministic actor
order and wrap at the ends. A modal dialog scopes
that list to its own descendants, focuses its first control when shown, and
restores the prior focus when closed. Arrow keys select the nearest eligible
control geometrically when the focused widget does not consume them. Semantic
navigation provides the same directional, next/previous, activate, and cancel
behavior to controllers without mentioning a controller API in GUI.

Buttons, toggle buttons, and check boxes activate with Enter or Space. Sliders
consume Left and Right. Escape closes cancellable dialogs. Buttons, choices,
text fields, and sliders render a visible focus treatment.

`Ui::set_text_input_service` accepts the platform-neutral Application service.
Focusing a `TextField` starts text input and supplies its stage rectangle;
layout changes update that rectangle, and leaving the field stops text input.
The GUI never includes SDL or Android types. Pre-edit composition is displayed
separately and only `TextInput` events commit text to the field.

The basic controls are:

- `Label` and `Image` for content;
- `Panel` and `Separator` for structure;
- `Button`, `ToggleButton`, and `CheckBox` for actions and choices;
- `TextField` for UTF-8 text insertion and cursor editing;
- `Slider` for continuous values.

The compositional containers are `Table`, `LinearLayout`, `Stack`,
`MarginContainer`, and `ScrollPane`. `Table` provides rows, column spans,
padding, alignment, fill, and weighted growth through chainable `Cell`
constraints. This makes adaptive forms and game menus possible without
absolute positioning.

`Window` is a floating, draggable, table-backed panel with a styled title bar.
It can expose a close control and edge/corner resizing, enforce an application
minimum size, and remain within the UI viewport. `Dialog` specializes it with
a content table, an action-button table, modal input blocking, result
callbacks, escape dismissal, and previous-focus restoration. `Ui` owns
floating window lifetimes and paints the modal dimming layer, so no platform
window API is involved.

All controls derive from `Widget`, which derives from `scene2d::Group`.
Minimum, preferred, and maximum size hints support adaptive handheld layouts;
default interactive controls enforce a 44-unit touch target.

## Skins and graphics

`Skin` is a named resource table for drawables and per-control styles.
`ColorDrawable` provides a dependency-free fallback. `RegionDrawable` accepts
the portable `graphics2d::TextureRegion` contract and sends it to `Painter`;
the GUI module never sees an SDL texture or renderer.

`NinePatchDrawable` divides a texture region into nine logical regions. Its
corners retain their source dimensions, edges stretch along one axis, and the
center stretches in both axes. When a destination is smaller than its fixed
borders, those borders scale proportionally instead of overlapping. Content
insets may be independent of the source splits.

Atlas loading belongs to the selected graphics backend. Once loaded, the
application binds regions into a skin:

```cpp
squared::gui::Skin skin;
skin.add_region_drawable(
    "button.normal",
    atlas.find_region("button.normal")->region(),
    {12.0F, 8.0F, 12.0F, 8.0F}
);

skin.add_nine_patch_drawable(
    "window.background",
    *atlas.find_region("window.background")
);

squared::gui::ButtonStyle action = skin.button_style("default");
action.normal = skin.drawable("button.normal");
skin.add_button_style("action", std::move(action));
```

The atlas overload reads libGDX `split` and optional `pad` metadata directly.
The region overload accepts explicit `NinePatchSplits` when metadata comes
from another source.

The package includes `app/src/main/assets/gui/kenney-test/skin.atlas`, a test
skin assembled from Kenney UI Pack 2.0 under CC0. Optional-module composition
places it directly in the Android asset set. It exercises image-backed
buttons, checkboxes, and sliders while preserving the same skin API used by
custom artwork.

The package also permanently carries a complete externally supplied
gdx-skins archive after it is imported with `tools/vendor-gdx-skins.fish` and
an explicit SHA-256. The importer validates archive paths and required asset
types, then writes `PIN.sha256`, `ASSET_INDEX.txt`, and the unchanged archive
as `assets/gui/gdx-skins/gdx-skins.zip`. Consequently every
generated project selecting Squared GUI receives the pinned files in its
internal assets.

The selected `gdx-holo` runtime files are also projected to
`assets/gui/gdx-skins/selected/gdx-holo/`. `load_libgdx_skin` accepts its JSON
as memory, normalizes libGDX's relaxed syntax under explicit limits, parses it
through Squared Data, resolves atlas drawables through a callback, and commits
only after the supported styles validate. See `Skin-Loading.md`.

The future HoloDisk-backed AssetManager will mount the complete archive and
supply bytes to this same loader; GUI does not depend on storage APIs.

## Minimal use

```cpp
squared::gui::Ui ui(width, height, std::move(skin));
auto column = std::make_unique<squared::gui::LinearLayout>();
column->add(std::make_unique<squared::gui::Label>("HoloDisk"));
column->add(std::make_unique<squared::gui::Button>("Mount", mount_disk));
ui.set_content(std::move(column));

ui.event(application_event);
ui.layout(painter);
ui.paint(painter);
```

## Tables, windows, and dialogs

The table API follows the useful shape of libGDX Scene2D UI while retaining
C++ ownership and Squared naming:

```cpp
auto form = std::make_unique<squared::gui::Table>();
form->add(std::make_unique<squared::gui::Label>("Name"))
    .align(squared::gui::Alignment::end);
form->add(std::make_unique<squared::gui::TextField>())
    .grow_x().fill_x();
form->row();
form->add(std::make_unique<squared::gui::Separator>())
    .column_span(2).grow_x().fill_x();

auto window = std::make_unique<squared::gui::Window>("Character");
window->set_closable(true);
window->set_resizable(true);
window->set_minimum_window_size({240.0F, 160.0F});
window->content_table().add(std::move(form)).grow().fill();
ui.show_window(std::move(window));
```

The close control captures its pointer independently from the draggable title
bar and asks `Ui` to remove the window safely after dispatch. Resizing starts
from any enabled edge or corner, never crosses the window's measured or
application-specified minimum, and is clamped to the `Ui` bounds. Dragging and
viewport resize events apply the same bounds constraint. A custom
`WindowStyle` supplies normal, hovered, and pressed close drawables plus close
size, resize-border thickness, and text color.

Dialogs are packed and centered during the next `Ui::layout` call when their
size is left at zero:

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

Modal dialogs consume pointer input outside their bounds. On dismissal, `Ui`
removes the dialog safely after event dispatch and restores the widget that
previously held focus.

The module is absent from both default Android template dependency graphs.
Projects opt in through Squared Project Generator's project-module workflow.
