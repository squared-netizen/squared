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

264 bytes (`static_assert`ed): the screens as 16 pointers, 4 waiting changes
(a `TapKind` and a pointer each), the report, the text-input service, the
`Drawing` pointer, the pixel and logical sizes, the safe area, a depth, a
count and four flags.

**The stack holds pointers to layouts**, nothing else (7.1's choice 7): only
the top screen has widgets. `show_top` makes a new root `Table`, hands it to
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

## `Stage::Drawing`

1,648 bytes under libc++ (Android, Termux) and 1,832 under libstdc++
(`static_assert`ed for each: the `Ui` holds standard containers, whose sizes
are the library's), one allocation per window: the atlas, the
font, the sprite batch, the `Ui`, a `loaded` flag, the camera and the
painter, **in that order**, so the constructor can load in its member
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

## Standard library

Kept: `std::unique_ptr` for the runner and the `Drawing` (single owners);
`std::array` for the stack and the queue; `std::string` and `std::vector`
while a skin loads (once per window, in `attach`); `gui::FontPtr`
(`std::shared_ptr`), gui's own type. Passed over: `std::vector` for the
stack and the queue (fixed at 16 and 4, no growth), `std::function` for the
hooks and the report (function pointers). See
[standard-library-deviations.md](standard-library-deviations.md).

## Memory profile

| What | Where | When | Size |
|---|---|---|---|
| `App` | static storage (`.bss`) | first use | `TelegraphLine` (88,000 at the default knobs) + 384 |
| the model's runner | heap | `run` | the runner and the model |
| `Stage::Drawing` | heap | each window | 1,648 (libc++; 1,832 libstdc++), plus the skin's textures and tables |
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

Not covered here: the template's `entry.cpp` (compiled on the phone; its new
lines were compiled and run on the host against the real `App` and
`Console`), a real GL context's surface loss, and how a screen looks (the
gui `Table` places its grid at the top left: it has no table-wide alignment
yet, noted for 7.9).
