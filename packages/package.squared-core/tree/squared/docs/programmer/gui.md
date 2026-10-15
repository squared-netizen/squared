# gui

The libGDX-flavoured widget set, skinning and the `Ui` host, on top of the scene graph they're built from: `Actor`, `Group`, `Stage` and input routing.

Each type lives in its own header. The headers at the module root (`squared/gui/*.hpp`) are the everyday ones. Those under `squared/gui/src/` are supporting types: you can include them directly, but the root headers that use them already include them. `squared/gui/gui.hpp` pulls in the whole module, and `squared/gui/input.hpp` pulls in the input-routing types together. Both are there for convenience; don't include them from a header of your own.

Developer counterpart: [../developer/gui.md](../developer/gui.md)

## Scene graph

| Type | Header | Purpose |
|---|---|---|
| `Actor` | `squared/gui/actor.hpp` | Base node with parent-relative bounds and frame traversal |
| `Group` | `squared/gui/group.hpp` | Actor that owns children in deterministic insertion order |
| `Stage` | `squared/gui/stage.hpp` | Root owner for a translation-only two-dimensional actor hierarchy |
| `ActorInterfaceId` | `squared/gui/src/actor_interface_id.hpp` | Stable identifier for one layer's Actor-derived interface |

## Input routing

| Type | Header | Purpose |
|---|---|---|
| `InputEvent` | `squared/gui/src/input_event.hpp` | Mutable event routed through a Stage actor path |
| `InputType` | `squared/gui/src/input_type.hpp` | Event kind carried by one InputEvent |
| `InputPhase` | `squared/gui/src/input_phase.hpp` | Dispatch phase of an event inside a Stage |
| `InputListener` | `squared/gui/src/input_listener.hpp` | Callback invoked for each phase during input dispatch |
| `InputListenerId` | `squared/gui/src/input_listener_id.hpp` | Stable actor-local listener identifier; insertions increase it |
| `InputKey` | `squared/gui/src/input_key.hpp` | Portable hardware-independent key names for key events |
| `InputModifiers` | `squared/gui/src/input_modifiers.hpp` | Backend-neutral modifier state copied at the input boundary |
| `NavigationAction` | `squared/gui/src/navigation_action.hpp` | Semantic focus and navigation action from any supported device |

## Geometry

| Type | Header | Purpose |
|---|---|---|
| `Size` | `squared/gui/src/size.hpp` | Two-dimensional extent in logical units |
| `Rectangle` | `squared/gui/src/rectangle.hpp` | Axis-aligned rectangle in logical units |
| `Insets` | `squared/gui/src/insets.hpp` | Insets from the edges of a rectangle, in logical units |
| `SizeHints` | `squared/gui/src/size_hints.hpp` | Proposed minimum, preferred, and maximum sizes for one widget |

## Drawing boundary

| Type | Header | Purpose |
|---|---|---|
| `Painter` | `squared/gui/src/painter.hpp` | Portable drawing boundary, implemented over `sq::graphics` by `BatchPainter` |
| `Drawable` | `squared/gui/src/drawable.hpp` | Skin image abstraction; it never exposes SDL or another backend type |
| `DrawablePtr` | `squared/gui/src/drawable_ptr.hpp` | Shared ownership of an immutable drawable |
| `ColorDrawable` | `squared/gui/src/color_drawable.hpp` | Drawable that paints a flat, optionally inset color |
| `RegionDrawable` | `squared/gui/src/region_drawable.hpp` | Drawable that samples one texture region |
| `NinePatchSplits` | `squared/gui/src/nine_patch_splits.hpp` | Scissor amounts used by nine-patch scaling |
| `NinePatchDrawable` | `squared/gui/src/nine_patch_drawable.hpp` | Scalable drawable that preserves corners and stretches edges and center |
| `BatchPainter` | `squared/gui/batch_painter.hpp` | The `Painter` that draws widgets through a `graphics::SpriteBatch` |

## Fonts

| Type | Header | Purpose |
|---|---|---|
| `FontResource` | `squared/gui/src/font_resource.hpp` | Immutable portable bitmap-font resource selected by GUI styles |
| `FontPtr` | `squared/gui/src/font_ptr.hpp` | Immutable shared ownership of one GUI font resource |

## Styles

| Type | Header | Purpose |
|---|---|---|
| `PanelStyle` | `squared/gui/src/panel_style.hpp` | Style data for a Panel widget |
| `LabelStyle` | `squared/gui/src/label_style.hpp` | Style data for a Label widget |
| `ButtonStyle` | `squared/gui/src/button_style.hpp` | Style data for a Button widget |
| `TextFieldStyle` | `squared/gui/src/text_field_style.hpp` | Style data for a TextField widget |
| `CheckBoxStyle` | `squared/gui/src/check_box_style.hpp` | Style data for a CheckBox widget |
| `SliderStyle` | `squared/gui/src/slider_style.hpp` | Style data for a Slider widget |
| `ProgressBarStyle` | `squared/gui/src/progress_bar_style.hpp` | Style data for a non-interactive ProgressBar widget |
| `WindowStyle` | `squared/gui/src/window_style.hpp` | Style data for a Window widget |
| `Skin` | `squared/gui/skin.hpp` | Named, reusable skin resources and widget styles |

## Input

| Type | Header | Purpose |
|---|---|---|
| `PointerAction` | `squared/gui/src/pointer_action.hpp` | Pointer action delivered to widgets |
| `PointerEvent` | `squared/gui/src/pointer_event.hpp` | Portable pointer event payload in widget-local logical units |
| `Key` | `squared/gui/src/key.hpp` | Portable key name used by the GUI input boundary |
| `KeyModifiers` | `squared/gui/src/key_modifiers.hpp` | Modifier state reused from `InputModifiers` in the input-routing contract |

## Widget base

| Type | Header | Purpose |
|---|---|---|
| `Widget` | `squared/gui/widget.hpp` | Base class for every GUI node. Widgets may own other widgets |
| `TooltipConfig` | `squared/gui/src/tooltip_config.hpp` | Timing and placement policy for transient Ui tooltips |

## Leaf widgets

| Type | Header | Purpose |
|---|---|---|
| `Label` | `squared/gui/label.hpp` | Read-only text widget |
| `Image` | `squared/gui/image.hpp` | Widget rendering one shared drawable |
| `Separator` | `squared/gui/separator.hpp` | Thin vertical or horizontal divider line |
| `ProgressBar` | `squared/gui/progress_bar.hpp` | Read-only determinate progress indicator |

## Containers

| Type | Header | Purpose |
|---|---|---|
| `Direction` | `squared/gui/src/direction.hpp` | Main stacking axis of a linear layout |
| `Alignment` | `squared/gui/src/alignment.hpp` | Per-cell alignment relative to the cell box |
| `Panel` | `squared/gui/panel.hpp` | Container painting a single style background |
| `Cell` | `squared/gui/cell.hpp` | Per-child constraints returned by Table::add for fluent configuration |
| `Table` | `squared/gui/table.hpp` | Grid layout with libGDX-style rows and chainable cell constraints |
| `LinearLayout` | `squared/gui/linear_layout.hpp` | Container stacking children along one axis with optional growth |
| `Stack` | `squared/gui/stack.hpp` | Container overlaying children into the same box, later on top |
| `MarginContainer` | `squared/gui/margin_container.hpp` | Container adding fixed insets around one child |
| `ScrollPane` | `squared/gui/scroll_pane.hpp` | Container clipping and vertically scrolling a larger child |

## Controls

| Type | Header | Purpose |
|---|---|---|
| `Button` | `squared/gui/button.hpp` | Clickable, focusable control with an optional label and icon |
| `ToggleButton` | `squared/gui/toggle_button.hpp` | Two-state button that reports changes through a callback |
| `CheckBox` | `squared/gui/check_box.hpp` | Toggle button rendering a checkbox glyph and label |
| `RadioButton` | `squared/gui/radio_button.hpp` | CheckBox-shaped choice intended for a one-of-many ButtonGroup |
| `ButtonGroup` | `squared/gui/button_group.hpp` | Non-owning coordinator for toggle buttons and radio-style choices |
| `TextField` | `squared/gui/text_field.hpp` | Single-line editable text input with cursor and composition support |
| `Slider` | `squared/gui/slider.hpp` | Draggable one-axis value selector with an optional gradient fill |

## Overlays

| Type | Header | Purpose |
|---|---|---|
| `Window` | `squared/gui/window.hpp` | Floating table-backed panel with a draggable title bar |
| `Dialog` | `squared/gui/dialog.hpp` | Modal Window with separate content and action-button tables |

## Host

| Type | Header | Purpose |
|---|---|---|
| `Ui` | `squared/gui/ui.hpp` | Owns one widget tree and consumes the framework application event type |

### Keeping clear of the status bar

A phone draws the interface edge to edge, so the status bar, the navigation
bar and a camera cut-out sit on top of it. Give the `Ui` the distances and the
content moves in from each edge:

```cpp
ui.set_safe_area({.left = 0.0F, .top = 48.0F, .right = 0.0F, .bottom = 32.0F});
```

Values are logical units. Negative or non-finite values count as zero; insets
larger than the viewport leave the content empty. The setting survives
`resize()` and `set_content()`, so call it again only when the bars change
&mdash; `Application::safe_area_changed()` is where you hear that. Windows and
dialogs are not inset; they may use the whole viewport. `safe_area()` reads the
current value. Default: all zero, which is the previous behaviour.

Lua: not bound.

### Do not change the content from inside a callback

`set_content()` destroys the widgets it replaces, and a button's callback runs
while that button is still handling the tap. Switching screens from a callback
therefore frees the widget that is calling you. Record the request and act on
it after the event has been handled (the demo's `ScreenStack` does this).

## Skin loading

`load_libgdx_skin` reads a libGDX `uiskin.json`. Style tables may be keyed by
the full libGDX class name (as a stock skin has them) or by its last part:
`com.badlogic.gdx.scenes.scene2d.ui.TextButton$TextButtonStyle` and
`TextButton$TextButtonStyle` are the same table. Supported: `Color`,
`BitmapFont`, `Skin$TintedDrawable`, `Button$ButtonStyle`,
`TextButton$TextButtonStyle`, `TextField$TextFieldStyle`, `Label$LabelStyle`,
`CheckBox$CheckBoxStyle`, `Slider$SliderStyle`, `ProgressBar$ProgressBarStyle`
and `Window$WindowStyle`; any other class in the file is reported as a warning.
Giving one class twice, once in each form, is an error.

Lua: not bound.

| Type | Header | Purpose |
|---|---|---|
| `SkinLoadSeverity` | `squared/gui/src/skin_load_severity.hpp` | Severity of one skin-loading diagnostics entry |
| `SkinLoadIssue` | `squared/gui/src/skin_load_issue.hpp` | One path-tagged diagnostics entry produced by skin loading |
| `SkinLoadLimits` | `squared/gui/src/skin_load_limits.hpp` | Explicit resource limits applied to one transactional skin load |
| `SkinLoadReport` | `squared/gui/src/skin_load_report.hpp` | Counted result of one transactional skin load |
| `SkinDrawableResolver` | `squared/gui/src/skin_drawable_resolver.hpp` | Resolves one atlas/resource name without exposing storage or backend APIs |
| `SkinFontResolver` | `squared/gui/src/skin_font_resolver.hpp` | Resolve one declared bitmap-font resource without fixing an I/O API |

## Text-entry widgets and the soft keyboard

`Ui` raises, moves and stops the platform text-input service based on
`Widget::wants_text_input()`. It returns false by default and `TextField`
returns true, so a custom text-entry widget opts in by overriding it:

```cpp
class CodeEditor final : public sq::gui::Widget {
public:
    [[nodiscard]] bool focusable() const noexcept override { return true; }
    [[nodiscard]] bool wants_text_input() const noexcept override
    {
        return true;
    }
};
```

Without the override the widget still takes keyboard focus and still receives
`text_input()`, but the on-screen keyboard is not raised for it on Android.

## Downcasting an Actor

squared builds with `-fno-rtti`, so `dynamic_cast` is not available. Use
`actor_cast` instead:

```cpp
for (std::size_t i = 0; i < group.child_count(); ++i) {
    if (auto* widget = sq::gui::actor_cast<sq::gui::Widget>(
            group.child_at(i))) {
        // this child is a Widget
    }
}
```

It accepts null, returns null when the actor does not publish the requested
interface, and has a `const` overload. `Group` and `Widget` both work as the
target type.

To make your own `Actor` subclass reachable the same way, publish an
identifier and override both `actor_interface` overloads, delegating to your
direct base for identifiers you do not recognise:

```cpp
class MyNode : public sq::gui::Group {
public:
    static constexpr sq::gui::ActorInterfaceId
        actor_interface_id = 0x4D'59'4E'44U;

    [[nodiscard]] void* actor_interface(
        sq::gui::ActorInterfaceId id) noexcept override
    {
        return id == actor_interface_id ? this : Group::actor_interface(id);
    }

    [[nodiscard]] const void* actor_interface(
        sq::gui::ActorInterfaceId id) const noexcept override
    {
        return id == actor_interface_id ? this : Group::actor_interface(id);
    }
};
```

## Drawing the interface

Widgets draw through a `Painter`. `BatchPainter` is the one that puts them on
screen:

```cpp
#include <squared/gui/batch_painter.hpp>

// Once, after the skin's atlas and font are loaded:
sq::gui::BatchPainter painter{batch, font.get()};   // font: the skin's FontPtr
const sq::graphics::AtlasRegion* white = atlas.find_region("white");
if (!painter.set_fill_source(white != nullptr ? &white->region() : nullptr)) {
    // no fill source at all; solid fills are skipped
}
painter.set_viewport(640.0F, 480.0F, graphics.pixel_width(),
                     graphics.pixel_height());

// Every frame:
ui.layout(painter);          // does nothing unless something changed
if (batch.begin(camera)) {
    ui.paint(painter);
    batch.end();
}
```

Keep one painter for the life of the interface. It holds the scratch storage
text is laid out in, so a new painter per frame would allocate again.

Call `set_viewport()` again on resize. Only clipping needs it.

**Pass the skin's `white` region.** Solid fills are drawn from it, so they
share an atlas page with every other widget graphic and the interface costs
one draw call instead of one per fill. Both stock skins ship a `white` region
for exactly this. Passing `nullptr` works and costs a draw call per fill.

Clipping is handled for you: a scroll pane pushes a clip, and anything drawn
inside it is scissored to that rectangle. Nested clips intersect.
