# Getting started

## The shortest real call site

`Ui` owns the widget tree; you give it a `Painter` implementation and feed it
events. Building a screen is three statements:

```cpp
#include <squared/gui/gui.hpp>

sq::gui::Ui ui{1280.0F, 720.0F};

auto& root = ui.set_content(std::make_unique<sq::gui::LinearLayout>());
static_cast<sq::gui::LinearLayout&>(root)
    .add(std::make_unique<sq::gui::Label>("hello"));
```

Per frame:

```cpp
ui.update(delta_seconds);
ui.layout(painter);
ui.paint(painter);
```

Events arrive either as framework events or as direct injections:

```cpp
ui.event(application_event);
ui.pointer(sq::gui::PointerAction::down, x, y);
ui.key_down(sq::gui::Key::enter);
```

## Supplying a Painter

`Painter` is the only rendering dependency the GUI has. Implement its six pure
virtuals over your 2D backend and the whole widget set works:

```cpp
class MyPainter final : public sq::gui::Painter {
    sq::gui::Size measure_text(std::string_view) override;
    void fill_rectangle(const sq::gui::Rectangle&,
                        sq::graphics::Color) override;
    void stroke_rectangle(const sq::gui::Rectangle&,
                          sq::graphics::Color, float) override;
    void draw_region(const sq::graphics2d::TextureRegion&,
                     const sq::gui::Rectangle&,
                     sq::graphics::Color) override;
    void draw_text(std::string_view, float, float,
                   sq::graphics::Color) override;
    void push_clip(const sq::gui::Rectangle&) override;
    void pop_clip() override;
};
```

The two font-aware overloads (`measure_text` with a `FontResource*`, and the
matching `draw_text`) have base implementations, so a painter that ignores
bitmap-font pages still compiles and runs.

## Buttons and callbacks

```cpp
auto button = std::make_unique<sq::gui::Button>("Play", [] { start(); });
button->set_style("primary");
```

## Dialogs

```cpp
auto dialog = std::make_unique<sq::gui::Dialog>(
    "Quit", [](std::string_view result) { if (result == "yes") quit(); });
dialog->text("Discard unsaved changes?")
      .button("Cancel", "no")
      .button("Quit", "yes");
ui.show_dialog(std::move(dialog));
```

## Skins

A `Skin` carries palette values, named drawables, named fonts, and one style
table per widget kind. Every lookup falls back to the style named `default`.

```cpp
ui.skin().add_nine_patch_drawable("button-up", atlas_region);

sq::gui::ButtonStyle style;
style.normal = ui.skin().drawable("button-up");
ui.skin().add_button_style("primary", style);
```

Loading a libGDX skin JSON document is a single call; see
`squared/gui/skin_loader.hpp`.

## What throws

Several constructors and `Skin` style lookups currently signal failure by
throwing (`std::invalid_argument`, `std::out_of_range`). This contradicts the
framework's own no-exceptions rule and is tracked in
[../developer/priority-audit.md](../developer/priority-audit.md). Until it is
resolved, build with exceptions enabled.
