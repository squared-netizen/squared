# stage &mdash; internals

Programmer docs: [../programmer/stage.md](../programmer/stage.md). The
decision record: the refactor plan's 7.1 ("yes to all", 2026-10-26) and the
app model's Ownership and Lifecycle sections.

`stage/app.hpp` (`sq::App`, `squared_main`), `stage/stage.hpp`
(`sq::Stage`), `src/app.cpp`, `src/stage.cpp`, and `src/catalog.cpp` with
`src/detail/catalog.hpp` (the layouts' default). `squared/squared.hpp` at the
tree's root includes it with gui and messaging. Sprint 7.1, 2026-10-26; 7.2's
screen stack folded in.

Depends on app (headers), gui, graphics, assets (the `Runtime`'s), files,
messaging, time, data (through gui's skin loader) and math. Nothing depends
on it: an app links it, and an app without an `sq::App` (the demo, still on
its own `Application`) pulls none of it in, since a static archive's member
is linked only when something refers to it.

## Ownership and lifetimes

| Object | Lives | Made by |
|---|---|---|
| `App` | the process (static storage in the platform layer) | the platform, on first use |
| `TelegraphLine` | inside the App; made again after a quit | the App |
| the model's runner | from `run` to the quit | `run`, one allocation |
| `Stage` | inside the App | the App |
| `Stage::Drawing` (skin pieces, batch, camera, painter, widgets) | a window: `attach` to `detach` | the Stage, one allocation |
| the top screen's widgets | from shown to replaced | `instantiate`, on each navigation |

**Member order is the start order.** `line_` is the App's first member,
private and declared before the public `stage`, so it is made first and
destroyed last; the model's runner is declared after both, so it goes first
(the destructor resets it explicitly as well): the model is joined while the
Telegraph exists, as the ownership rules require.

**A recreated activity.** The platform's `Platform` (per activity) holds a
reference to the process's App. `create()` always calls `Stage::attach`
(a new window: the skin is loaded again into new textures, the top screen is
built again), and calls `squared_main` only when `started_` is false: the
first window, or the first since a quit. `dispose()` without a quit only
detaches the window's half; the model runs on.

**A quit** (`quit()`, the Stage's `leave_` from a `quit` tap or back at the
root, or an unconsumed back or quit event) is acted on in `dispose()`, after
the platform finished the activity: the runner is destroyed (the model saves
in `background` if the app was visible, then ends), the line is made again,
the stack is cleared, `started_` and the flags reset. Then the next launch in
the same process is a first start.

**The line is made again** with `std::destroy_at` and `std::construct_at` in
its own storage. A `TelegraphLine` serves one model: after its runner is gone
it keeps that model's `quitting_` flag, its cached values and its rings, so a
second runner would end at its first turn (found by the test: the threaded
model looked answered only because the first model's last value was still
cached). Making it again keeps the Stage's `TelegraphUi &` valid (same
object storage, same type: transparently replaceable) and costs a zeroing of
88 KB, once per quit. Rejected: a `reset()` on `TelegraphLine` in messaging,
which would have to reset every member it ever gains.

**Not exited.** The app model's "quit ends the process" is met in effect, not
literally: after a quit the process stays cached by Android, but nothing of
the last run is left in it (no model, a new line, an empty stack), so the
next launch starts clean. Calling `exit` from the glue's thread was not worth
the risk.

## `App::run` and `detail::ModelHolder`

`ModelRunner<M>` is a template on the model's type, which the App cannot
name. `run<M>` keeps it in a `detail::ModelOf<M>`, behind
`detail::ModelHolder` (a virtual destructor and a virtual `turn()`): one heap
allocation, once, holding the runner and the model inside it (the runner
already holds the model in place). Decided in 7.1's choice 5 over a fixed
buffer in the App (no heap, but a make variable to raise as a model grows)
and over `App<Model>` (a template every type would depend on). Cost: 8 bytes
in the App, a vtable, and 16 bytes of heap bookkeeping; `turn()` is one
virtual call a frame, inline models only. A second `run` is refused (the
runner's own precondition is one to a line).

## The Stage

936 bytes (`static_assert`ed; pointers, `string_view`s and arrays of them,
the same in both standard libraries): the screens (16 `Shown`s, two pointers
each), 4 waiting changes (a `TapKind` and a `Shown`), the 16 added C++
screens (a pointer each), the 16 custom types (a name and a function pointer
each), the report, the text-input service, the `Drawing` pointer, the pixel
and logical sizes, the safe area, a depth, three counts and four flags. Was
264 in 7.1; 7.2's registries and the second pointer per screen are the rest.

**The stack holds a layout or a C++ screen per entry**, nothing else (7.1's
choice 7, 7.2's choice 3): `Shown` is a `const Layout *` and a
`const detail::ScreenEntry *`, one of them set. Only the top screen has
widgets. `show_top` makes a new root `Table`, hands it to
the `Ui` (which frees the old tree), and runs `instantiate` with the Stage's
hooks: `tapped` is a static member taking the Stage as context. So `back`
rebuilds the screen beneath from its layout, and 7.6's "release hidden
screens under memory pressure" has nothing left to release. Cost: a
navigation allocates a screen's widgets (tens of small allocations); a frame
allocates nothing.

**Deferred navigation**, a bounded command queue. A tap runs inside
`Ui::event`, inside the tapped button: building the next screen there would
free the button while it runs. So while `dispatching_` is set, `push`,
`replace` and `back` go into `pending_` (4 entries, no allocation) and
`flush()` applies them after delivery, a copy of the queue first so a change
that asks for another (none does yet) cannot loop. Outside an event they
apply at once, which is what `squared_main`'s `push` needs. `depth()`
accounts for what waits, so `back()` and the stack limit answer for the state
after the queue. The demo's `ScreenStack` did the same with a
`std::vector<Pending>` of `unique_ptr<Screen>`s; this one holds two words a
change.

**Taps** are read with `gui::read_tap`. A telegram line is parsed by
`messaging::read_telegram` into 1 KB on the stack (`TelegramData`) and sent
as it is with `TelegraphUi::send`: the replay tools' path, so a tap and a
replayed line are the same thing. Navigation finds its target with
`Stage::find`.

**`Stage::find` and the weak default.** The names a layout's taps navigate to
are the app's layout files, which only sqgen's generated code knows. sqgen
already writes `layouts::all()` (sorted); the Stage declares the same
function (`src/detail/catalog.hpp`) and binary-searches it. An app with no
layout files has no definition, so `src/catalog.cpp` defines an empty one,
`[[gnu::weak]]`, alone in its file: through the archive it is linked only
when nothing else defines it, and linked as an object beside a strong
definition (the stage test does both) the strong one wins. No registration
call, no static constructor. ELF behaviour, on both targets (Linux, Android).

## C++ screens (7.2)

**A screen is a type** (`sq::ScreenType`: a `name` convertible to
`string_view` and a static `build( LayoutWriter & )`), not an object: the
Stage keeps `&detail::screen_entry<S>`, an `inline constexpr` variable
template of the name and the function pointer, one per type in read-only
data. So pushing a screen allocates nothing and costs no vtable; there is no
per-screen state to lose (the model holds the values; 7.6's record will hold
a scroll position). Rejected (7.2's choice 1): the spec's `sq::Screen`
subclass with a virtual `build()` and an object per push.

**`write`** makes a `LayoutWriter` over the window's scratch
(`Drawing::Scratch`: 256 nodes, 8 KB of text, 256 numbers, and the `Layout`
it wrote: 17,480 bytes, the same in both libraries) and calls `build`; an
incomplete writer (storage out, or a refused call) is reported and what it
wrote is shown, since the writer's table is always consistent. `instantiate`
copies what the widgets keep (words, tap lines), so the scratch is free again
as soon as the screen is built; `back` writes it again.

**By name.** `add<S>()` puts the entry in a table of 16 (`max_added`),
refusing an invalid name (`messaging::valid_name`), a name a layout file has
(a layout wins any lookup, so the screen could never be reached), and a name
another entry has; adding the same entry again is a no-op, which a
`squared_main` run again after a quit relies on (the tables are cleared on a
quit anyway). A tap's target resolves to a layout first (`find`, a binary
search), then an added screen (a linear scan of at most 16). sqgen writes
`names.json`'s `screens` as `layouts::screens()`, sorted, beside
`layouts::all()`, with the same weak default; after `squared_main`,
`check_screens` reports each one not added. That is the check the build
cannot make: the layouts' targets are checked against the list at build
time, the list against the app's `add` calls at start.

**Typed taps** live in gui, which knows no Stage: `NodeWriter::tap<S>( how )`
takes any `S` with a `name` (`gui::detail::named`) and writes the same line
`tap( how, S::name )` would.

**Custom nodes.** `custom( name, make )` puts a name and a
`CustomMaker` (`std::unique_ptr<gui::Widget> (*)()`) in a table of 16; the
Stage's `made`, a static member, is `instantiate`'s `custom` hook and finds
the maker by a linear scan. No maker: `instantiate` stands a label in, and
reports the node as built short. A function pointer, not `std::function`: a
maker needs no captures (it makes a widget, the widget reads the model).

## `Stage::Drawing`

19,128 bytes under libc++ (Android, Termux) and 19,312 under libstdc++:
1,648 and 1,832 for the window's own pieces (`static_assert`ed for each: the
`Ui` holds standard containers, whose sizes are the library's) and the
17,480-byte scratch, last, so the two add exactly. One allocation per
window: the atlas, the font, the sprite batch, the `Ui`, a `loaded` flag, the
camera, the painter and the scratch, **in that order**, so the constructor can load in its member
initializers: `loaded( load( files, why ) )` loads the atlas, the font and the
skin straight into the `Ui`'s own skin (no copy, no move), then the camera and
the painter (which takes the loaded font) are made; the body sets the
painter's fill from the skin's `white` region. Whole or not at all: the Stage
keeps a `Drawing` only when `loaded`, so every member is always there and
nothing is an `optional` (an earlier version had four, and clang-tidy's
unchecked-optional-access could not see the invariant). The skin directory
is `skins/default/skin` in the app's bundled files (7.4 moves the skin into
the package). The loading is the demo's `Services`, moved.

**Logical units**: the short side is 720 and the long side follows the
drawable's aspect, as in the demo. Pointers are scaled once in `event`; the
safe area is converted per axis. `resize` re-lays out the shown screen
without building it again; a lost context (`resources_preserved()` false)
does build it again.

**Failures** go to the platform's `Report` through `say`, a `printf`-style
member formatting into 256 bytes on the stack. A skin that does not load
leaves no `Drawing` and sets `failed_`; a layout `instantiate` refuses sets
`broken_`; either draws dark red. A layout built short (a custom type or a
drawable missing) is reported and shown.

## Patterns

- **Facade**: `App`, over the Telegraph, the model's runner and the Stage; an
  app sees `stage`, `run` and `quit`. It is also the platform's
  `sq::app::Application` (virtual), which costs one vtable for the process.
- **Command queue (bounded)**: the Stage's waiting navigations, above.
- **Type erasure**: `detail::ModelHolder`, above. Chosen over `std::function`
  (it holds an object, not a call) and over `std::any` (RTTI).
- **Registry (bounded)**: the added screens and the custom makers, 16 each,
  no allocation, filled in `squared_main`; a full table refuses and reports.
- **Static polymorphism**: a C++ screen is any type `sq::ScreenType`
  accepts, reached through one function pointer; no base class.

## Standard library

Kept: `std::unique_ptr` for the runner and the `Drawing` (single owners);
`std::array` for the stack, the queue, the registries and the scratch; `std::string` and `std::vector`
while a skin loads (once per window, in `attach`); `gui::FontPtr`
(`std::shared_ptr`), gui's own type. Passed over: `std::vector` for the
stack and the queue (fixed at 16 and 4, no growth), `std::function` for the
hooks and the report (function pointers). See
[standard-library-deviations.md](standard-library-deviations.md).

## Memory profile

| What | Where | When | Size |
|---|---|---|---|
| `App` | static storage (`.bss`) | first use | `TelegraphLine` (88,000 at the default knobs) + 1,088 (the Stage is 936) |
| the model's runner | heap | `run` | the runner and the model |
| `Stage::Drawing` | heap | each window | 19,128 (libc++; 19,312 libstdc++), the scratch's 17,480 included, plus the skin's textures and tables |
| a C++ screen | read-only data | build time | a name and a function pointer |
| a screen's widgets | heap | each navigation | as `instantiate` makes them |
| a tap's telegram | stack | each tap | 1 KB |
| a report line | stack | each report | 256 bytes |
| a frame | &mdash; | &mdash; | nothing (tested) |

## Tested

`stage/test/stage_test.cpp`, on the null backend, with a copy of the
template's default skin in `test/assets/` and layouts written with
`LayoutWriter`; it defines `layouts::all()` as sqgen would:

- **A start:** `squared_main` runs once, its first `push` shows, a second
  `run` is refused and reported.
- **Taps:** a telegram reaches an inline model and its answer comes back
  through the drain; `push`, `replace`, `back` by tap and by key; a name with
  no layout and a pane are reported and change nothing; the stack's limit.
- **No allocation** in 20 frames, under the frame-allocation counter.
- **Lifecycle:** background, foreground and low memory reach the model.
- **A recreated activity:** the model runs on, `squared_main` does not run
  again, the stack comes back with new widgets.
- **Quitting:** back at the root leaves; the model saves and stops; the next
  start runs `squared_main` with a Telegraph holding nothing; a `quit` tap;
  a `back` tap at the root.
- **A model on its own thread:** a tap's telegram is answered across
  threads; the quit joins it after it saves (and ThreadSanitizer is clean).
- **No skin:** reported, `failed()`, no widgets, the stack kept; a layout
  `check` refuses: reported, nothing built.

**C++ screens (7.2):** a typed tap reaches one by name; `top_name`,
`showing`; a custom node made by its maker and one without (stood in,
reported); no allocation in frames on it; `back` builds the C++ screen again,
then the layout beneath; a name with neither layout nor screen; a screen
bigger than the scratch shows what fit and is reported; a C++ screen as the
root, its Back leaving; names.json's screens checked at start (one not
added); the refused adds (not a name, a layout's name); after a quit the
tables are filled again with nothing said twice.

`stage/test/stage_alone_test.cpp`: an App with no layout files links (the
weak default), shows a C++ layout, and a `push` tap to a name says there is
none.

Planted bugs, 24 (`plant72.py`), each fail a test: navigation not waiting for
a tap, no drain, a tap not sent, pointers not scaled, a `back` or `quit` tap
ignored, no stack limit, `find` taking the first, a failed skin not said or
not marked, no rebuild on `back`, a bad layout not said, `squared_main` every
window, the line not made again, a recreated activity shutting down, a quit
not stopping the model, the Stage's leave not heard, each lifecycle call not
sent, an inline model never turned, reports not reaching the Stage, back by
key staying, and the default catalog not weak.

Planted bugs for 7.2, 12 (`plant73.py`), each fail a test: a C++ screen not
built, an overflow not said, taps not reaching added screens, a bad name or a
layout's name added, custom makers not used, names.json's screens not checked
or all said, a typed tap with no name, `top_name` of a layout only,
`replace<S>` pushing, and sqgen writing no screens.

Not covered here: the template's `entry.cpp` (compiled on the phone; its new
lines were compiled and run on the host against the real `App` and
`Console`), a real GL context's surface loss, and how a screen looks (the
gui `Table` places its grid at the top left: it has no table-wide alignment
yet, noted for 7.9).
