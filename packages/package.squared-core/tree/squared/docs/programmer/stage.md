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
| `squared/stage/stage.hpp` | `sq::Stage` |
| `squared/squared.hpp` | everything an app is written against: the App, the Stage, the widgets and layouts, the Telegraph |

Sprint 7.1. Panes (`open`, `close`) come in 7.3, binding a widget to a value
in 7.5, C++ screens (a `build()`) in 7.2.

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
| `push( layout )` | show `layout` over the screen there is. False, and reported, when the stack holds `max_depth` (16) |
| `replace( layout )` | show `layout` instead of the screen there is, or as the first |
| `back()` | back to the screen beneath. False at the first screen: nothing to go back to |
| `depth()` | how many screens, once anything waiting has happened |
| `top()` | the screen shown; nullptr for none |
| `Stage::find( name )` | the app's layout called `name`, from its layout files; nullptr for none |
| `telegraph()` | the UI end of the Telegraph |
| `ui()` | the top screen's widgets (`sq::gui::Ui`); nullptr while no skin is loaded |
| `failed()` | the skin did not load |

A layout passed to `push` or `replace` must outlive its place on the stack:
a compiled one (`layouts::settings`) is a constant, so it always does.

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
| `push <name>`, `replace <name>` | that layout, found by `Stage::find`; a name with no layout is reported and does nothing |
| `back` | back; at the first screen, leave (as the back button does there) |
| `quit` | quit |
| `open <name>`, `close` | reported: panes come in 7.3 |

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
| `stage: tap <line>: no layout is called that` | a `push` or `replace` to a name that is not a layout |
| `stage: tap <line>: not a telegram` | a `set` or `do` line that does not read |
| `stage: tap <line>: the Telegraph had no room; not sent` | the ring was full |
| `stage: tap <line>: panes come in 7.3` | `open` or `close` |
| `stage: push <layout>: the stack holds 16 screens` | `max_depth` |
| `app: run: a model runs already; an app has one` | a second `run` |

## Memory

The App is made once, in static storage, not on the heap: the Telegraph line
(88,000 bytes at the default sizes, which its knobs set) and 384 bytes more.
A model's runner is one allocation, at `run`. Each window allocates the
skin's textures and tables and one block of 1,648 bytes on Android (batch,
camera, painter, widgets); each screen shown allocates its widgets. **A frame allocates
nothing.**

## Lua

Not bound: the Lua layer is not built yet.
