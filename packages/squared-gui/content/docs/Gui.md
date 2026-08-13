# Squared GUI

Squared GUI is an optional portable retained-mode interface layer. It uses
Scene2D for ownership and hierarchy, consumes the framework's existing
`application::Event` values, and draws through Graphics and Graphics2D. It has
no dependency on SDL, OpenGL, Android, HoloDisk, or a particular font system.

## Small, composable boundary

`squared::gui::Ui` owns one widget tree. Its public input boundary accepts
Application pointer and resize events directly. Keyboard and text methods
remain explicit until those event kinds exist in Application. Pointer events
bubble through the existing Scene2D parent chain, and capture is tracked by
the framework pointer ID; GUI does not introduce another general event bus.

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
`Dialog` specializes it with a content table, an action-button table, modal
input blocking, result callbacks, escape dismissal, and previous-focus
restoration. `Ui` owns floating window lifetimes and paints the modal dimming
layer, so no platform window API is involved.

All controls derive from `Widget`, which derives from `scene2d::Group`.
Minimum, preferred, and maximum size hints support adaptive handheld layouts;
default interactive controls enforce a 44-unit touch target.

## Skins and graphics

`Skin` is a named resource table for drawables and per-control styles.
`ColorDrawable` provides a dependency-free fallback. `RegionDrawable` accepts
the portable `graphics2d::TextureRegion` contract and sends it to `Painter`;
the GUI module never sees an SDL texture or renderer.

Atlas loading belongs to the selected graphics backend. Once loaded, the
application binds regions into a skin:

```cpp
squared::gui::Skin skin;
skin.add_region_drawable(
    "button.normal",
    atlas.find_region("button.normal")->region(),
    {12.0F, 8.0F, 12.0F, 8.0F}
);

squared::gui::ButtonStyle action = skin.button_style("default");
action.normal = skin.drawable("button.normal");
skin.add_button_style("action", std::move(action));
```

The package includes `app/src/main/assets/gui/kenney-test/skin.atlas`, a test
skin assembled from Kenney UI Pack 2.0 under CC0. Optional-module composition
places it directly in the Android asset set. It exercises image-backed
buttons, checkboxes, and sliders while preserving the same skin API used by
custom artwork.

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
window->content_table().add(std::move(form)).grow().fill();
ui.show_window(std::move(window));
```

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
