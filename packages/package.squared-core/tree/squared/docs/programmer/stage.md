# stage: the App and the Stage

Internals: [../developer/stage.md](../developer/stage.md).

An app is a model and its screens. **`sq::App`** holds the Telegraph, runs
the model at its far end, and holds the **`sq::Stage`** at its near one: the
screens, the skin, drawing and input. You write two things, `squared_main`
and your model; the platform layer (the Android template's `entry.cpp`) does
the rest.

| Header | Gives you |
|---|---|
| `squared/stage/app.hpp` | `sq::App`, `squared_main` |
| `squared/stage/stage.hpp` | `sq::Stage`, `sq::ScreenType`, `sq::CustomMaker` |
| `squared/squared.hpp` | everything an app is written against: the App, the Stage, the widgets and layouts, the Telegraph |

Sprint 7.1, 7.2's C++ screens and custom nodes, and 7.3's panes. Binding a
widget to a value comes in 7.5.

## The shortest call site

```cpp
#include <layouts.hpp>
#include <squared/squared.hpp>

void squared_main( sq::App &app ) { app.stage.push( layouts::landing ); }
```

With a model:

```cpp
void squared_main( sq::App &app )
{
    app.run<Prefs>();
    app.stage.push( layouts::landing );
}
```

`layouts::landing` is `sq_app/layouts/landing.json`, compiled by the build
([layouts.md](layouts.md#sqgen-the-build-step)).

## `squared_main`

```cpp
void squared_main( sq::App &app );
```

Yours to write, in the global namespace. Called **once**, when the first
window is up and the skin is loaded: push the first screen here, and start
the model. Called again only after a quit, when the app starts afresh.

## `sq::App`

One for the whole process. The platform makes it the first time and keeps
it: when Android destroys the activity and makes another in the same process,
the new one reattaches to the same App, the model keeps running, and the
screens come back as they were.

| Member | Does |
|---|---|
| `stage` | the screens: `app.stage.push( layouts::landing )` |
| `run<M>( args... )` | start the model `M`, made from `args` on its own thread (named "model"). One model to an app: a second is refused, reported, and returns false |
| `run<M>( sq::messaging::on_ui_thread, args... )` | the same model on the UI thread, one turn a frame: for tests and debugging |
| `running()` | whether a model runs |
| `quit()` | quit: the model saves and stops, the platform closes the app |
| `telegraph()` | the UI end of the Telegraph (`sq::messaging::TelegraphUi`): send to the model, read what it said |

A model is a plain struct with `receive`, and optionally `start`, `tick`,
`background`, `foreground`, `low_memory` (the model's page,
[messaging.md](messaging.md)). The App calls them through the Telegraph:

| Android | The model |
|---|---|
| leaving the screen | `background( telegraph )`: save here; it may never come back |
| coming back | `foreground( telegraph, elapsed )`, `elapsed` in milliseconds, deep sleep included |
| short of memory | `low_memory( telegraph )` |
| a quit | `background`, if it was not already, then it is destroyed on its thread |

**Quitting** comes from `app.quit()`, a `quit` tap, or back at the first
screen (by the back button or a `back` tap). The model saves and stops, and
the stack is emptied; the next launch, even in the same process, runs
`squared_main` again with a new Telegraph, holding nothing of the last run.

The rest of `App` (`create`, `handle_event`, `render`, ...) is the
platform's `sq::app::Application` ([app.md](app.md)): the platform calls it,
an app does not.

## `sq::Stage`

A stack of screens, each a layout. Only the top one has widgets; a screen
that comes back (`back`) is built again from its layout, from scratch, so it
loses its scroll position and half-typed text until 7.6 keeps them.

| Member | Does |
|---|---|
| `push( layout )` | show `layout` over the screen there is. False, and reported, when the stack holds `max_depth` (16), or `layout` is a pane |
| `replace( layout )` | show `layout` instead of the screen there is, or as the first. False, and reported, for a pane |
| `back()` | what the back button does: close the top pane if one is open, or else back to the screen beneath. False at the first screen with no pane open: nothing to go back to |
| `open( layout )` | open the pane `layout` over the screen shown (below). False, and reported, when `max_panes` (4) are open or `layout` is a screen |
| `open<S>()` | open the C++ screen `S` as a pane |
| `close()` | close the top pane. False when none is open |
| `depth()` | how many screens, once anything waiting has happened |
| `panes()` | how many panes are open, once anything waiting has happened |
| `push<S>()`, `replace<S>()` | the same for the C++ screen `S` (below) |
| `add<S>()` | let taps reach the C++ screen `S` by its name (`push about`). In `squared_main`. False, and reported, for a name that is not one, that a layout file or another screen has, or past `max_added` (16) |
| `custom( name, make )` | make a layout's custom node `name` (`"type": "custom", "name": "minimap"`) with `make`. `name` must outlive the Stage: a literal does. False, and reported, for a name taken or past 16 |
| `top()` | the table shown: a layout, or what a C++ screen wrote (valid while shown, until a C++ pane is opened over it: they share one scratch); nullptr for none |
| `top_name()` | the name of the screen shown; empty for none |
| `pane_name()` | the name of the top pane; empty for none |
| `showing<S>()` | whether the C++ screen `S` is the one shown |
| `Stage::find( name )` | the app's layout called `name`, from its layout files; nullptr for none |
| `telegraph()` | the UI end of the Telegraph |
| `ui()` | the top screen's widgets, its panes' among them (`sq::gui::Ui`); nullptr while no skin is loaded |
| `failed()` | the skin did not load |

A layout passed to `push` or `replace` must outlive its place on the stack:
a compiled one (`layouts::settings`) is a constant, so it always does.

## C++ screens

What a layout file cannot say, a screen written in C++ can: a type with a
name and a `build`, no object (`sq::ScreenType` checks it):

```cpp
struct About {
    static constexpr std::string_view name = "about";
    static void build( sq::gui::LayoutWriter &root )
    {
        root.add<sq::gui::Label>( "this screen is written in C++" );
        root.row();
        root.add<sq::gui::Button>( "Back" ).tap( sq::gui::Navigate::back );
    }
};

app.stage.push<About>();
```

`build` writes the same table a layout file compiles to
([layouts.md](layouts.md#from-c-layoutwriter)), and runs **each time the
screen is shown**, so it keeps nothing between showings: values come from
the model. It writes into a scratch the Stage keeps per window,
`scratch_nodes` (256) nodes, `scratch_text` (8 KB) of text and
`scratch_numbers` (256) numbers; a screen that writes more shows what fit,
and is reported.

**To reach it by name**, from a layout file's `push about` or another
screen's tap, add it in `squared_main`, and list it in `names.json`'s
`screens` (sqgen checks a layout's targets against the files and that list):

```cpp
void squared_main( sq::App &app )
{
    app.stage.add<About>();
    app.stage.push( layouts::landing );
}
```

At start the Stage reports each screen `names.json` lists that was not
added. In C++, a tap can name the type instead: `.tap<About>(
sq::gui::Navigate::push )` writes `push about`, checked when it compiles.

## Panes

A pane is a layout over the screen shown: a question, a menu, a drawer. In a
file it is `"kind": "pane"`, with two keys of its own
([layouts.md](layouts.md#a-layout-file)):

```json
{ "kind": "pane", "place": "bottom", "modal": true, "rows": [
  [ { "type": "label", "text": "Quit?", "span": 2 } ],
  [ { "type": "button", "text": "Yes", "tap": "quit" },
    { "type": "button", "text": "No", "tap": "close" } ] ] }
```

A screen's button opens it, `"tap": "open quit"`; in C++,
`app.stage.open( layouts::quit )`. A button in it closes it, `"tap":
"close"`, as does back.

| `place` | Where |
|---|---|
| `center` (the default) | the middle, at the size it asks for |
| `left`, `right` | that edge, the screen's height |
| `top`, `bottom` | that edge, the screen's width |

Always inside the safe area, and never bigger than it. **Modal** (`true`;
the default is false): the screen dims and takes no taps until the pane
closes. Not modal: a tap outside the pane reaches the screen. A pane opened
over a modal pane is modal too, since nothing beneath a modal pane takes
taps.

A C++ screen opens as a pane too, `open<About>()` or `open about` (when it
is added); two constants, both optional, give its place and modal:

```cpp
struct Ask {
    static constexpr std::string_view name = "ask";
    static constexpr sq::gui::PanePlace place = sq::gui::PanePlace::bottom;
    static constexpr bool modal = true;
    static void build( sq::gui::LayoutWriter &root );
};
```

**A pane belongs to its screen.** Up to `max_panes` (4) are open at once;
the last opened is on top. Leaving the screen (a push, a replace, a back to
the screen beneath) closes them, and they do not come back with it. A pane
is built when it opens and freed when it closes, so a pane opened again
starts afresh: its values come from the model. A window made again (Android
recreating the activity) builds the screen and its open panes again.

## Custom nodes

A layout's `"type": "custom"` node (`add_custom( "minimap" )` in C++) is made
by a function the app gives:

```cpp
std::unique_ptr<sq::gui::Widget> make_minimap() { return std::make_unique<Minimap>(); }

app.stage.custom( "minimap", make_minimap );
```

A custom node with no maker shows a label saying so, and is reported.

**Navigation asked for while an event is being delivered waits** until
delivery has finished: a tap's `push settings` is applied after the tap,
since building the next screen replaces the widgets, the tapped button among
them. Up to four navigations can wait for one event; more are dropped and
reported.

## Taps

A button's tap (its `tap` in a layout file) does what it reads
([layouts.md](layouts.md#taps)):

| Tap | Does |
|---|---|
| `set ...`, `do ...` | the telegram, to the model, as it reads. A full Telegraph drops it, and says so |
| `push <name>`, `replace <name>` | that layout, found by `Stage::find`, or that added C++ screen; a name with neither is reported and does nothing |
| `open <name>` | that pane, or that added C++ screen as a pane, over the screen shown |
| `close` | the pane the button is in (whichever is on top) |
| `back` | what the back button does: the top pane, else back; at the first screen with no pane open, leave |
| `quit` | quit |

## The window

The Stage draws in **logical units**: the drawable's shorter side is 720, the
longer one follows its shape, so a widget is the same size on every screen
and a unit is square either way up. Touches arrive in pixels and are scaled
once, here. The safe area (the bars over the screen's edges) is kept clear.

The skin is the default one in the app's bundled files,
`skins/default/skin/` (`uiskin.atlas`, `uiskin.json`, `default.fnt`), which
the Android template ships; 7.4 moves it into the package. Rotation re-lays
out the screen without rebuilding it.

## When something goes wrong

Nothing stops the app; each failure is one line to the platform's report
(Android: the system log, and the Console, opened, in a `CONSOLE` build):

| Line | Means |
|---|---|
| `stage: the skin did not load: ...` | and why. The Stage draws dark red |
| `stage: <layout>: <problem> (node N)` | `instantiate` refused the layout (dark red), or built it short: a custom type or a drawable missing, the rest shown |
| `stage: tap <line>: no layout or added screen is called that` | a `push` or `replace` to a name that is neither |
| `stage: names.json lists the screen <name>, and it is not added: ...` | at start: `add<S>()` it in `squared_main` |
| `stage: add <name>: ...` | not a name; a layout file or another screen has it; 16 added already |
| `stage: custom <name>: ...` | no name or maker; another maker has it; 16 already |
| `stage: <screen>: not all of it was written: ...` | a C++ screen wrote past the scratch, or a call it made was refused; what fit is shown |
| `stage: tap <line>: not a telegram` | a `set` or `do` line that does not read |
| `stage: tap <line>: the Telegraph had no room; not sent` | the ring was full |
| `stage: push <layout>: a pane: open it` | `push` or `replace` of a pane |
| `stage: open <layout>: a screen: push it` | `open` of a screen |
| `stage: open <name>: 4 panes are open already` | `max_panes` |
| `stage: tap close: this is a screen, not a pane` | a C++ screen's `close` tap |
| `stage: push <layout>: the stack holds 16 screens` | `max_depth` |
| `app: run: a model runs already; an app has one` | a second `run` |

## Memory

The App is made once, in static storage, not on the heap: the Telegraph line
(88,000 bytes at the default sizes, which its knobs set) and 1,216 bytes more
(the Stage is 1,104: its stack, the added screens and custom types, the pane
slots). A model's runner is one allocation, at `run`. Each window allocates
the skin's textures and tables and one block of about 19 KB (on Android
19,128 bytes: batch, camera, painter, widgets, and the 17 KB scratch C++
screens and panes write into); each screen shown allocates its widgets, and
each pane opened its window (304 bytes) and widgets, freed when it closes. A
C++ screen itself allocates nothing. **A frame allocates nothing.**

## Lua

Not bound: the Lua layer is not built yet.
