# Getting started

## An app

An app is a layout file and one function. `sq_app/layouts/landing.json`:

```json
{ "rows": [ [ { "type": "label", "text": "Hello" } ],
            [ { "type": "button", "text": "Quit", "tap": "quit" } ] ] }
```

and `sq_app/src/main.cpp`:

```cpp
#include <layouts.hpp>
#include <squared/squared.hpp>

void squared_main( sq::App &app ) { app.stage.push( layouts::landing ); }
```

The build compiles the layout ([layouts.md](layouts.md)); the App loads the
skin, shows the screen, routes touches, and quits on the button
([stage.md](stage.md)). A generated Android project starts as exactly this.
Everything an app writes goes through those two: screens and panes as layout
files or C++ screens, a model, and the Stage's calls. The widgets' host under
them (`gui::Ui`) is internal since 7.7.

## Your own widget

What a layout cannot express is a custom node: a `sq::gui::Widget` of your
own, made by a function the app registers.

```cpp
class Dial final : public sq::gui::Widget {
public:
    [[nodiscard]] sq::gui::Size preferred_size(sq::gui::Painter&,
                                               const sq::gui::Skin&) const override
    {
        return {120.0F, 120.0F};
    }

    void paint(sq::gui::Painter& painter, const sq::gui::Skin&, float x,
               float y) const override
    {
        painter.fill_rectangle({x, y, width(), height()}, sq::graphics::Color::white());
    }
};

std::unique_ptr<sq::gui::Widget> make_dial() { return std::make_unique<Dial>(); }

app.stage.custom("dial", make_dial);   // in squared_main; "type": "custom", "name": "dial"
```

A widget draws through the `Painter` it is given: `fill_rectangle`,
`stroke_rectangle`, `draw_region`, `draw_text` and `measure_text`, with
`push_clip` and `pop_clip`. Input comes to `pointer_event`, `key_down` and
`text_input`, and `focusable()` says whether it takes the focus
([gui.md](gui.md)).

## Skins

One skin for now: libGDX's default, compiled into the stage library
([stage.md](stage.md#the-window)); an app ships none. A `Skin` carries palette
values, named drawables, named fonts, and one style table per widget kind;
every lookup falls back to the style named `default`. An app's own skin is
in the backlog.

## What throws

Several constructors and `Skin` style lookups currently signal failure by
throwing (`std::invalid_argument`, `std::out_of_range`). This contradicts the
framework's own no-exceptions rule and is tracked in
[../developer/priority-audit.md](../developer/priority-audit.md). Until it is
resolved, build with exceptions enabled.
