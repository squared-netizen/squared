# stage &mdash; internals

Programmer docs: [../programmer/stage.md](../programmer/stage.md). The
decision record: the refactor plan's 7.1 to 7.4 ("yes to all",
2026-10-26) and the app model's Ownership and Lifecycle sections.

`stage/app.hpp` (`sq::App`, `squared_main`), `stage/stage.hpp`
(`sq::Stage`), `src/app.cpp`, `src/stage.cpp`, and `src/catalog.cpp` with
`src/detail/catalog.hpp` (the layouts' default). `squared/squared.hpp` at the
tree's root includes it with gui and messaging. Sprint 7.1, 2026-10-26; 7.2's
C++ screens, 7.3's panes and 7.4's compiled-in skin folded in.

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
| a pane's window and widgets | from opened to closed, or its screen left | `build_pane`, on each open |

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

1,112 bytes (`static_assert`ed; pointers, `string_view`s and arrays of them,
the same in both standard libraries): the screens (16 `Shown`s, two pointers
each), 4 waiting changes (a `TapKind`, a pane slot and a `Shown`), the 16
added C++ screens (a pointer each), the 16 custom types (a name and a
function pointer each), the 4 pane slots (40 bytes each, below), the report,
the text-input service, the skin's file system (a test's; below), the
`Drawing` pointer, the pixel and logical sizes,
the safe area, a depth, four counts and four flags. Was 264 in 7.1 and 936 in
7.2; the pane slots are 7.3's 168, the skin's pointer 7.4's 8. In the App
it costs less than it adds: the App is `TelegraphLine`-aligned (64), so it
grew from line + 1,088 to line + 1,216, and 7.4's 8 bytes fit in its
padding.

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

## Panes (7.3)

**A pane is a layout of kind `pane`**, shown over the top screen in a
`gui::Window` (7.3's choices 1 and 3): `open( layout )` takes a compiled one,
`open<S>()` a C++ screen type, and the `open <name>` tap either. Its shape
(`gui::PaneShape`: a `PanePlace` and `modal`, 2 bytes) is the file's `place`
and `modal`, which sqgen writes as a sixth constructor argument; it lives in
`Layout`'s tail padding, so a layout stays 72 bytes (choice 2). A C++ type's
comes from its optional `place` and `modal` constants: `detail::shape_of<S>()`
is `consteval` and reads them with `if constexpr( requires ... )`, into the
`ScreenEntry` beside the name and the build. `write` gives the writer the
entry's shape, so the table a C++ pane writes carries it the way a compiled
one does, and one path builds both. The kinds are checked at the door:
`push` and `replace` refuse a pane, `open` a screen (sqgen already refuses
them in a file's taps; this is the C++ side).

**Slots, not a stack.** 4 `Pane` slots (`max_panes`, choice 5): the Stage's
address, a `Shown`, the `gui::Window *` while built, its place, and an
`order` (0 free, else 1 for the lowest open pane). The panes' widgets get the
slot's address as their hooks' context (`tapped_in_pane`, `made_in_pane`),
so a `close` tap knows which pane it is in, whichever is on top; a slot keeps
its address while the pane is open, which a compacted array would not
(closing a lower pane would move the upper one under its buttons' contexts).
The top pane is the slot whose `order` is the count. A linear scan of 4 for
each, no allocation. Cost: the Stage pointer in each slot (32 bytes), chosen
over recovering the Stage from a slot's address (`offsetof` on a
non-standard-layout class is only conditionally supported).

**Changes wait like navigation does.** `open` and `close` are `TapKind`s in
the same 4-entry queue, a `close` carrying its slot (`top_pane` for "the top
one"). `after()` replays the queue over a copy of the depth and the pane
count, so `depth()`, `panes()`, `back()` and the four-pane limit answer for
the state after it; a push or replace there counts the panes as gone.

**Built when opened, freed when closed** (choice 4, which changed the spec's
"reused, not rebuilt"): `build_pane` makes a `Window` (no title bar, not
movable, modal as asked), instantiates the layout into its content table and
hands it to `Ui::show_window` without centring; `close_pane` calls
`Ui::close_window`, which frees the tree, then renumbers the slots above.
Keeping a closed pane's widgets would hold memory for something not shown;
priority 1 over the speed of a second open.

**They belong to their screen** (choice 5): a push, a replace or a back to
the screen beneath drops every slot (`drop_panes`) before `show_top`, whose
`Ui::set_content` frees the windows with the old content. They do not come
back with the screen. A window made again (`attach` after `detach`, or a lost
context) is the same screen, so `show_top` builds its panes again, lowest
first; `detach` and `show_top` null the windows first, so no slot points at a
freed one.

**Back closes the top pane first** (choice 6), for the back key and the
`back` tap alike, so `back()` means what the back key does and is false only
at the root with no pane open.

**Modal.** gui's `Ui` blocks everything outside its top modal window, a
window above it included. So a pane opened over a modal one is made modal
too (`build_pane` looks at the open slots below it); otherwise it would show
and take no taps. A tap outside a non-modal pane reaches the screen
(`Window::pointer_event` takes only what is inside it).

**Placement** (`place_panes`, before each `Ui::layout`): the safe area is the
frame; `center` is the pane's preferred size, capped to it and centred in it;
`left` and `right` are its preferred width and the frame's height, `top` and
`bottom` its preferred height and the frame's width. Rotation and safe-area
changes come through `lay_out`, so they place again.

**One scratch.** A C++ pane writes into the same `Drawing::Scratch` as a C++
screen: `instantiate` copies what the widgets keep, so the screen's widgets
are unaffected, but `top()` for a C++ screen points into that scratch and is
documented as valid until a C++ pane is written over it. A second 17 KB
scratch for panes was the alternative: priority 1.

**gui changed for it:** `Window::set_titled( false )` leaves no title bar
(`title_height_` 0: no title text, background or close button, and the
content takes the whole window; `close_bounds` is an empty rectangle so no
tap lands on a close button that is not there). A bool in the existing
padding: `Window` is still 304 bytes. 7.7 folds `Window` and `Dialog` into
the pane.

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
unchecked-optional-access could not see the invariant). The skin is read
from `skins/default/skin` in the compiled-in files (below). The loading is
the demo's `Services`, moved.

## The default skin (7.4)

**Compiled into the stage library** (7.4's choice 1): `src/default_skin.cpp`
holds the four files (`uiskin.atlas`, its page `uiskin.png`, `uiskin.json`,
`default.fnt`: 46,305 bytes of read-only data, and a 128-byte table) as
constant bytes, which `tools/embed_files.sh` writes from
`stage/skin/default/skin/` (choice 3); `detail::default_skin()` returns the
table. The Stage reads them through one `files::EmbeddedFileSystem`, a
function-local static in `stage.cpp`, so the atlas, the font and the skin
loader are unchanged, and the atlas page's texture reloads from the same
bytes after a lost context (choice 2). An app ships no skin, and nothing it
bundles is read for one: `attach` no longer uses `runtime.files`.

Why not assets in the APK (the rejected choice): the package would add a
second assets directory to the template's `aapt2` line and merge the asset
index, a template change for every app, and the skin would still be missing
on desktop Linux and in tests without a copy. Compiled in, it is the same
everywhere, and on priority 1 it is better: the bytes are clean pages mapped
from the `.so`, which the kernel can drop and read again, where an asset is
read into the heap first. A read still copies each file into a vector while
it loads, as before; that is once a window.

**The files** live at `stage/skin/default/` with the skin's sources (`raw/`,
26 nine-patches and images it is packed from) and its previews, which are not
compiled in, `NOTICE.md` (choice 4) and `LICENSE-2.0.txt`, the Apache
License that a redistribution carries a copy of. One skin (choice 5): the template
and hello no longer carry `sq_android/assets/skins/`; the demo keeps its own
copy until it moves to `sq::App` (choice 6). An app's own skin is in the
backlog.

**The test seam.** `skin_files_` is nullptr (the compiled-in files) unless a
test sets it, through `detail::StageProbe`, a friend declared here and
defined only by a test: `stage_test` points it at an empty
`EmbeddedFileSystem` to see a skin fail. 8 bytes, chosen over losing the
failure test.

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
  accepts, reached through one function pointer; no base class. Its pane
  shape is read at compile time from optional constants.
- **Object pool (fixed slots)**: the 4 pane slots, whose addresses are the
  panes' hook contexts; free is `order == 0`.

## Standard library

Kept: `std::unique_ptr` for the runner and the `Drawing` (single owners);
`std::array` for the stack, the queue, the registries, the pane slots and the scratch; `std::string` and `std::vector`
while a skin loads (once per window, in `attach`); `gui::FontPtr`
(`std::shared_ptr`), gui's own type. Passed over: `std::vector` for the
stack and the queue (fixed at 16 and 4, no growth), `std::function` for the
hooks and the report (function pointers). See
[standard-library-deviations.md](standard-library-deviations.md).

## Memory profile

| What | Where | When | Size |
|---|---|---|---|
| `App` | static storage (`.bss`) | first use | `TelegraphLine` (88,000 at the default knobs) + 1,216 (the Stage is 1,112) |
| the default skin's files | read-only data, mapped from the `.so` | load time | 46,305 bytes, and its 128-byte table |
| the model's runner | heap | `run` | the runner and the model |
| `Stage::Drawing` | heap | each window | 19,128 (libc++; 19,312 libstdc++), the scratch's 17,480 included, plus the skin's textures and tables |
| a C++ screen | read-only data | build time | a name and a function pointer |
| a screen's widgets | heap | each navigation | as `instantiate` makes them |
| a pane | heap | each open | a `gui::Window` (304) and its widgets; freed when it closes |
| a tap's telegram | stack | each tap | 1 KB |
| a report line | stack | each report | 256 bytes |
| a frame | &mdash; | &mdash; | nothing (tested) |

## Tested

`stage/test/stage_test.cpp`, on the null backend, with the compiled-in skin
(the app's bundled files are a directory that does not exist) and layouts
written with `LayoutWriter`; it defines `layouts::all()` as sqgen would:

- **The skin is its files:** every compiled-in file equals its file in
  `stage/skin/default/skin/`, byte for byte (the files are the test's
  prerequisites, so changing one runs it again).

- **A start:** `squared_main` runs once, its first `push` shows, a second
  `run` is refused and reported.
- **Taps:** a telegram reaches an inline model and its answer comes back
  through the drain; `push`, `replace`, `back` by tap and by key; a name with
  no layout is reported and changes nothing; the stack's limit.
- **No allocation** in 20 frames, under the frame-allocation counter.
- **Lifecycle:** background, foreground and low memory reach the model.
- **A recreated activity:** the model runs on, `squared_main` does not run
  again, the stack comes back with new widgets.
- **Quitting:** back at the root leaves; the model saves and stops; the next
  start runs `squared_main` with a Telegraph holding nothing; a `quit` tap;
  a `back` tap at the root.
- **A model on its own thread:** a tap's telegram is answered across
  threads; the quit joins it after it saves (and ThreadSanitizer is clean).
- **No skin** (an empty file system, through the probe): reported,
  `failed()`, no widgets, the stack kept; a layout
  `check` refuses: reported, nothing built.

**C++ screens (7.2):** a typed tap reaches one by name; `top_name`,
`showing`; a custom node made by its maker and one without (stood in,
reported); no allocation in frames on it; `back` builds the C++ screen again,
then the layout beneath; a name with neither layout nor screen; a screen
bigger than the scratch shows what fit and is reported; a C++ screen as the
root, its Back leaving; names.json's screens checked at start (one not
added); the refused adds (not a name, a layout's name); after a quit the
tables are filled again with nothing said twice.

**Panes (7.3):** a tap opens one over the screen, which stays; modal, at the
bottom, the screen's width, no title bar (the content above the 36 units a
bar takes); a modal pane keeps taps from the screen; a pane opens another,
modal over a modal one, on the right at the screen's height; back closes the
top pane first; a `close` closes the pane it is in, the top one or not; a tap
outside a non-modal pane reaches the screen, and leaving the screen closes
its panes, which do not come back; a C++ pane by type (on the left, from its
constant) and by name (an added screen); a pane's tap navigating the screen;
`open` a screen, `push` and `replace` a pane, `close` with none: refused and
said; four at most; no allocation in frames with four open; a window made
again builds the four again; back closes them one by one, then leaves. Two
panes opened from inside an event (a custom widget's) wait, and `panes()`
counts them while they do.

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

Planted bugs for 7.3, 27 (`plant74.py`), 26 caught: back skipping the panes,
a screen left keeping them, a pane's `close` closing the top one, modal never
set or not passed up, panes not placed, right placed left, bottom placed top,
sides not full height, panes not rebuilt, four not enforced, a screen opened
as a pane, a pane pushed, a waiting open not counted, a title bar kept or
never cleared, sqgen dropping `modal`, a file's `place` or `modal` ignored, a
screen taking `place`, the writer dropping the shape, a C++ pane's shape
ignored, `shape_of` dropping `place` (a `static_assert`), `pane_name` of a
layout only, slots not renumbered, the `open` tap missing C++ panes. The one
not caught is equivalent: `detach` not nulling the windows, which nothing
reads before `show_top` nulls them again; kept as hygiene.

Not covered here: the template's `entry.cpp` (compiled on the phone; its new
lines were compiled and run on the host against the real `App` and
`Console`), a real GL context's surface loss, and how a screen looks (the
gui `Table` places its grid at the top left: it has no table-wide alignment
yet, noted for 7.9).
