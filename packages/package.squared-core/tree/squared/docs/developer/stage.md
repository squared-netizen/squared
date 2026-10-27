# stage &mdash; internals

Programmer docs: [../programmer/stage.md](../programmer/stage.md). The
decision record: the refactor plan's 7.1 to 7.6 ("yes to all",
2026-10-26; 7.6's four picks, 2026-10-27) and the app model's Ownership and
Lifecycle sections.

`stage/app.hpp` (`sq::App`, `squared_main`), `stage/stage.hpp`
(`sq::Stage`), `stage/save.hpp` (`sq::Save`, `detail::Saving`), `src/app.cpp`,
`src/stage.cpp`, `src/save.cpp`, and `src/catalog.cpp` with
`src/detail/catalog.hpp` (the layouts' default). `squared/squared.hpp` at the
tree's root includes it with gui and messaging. Sprint 7.1, 2026-10-26; 7.2's
C++ screens, 7.3's panes, 7.4's compiled-in skin and 7.5's bound values and
lists folded in.

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
| what each screen and pane keeps (`Stage::Kept`) | inside the Stage, at its depth or in its pane slot | `keep_shown`, when it is left or loses its widgets |
| the stack, for Android (`squared app 1`, then the Stage's lines) | Android's saved instance state, from its save to the activity it makes again | `App::save_state`, asked by the platform |
| `squared/model.state` | the app's private files, from a save to the next | the model's thread (`detail::Saving`) |
| the model's last save, read | from `run` to the model's `start` | `App::model_file` |

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
detaches the window's half; the model runs on. **A new launch with a run
held** (7.8): a `create()` given no saved state while `started_` is set is a
new launch in a live process (a swipe from the recent apps that left the
process alive): `shut_down` first, as a quit does, then the first-start path.
The model saved when the app left the screen, so its quit saves nothing
more. This is why the App always gives Android at least its first line,
`squared app 1`, even with `keep_screens( false )` or nothing to keep: an
activity Android makes again (a configuration it does not handle, "Don't
keep activities") is then told from a new launch.

**What the activity handed over goes with it.** The text-input service and
the report are the platform's, per activity (the template's `Platform`
holds both, and its report's context is the `Platform`). `dispose()`, quit
or not, sets both back to none: the next activity hands its own, after its
`create()`. Found on the phone (pkg57): quit, then opened again from the
recent apps, the process lived on, and the new window's first layout asked
the destroyed activity's keyboard `active()`, a call through a freed vtable
(SIGSEGV at `0x28`, the slot of `active`). The template hands the service
over after `create()`, so a dangling one was used for the whole of a
`create()`. Tested with a keyboard that fails any call once its activity is
gone.

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

4,056 bytes (`static_assert`ed; pointers, `string_view`s and arrays of them,
the same in both standard libraries): the screens (16 `Shown`s, two pointers
each), 4 waiting changes (a `TapKind`, a pane slot and a `Shown`), the 16
added C++ screens (a pointer each), the 16 custom types (a name and a
function pointer each), the 4 pane slots (192 bytes each, below, 148 of them
what the pane keeps), what the 16 screens keep (148 bytes each), the report,
the text-input service, the skin's file system (a test's; below), the
`Drawing` pointer, the pixel and logical sizes,
the safe area, a depth, four counts and eight flags. `ui()`, the window's
`gui::Ui`, is private since 7.7 (the Ui is internal); the tests reach it
through `detail::StageProbe`. Was 264 in 7.1 and 936 in
7.2; the pane slots are 7.3's 168, the skin's pointer 7.4's 8; 7.5a's two
flags fit in padding; 7.6's kept state is 2,960 (16 screens and 4 panes at
148), the user's pick ("scrolls and typed text") over 520 for scrolls
alone. In the App, with its two paths (7.6), line + 4,224 under libstdc++
(`static_assert`ed as at most that: libc++'s strings are 8 bytes smaller,
and the App is `TelegraphLine`-aligned, 64).

**The stack holds a layout or a C++ screen per entry**, nothing else (7.1's
choice 7, 7.2's choice 3): `Shown` is a `const Layout *` and a
`const detail::ScreenEntry *`, one of them set. Only the top screen has
widgets. `show_top` makes a new root `Table`, hands it to
the `Ui` (which frees the old tree), and runs `instantiate` with the Stage's
hooks: `tapped` is a static member taking the Stage as context. So `back`
rebuilds the screen beneath from its layout (and puts back what it kept,
below), and 7.6's "release hidden screens under memory pressure" has
nothing left to release. Cost: a
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
`gui::Window` (7.3's choices 1 and 3; a `gui::PaneFrame` since 7.7): `open( layout )` takes a compiled one,
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
address, a `Shown`, the `gui::PaneFrame *` while built, its place, and an
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
"reused, not rebuilt"): `build_pane` makes a `gui::PaneFrame` (the skin's
`pane` style, modal as asked; a `Window` with no title bar until 7.7),
instantiates the layout into its content table and hands it to
`Ui::show_pane`; `close_pane` calls `Ui::close_pane`, which frees the tree,
then renumbers the slots above.
Keeping a closed pane's widgets would hold memory for something not shown;
priority 1 over the speed of a second open.

**They belong to their screen** (choice 5): a push, a replace or a back to
the screen beneath drops every slot (`drop_panes`) before `show_top`, whose
`Ui::set_content` frees the frames with the old content. They do not come
back with the screen. A window made again (`attach` after `detach`, or a lost
context) is the same screen, so `show_top` builds its panes again, lowest
first; `detach` and `show_top` null the windows first, so no slot points at a
freed one.

**Back closes the top pane first** (choice 6), for the back key and the
`back` tap alike, so `back()` means what the back key does and is false only
at the root with no pane open.

**Modal.** gui's `Ui` blocks everything outside its top modal pane, a pane
above it included. So a pane opened over a modal one is made modal
too (`build_pane` looks at the open slots below it); otherwise it would show
and take no taps. A tap outside a non-modal pane reaches the screen
(`PaneFrame::pointer_event` takes only what is inside it).

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

**gui changed for it:** in 7.3 `Window::set_titled( false )` left no title
bar; in 7.7 `Window` went, and `Dialog` with it: a pane's frame is
`gui::PaneFrame` (168 bytes, internal), drawn with the skin's own `pane`
window style. gui's developer page has the fold.

## Bound values (7.5a)

A node's `bind` carries its value type and whether it sends `set`
([layouts.md](layouts.md#layoutnode-32-bytes)); the Stage does the rest.

**The table.** `Drawing::bindings`, 128 `Binding`s (`max_bindings`) of 32
bytes, after the scratch: the `Address`, the widget, a pending slider value,
the node and value types, the owner (a pane's slot, or `screen_owner`
0xFF), and three bits (`sends_set`, `sending`, `stale`). A free slot has no
widget; the scans are linear over 128, which the frames do twice (one pass
in `send_edits`, one per state that comes). In the `Drawing`, not the Stage,
since widgets exist only with a window: 4 KB a window, freed with it.
Priority 1 over a growing vector: the size is known before the first
screen. Rejected: a pointer in each widget (every widget pays for the few
that are bound, and gui would learn of the Telegraph).

**Bound as built.** `instantiate`'s `bound` hook (`bound` for the screen,
`bound_in_pane` for a pane, its slot the owner) takes a free slot, calls
`TelegraphUi::bind` so the cache keeps the value while it is shown, wires
the widget's callback when the node sends `set`, and shows the cached value
if there is one: a pane opened later shows the latest at once. A full table
says so once (until the next `unbind_all`) and the rest show their own
text. `show_top` and `detach` unbind all (the old content's widgets go);
`close_pane` unbinds its owner's.

**Shown as it comes** (7.5's choice: no per-frame poll). `update` drains
with a local handler whose `state` calls `changed`, which finds the bound
slots for the address and shows the cached value (`find`), or marks the slot
`stale` while the user holds the widget (`held`: a slider `dragging()`, a
text field `focused()`). `send_edits`, after the drain, shows a stale slot
once it is let go. `show` refuses a telegram of another type than the node
was bound as, which only a model whose key disagrees with `names.json` can
send, and sets the widget with `applying_` up, so its change callback does
not send the value back.

**What the user does goes out** (`edited`): a check box at once
(`Telegram::boolean` set); a slider's moves into `pending`, `sending` set,
and `send_edits` sends the last once a frame (an integer key gets
`lround`); a text field on commit (Enter, or the focus lost after an edit),
its text through a 1 KB `TelegramData` on the stack. A full ring is
reported; the widget keeps what the user did, and the model's next state
corrects it.

**Text** shown on a label or a field is a `string_view`: a text value's
cached bytes as they are, anything else written by `messaging::write_value`
into 128 bytes on the stack (four floats is the longest). `set_text` takes a
view and assigns it into the widget's `std::string`, which keeps its
capacity, so a value shown again allocates only when it is longer than any
the widget held (tested past the short-string size). 7.5a changed both
widgets' `set_text` from a `std::string` by value, which made every caller
build a string first.

**Laid out each frame, when something changed** (7.5b; a 7.5a bug the list
found): `render` calls `Ui::layout` before it paints, which validates only
what was invalidated. Before, layout ran only on a navigation, a resize or a
pane, so a bound label's longer text kept its first width (clipped) until
then. A frame where nothing changed lays out nothing. That made a `Table`'s
layout a per-frame path, which allocated six `std::vector`s each time; it
now borrows kept ones (gui's developer page).

## Bound lists (7.5b)

Decided 2026-10-26 ("yes to all"): the row is a node of the layout (not the
spec's C++ row builder), uids in rows, one height, the rows on screen and one
more reused, the asked-for window kept in the cache, drag to scroll, one
level.

**Built.** gui's `instantiate` makes a `gui::List`, which copies its row (the
row's nodes, and the layout's words and numbers, so a C++ screen's scratch
may be written over), and calls the new `listed` hook. The Stage's `listed`
(`listed_in_pane` for a pane) takes one of 4 `ListBinding` slots
(`Drawing::lists`), gives the list its row hooks (`tapped_in_row`,
`tapped_row_uid`, `bound_in_row`, `made_in_row`, with the row's table as
context and the binding as the list's `owner()`), tells it to call
`row_shown`, binds the count in the cache, reads it if cached, and sends a
reset (`TelegraphUi::reset_view`). A second widget of a list already shown,
or a fifth list, gets empty hooks and stays empty, reported: the model keeps
one view of a list (`ModelList`), so two widgets would fight over it.

**Rows.** The list builds row tables as it lays out; each bound node in a row
is a `Binding` with `in_row`, `list` (2 bits) and `owner` = the row table's
slot, and `row_uid` when its address takes the row's uid; the uid is 0 (not
an address: not bound in the cache, nothing sent) until the row has an item.
`row_shown`, and a row's state as it arrives, call `show_row`: the uid of the
row's item from the cache (`{rows, row}`), and each of the row's bindings
moved to it (the old address unbound, the new bound, shown from the cache or
blanked). A binding's widget never changes, so edits, held widgets and stale
values work as for any binding, at the item's address. A row's tap whose
telegram takes the row's uid comes to `tapped_row_uid` (gui calls `tapped_row`
for nodes with `row_uid`), which sends it with `Telegram::with_uid`; its
other taps go where the screen's or pane's would.

**What the cache keeps.** The model sends a row's item when the row comes
into its view, and an item's values once, when it comes in; so the UI must
keep them while they are in the view it asked for, or a scroll back would
find them evicted and nothing would come again. Each frame, `ask_views`
gives each list's `messaging::ListWindow` the rows on screen; when it asks,
the rows no longer asked for are unbound with their items' values, the new
ones are bound (so their answers have room), and the `view` is sent (again
next frame if the ring was full). A row's state for a row asked for keeps the
new item's values (`keep_item`, one bind per value the row shows, from the
row's nodes: `max_row_values`, 8) before it lets the old one's go, so an item
that only moved rows is never let go. `kept` holds the item per row asked
for, at the row modulo its size (no two rows of one window share a place).
The window is sized when the list is built (`size_lists`, after the
screen's or pane's widgets are made, 7.8: until then the first frame with a
height made it, the 340 bytes hello's Console showed): at most half the
cache over the lists shown, divided by one plus the row's values, clamped to
`TELEGRAPH_LIST_VIEW`; if that is fewer rows than the screen shows, it is
said once.
`drop_list` lets all of it go (leaving a screen, closing a pane, `detach`).

**The reset** (a protocol addition, 7.5b): a new widget sends `do
<list>.view (1 0)`, last before first, which `ModelList::view` takes as
"forget what you sent" (its window emptied) and answers with the count. Without
it, a list shown again would get nothing for the rows the model still
thought it had sent, though the cache may long have let them go; and a count
let go would leave the list empty for good.

**gui's `List`** is in gui's developer page: row `r` is in table `r` modulo
the tables, so a row keeps its table while it stays on screen; a drag is
watched in the capture phase, so a finger on a row's button scrolls once it
moves `drag_slop`, and the button gets a cancel.

## What is kept (7.6)

Decided 2026-10-27, the user's four picks: (1) a screen keeps its scrolls
and an unfinished edit; (2) the stack and its panes are saved in
`background()` and shown again at the next start; (3) a model's optional
`save( sq::Save & )`, given back as sets before `start`; (4) the Console's
`screens` and `screens low-memory`.

**`Stage::Kept`** (148 bytes, `static_assert`ed): the first
`kept_scrolls` (4) scroll offsets, in the layout's order, and the first
text field with an unfinished edit: its index among the screen's text
fields (`no_field`, 0xFF, for none) and up to `kept_text` (128) bytes of its
text, cut where a UTF-8 character starts. One per depth (`kept_`) and one
per pane slot (`Pane::kept`). A **memento**, held by the Stage, not the
widgets.

**`walk`** visits a widget tree in document order (the `Group` children's
order, which is the layout's) and calls the visitor's `list`, `pane` or
`field`: `gui::List` and `gui::TextField` end the walk there (a list's rows
are its own, and change as it scrolls); a `ScrollPane` is visited and walked
into. Widgets are found with `gui::actor_cast`, so `ScrollPane` and
`TextField` gained interface ids (7.6; the List had one). `Keeping` and
`Putting` are the two visitors, templates on the kept type so the private
`Kept` is named only inside the Stage's members.

**When it is kept** (`keep_shown`, the top screen and every built pane):
before a push covers the top; in `detach` (the window going); in `attach`
when a window is made again without a detach; before a lost context's
`show_top`; and before the background's save. **When it is cleared**: the
slot a push fills, the screen a replace puts in, the screen `back` pops, a
pane opened, closed or dropped, and `clear()`. **When it is put back**: at the
end of `show_top`, after a first layout (a scroll pane clamps to its
content's height, which the layout gives it), then laid out again for what
moved. A list does not clamp there: `List::restore_scroll` keeps the offset
as `wanted_` and each layout scrolls as far as the rows reach, until they
reach it (the model sends the count again after a process ends); any scroll
since forgets it.

**An edit put back is held.** `Stage::held` counts a text field with
`edited()` as held, focused or not, so the model's value waits (`stale`)
instead of replacing the user's text; Done or leaving the field commits it
as before, and `stale` shows the latest after. `TextField::restore_edit` sets
the text and `edited_`; `edited()` is new and public.

**The saved stack** is text, one line a screen from the root and then one
a pane from the lowest: `squared screens 1`, then `screen <name> <n>
<offset>... <field> <edit>` (`pane` likewise), offsets as whole units, the
edit as `x` and hex (`x` alone for an empty edit), `- -` for none.
`Stage::save` writes it into the App's 8 KB with `LineWriter` (no
allocation, `std::to_chars`); every name must find the same screen again
through `resolve` (a layout file, or an added C++ screen), or nothing is
saved and the reason said. `Stage::restore` parses all of it into locals
first (`take_word`, `std::from_chars` with bounds, the kinds checked: a
screen's layout a screen, a pane's a pane; screens before panes; at least
one screen), and replaces the stack only when every line read: else it says
the line and why, and `squared_main`'s stack stays. Then `show_top` builds
the top and the panes, and puts back what they keep.

**The App's part.** The stack goes to Android, not to a file (7.8; 7.6 and
7.7 wrote `squared/screens`, which `find_saves` now removes once at start).
`save_state` writes `squared app 1` and, unless `keep_screens( false )`, the
Stage's lines after it, into the platform's buffer; nothing during a quit, so
a quit's next launch is fresh. `restore_screens`, after `squared_main`, takes
the bytes `set_saved_state` gave: none is a new launch; a first line that is
not the App's is said and passed over; the rest goes to `Stage::restore`.
Android shows a kept stack only once by itself: it keeps the state of the
activity it brings back, and that activity gives it the stack again at its
own save. At the first start, `find_saves` makes the app's private directory
`squared/` (`FileSystem::local( ... ).make_directories()`) and keeps the
platform's path of `squared/model.state` (`FileSystem::resolve_platform_path`,
7.6), written and read through a `files::PosixFileSystem` with no roots,
made on the stack each time, with `FileType::Absolute`: the activity's file
system dies with the activity, and the model's thread writes after it may
have gone. `background()` gives an inline model its turn at once (it saves
before the app stops; a threaded model wakes for it).

**Back at the root (7.8)** sets `to_background_` in the App (the back key,
which no widget and no pane took) or `background_` in the Stage (a `back`
tap that found nothing to go back to); `take_background_request` takes
either, once. Only `quit()`, a `quit` tap or the platform's `QuitRequested`
quit.

**The model's save: `detail::Saving<M>`**, a decorator the App runs instead
of `M` when `M` has `save( sq::Save & )` (`SavingModel`); any other model
runs as before, with nothing added. It owns a `ModelFile` (the path, the
last save's bytes, a pointer to the App's `save_failed_`) and `M`, and
forwards each hook `M` has: `tick`, `low_memory`, `foreground`, `wake_at`
and `due` are members constrained on `M` having them, so the Telegraph's
and the runner's `requires` checks see exactly `M`'s set. `start` gives the
last save back first (`give_back`: each line read with
`messaging::read_telegram`, only `set`s, as a `Received` with its text or
uids from a 1 KB `TelegramData`), frees it, then calls `M::start`.
`background` calls `M::background`, then `M::save` into 8 KB on the model
thread's stack, then `write_save` (atomic, as above); a full save or a
failed write sets `save_failed_`, which `foreground` reports (the model's
thread has no report). `run<M>` makes it with the tag first when it is
`on_ui_thread` (`saving_on_ui_thread`), since `ModelRunner` takes the tag
second.

**`sq::Save`** writes each value as `write_telegram` does, with the key's
own text registered in a one-name `NameTable` (256 bytes on the stack), so
the line reads as the key is written. `state` is constrained on the key
carrying `set` (`messaging::sendable<K, Verb::set, V>`): a key the model only
states does not compile, since the value would come back as a `set` it does
not take.

**A rotation lays out once.** `resize` and `set_safe_area` no longer lay
out: they set `layout_due_`, and `settle()` lays out what waits at the
first of the next event (`event`, so a finger lands where the widgets
are), `update` (so a list's height is known before it asks for rows) or
`render`. The platform calls `resize` and `safe_area_changed` one after the
other on a rotation; before, each laid out, with the old insets for the
first (the refactor notes' "three layouts in one frame"). Navigation still
lays out at once (`show_top`, `open_pane`): a test, and the next tap, find
the new screen placed.

**Low memory.** `Stage::low_memory` frees gui's kept table working sizes
(`gui::Table::release_kept`, which drops the process-wide `Lines` pool when
none is borrowed); the App then passes it to the model. Screens beneath the
top hold nothing already.

**The Console's `screens`** is the template's (`entry.cpp`): it calls the
public `Stage::describe( Report )` (it keeps the shown screen first, so the
lines are what a background would write) and, for `screens low-memory`,
`App::low_memory()`, as Android's `onLowMemory` reaches it.

## `Stage::Drawing`

23,864 bytes under libc++ (Android, Termux) and 24,048 under libstdc++:
1,648 and 1,832 for the window's own pieces (`static_assert`ed for each: the
`Ui` holds standard containers, whose sizes are the library's), the
17,480-byte scratch, the 4,096-byte table of bound values and the 4 list
slots (160 bytes each: the list, its names, its `ListWindow`, the rows
asked for, the kept items' vector, the 8 value names), last, so they add
exactly. One allocation per
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

**The files** live at `stage/skin/default/`: `skin/`, the four compiled in,
`NOTICE.md` (choice 4) and `LICENSE-2.0.txt`, the Apache License that a
redistribution carries a copy of. Only those: the images the atlas is
packed from (`raw/`) and the previews came along at first and were taken
out on the user's word (pkg55); they are in libGDX's repository. One skin (choice 5): the template
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
safe area is converted per axis. `resize` lays the shown screen out again
without building it, once with the safe area that follows (above); a lost
context (`resources_preserved()` false) does build it again, and puts back
what it kept.

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
  panes' hook contexts; free is `order == 0`. The 128 bound-value slots;
  free is no widget. The 4 list slots; free is no list.
- **Flyweight**: a list's rows share one copy of the row's nodes, built once
  for each table on screen; the tables are reused as it scrolls.
- **Observer (low frequency)**: the drain's `state` hook tells the Stage a
  value changed, through a compile-time handler, not a list of callbacks.
- **Memento**: `Stage::Kept`, what a screen's widgets were, kept by the Stage
  while they do not exist; and the saved stack, the same in a file. 148
  bytes a screen or pane, no indirection.
- **Decorator (static)**: `detail::Saving<M>` wraps a model that saves,
  forwarding the hooks it has, adding the give-back and the save; a model
  without `save` is not wrapped. No virtual: the runner holds it by value.
- **Visitor (static)**: `walk` with `Keeping` and `Putting`, resolved at
  compile time.

## Standard library

Kept: `std::unique_ptr` for the runner and the `Drawing` (single owners);
`std::array` for the stack, the queue, the registries, the pane slots, the scratch, the bound values and the lists; `std::string` and `std::vector`
while a skin loads (once per window, in `attach`); a `std::vector` of the
items kept per list (sized once, when the list is built, 4 bytes
a row asked for); `gui::FontPtr`
(`std::shared_ptr`), gui's own type; one `std::string` in the App, the
platform's path of the model's save (once, at start; a fixed array would
hold 256 bytes resident, the string about 50 on the heap); a `std::span`
of what Android kept, during `create()` only; the
`std::vector<std::byte>` of the model's last save, from `run` to the
model's `start`; `std::atomic<bool>` for a failed save, written on the
model's thread, read on the UI thread; `std::to_chars` and `std::from_chars`
for the saved stack. Passed over: `std::vector` for the
stack and the queue (fixed at 16 and 4, no growth), `std::function` for the
hooks, the report and the text field's commit (function pointers). The
check box's and the slider's change callbacks are gui's `std::function`s,
kept: the Stage's lambdas capture two pointers, stored in place by both
libraries, so binding allocates nothing for them. See
[standard-library-deviations.md](standard-library-deviations.md).

## Memory profile

| What | Where | When | Size |
|---|---|---|---|
| `App` | static storage (`.bss`) | first use | `TelegraphLine` (88,000 at the default knobs) + 4,224 (the Stage is 4,056, 2,960 of it what screens and panes keep) |
| the path | heap | first start | the platform's path of `squared/model.state` |
| the saved stack | the platform's stack, then Android's | each save instance state | 8 KB on the glue thread's stack; a `malloc` copy of what was written (a line a screen, tens of bytes) that the glue hands Android |
| a model's save | the model thread's stack, then the file | each background, after `background` | 8 KB (`Save::capacity`) |
| the model's last save | heap | `run`, freed at the model's `start` | the file's size |
| the default skin's files | read-only data, mapped from the `.so` | load time | 46,305 bytes, and its 128-byte table |
| the model's runner | heap | `run` | the runner and the model |
| `Stage::Drawing` | heap | each window | 23,864 (libc++; 24,048 libstdc++), the scratch's 17,480, the bound values' 4,096 and the lists' 640 included, plus the skin's textures and tables |
| a list shown | heap | each list built | its `gui::List` (384 with libstdc++), a copy of its row's nodes (32 bytes each), the layout's words and numbers; its row tables and their widgets, for the rows on screen and one more, as it is first laid out; 4 bytes a row it asks for, once |
| a C++ screen | read-only data | build time | a name and a function pointer |
| a screen's widgets | heap | each navigation | as `instantiate` makes them |
| a pane | heap | each open | a `gui::PaneFrame` (168) and its widgets; freed when it closes |
| a tap's telegram | stack | each tap | 1 KB |
| a text field's edit | stack | each commit | 1 KB |
| a bound label's text | stack, then the label's string | each value shown | 128 bytes; the string grows only for a longer text |
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
- **Back at the root** sends the app to the background (7.8): not a quit,
  taken once; the back key and a `back` tap at the root alike.
- **Quitting:** `quit()`; Android is given nothing; the model saves and
  stops; the next start runs `squared_main` with a Telegraph holding
  nothing; a `quit` tap.
- **A swipe starts fresh (7.8):** an activity given nothing, the process
  alive: `squared_main` again and a new model, at the first screen, the old
  one stopped with nothing more to save; an activity given what the App
  gave Android keeps the run, and so does the first line alone
  (`keep_screens( false )`).
- **Frames after a build allocate nothing (7.8):** a list of 1,000 shown,
  30 frames under the counter (its buffer sized when built; the painter's
  text layout made with room for 64 glyphs).
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

**Bound lists (7.5b):** a C++ screen of 1,000 books (a title, a count, a
check box and a tap a row): built as laid out, the rows on screen and one
more, blank until the model says; the count from the cache; the reset, the
view and the rows shown; a row's tap with a small move goes to its book, a
row's check box sets its book's value; scrolled to row 500: blank before
the answer, a tap then not sent and said, then the right books in the same
tables; a full cache (600 values from the model) lets go of the rest and
keeps the rows asked for; a row keeps its table while on screen; the cache
keeps no more than half of itself for it; a drag on a row's button scrolls
exactly as far, releases the button and taps nothing; a drag on a title
scrolls once; a sort shows the books now there; 40 frames scrolling to new
rows allocate nothing; a second widget of the list stays empty and is
said; leaving lets all of it go (the cache's bound count as before); after
a flood, the list shown again gets all of it again; cut to three: the
count, the scroll back, the rows past it empty.

**Bound values (7.5a):** seven widgets on five addresses (one shown
three times): each shows its own text until the model's first states, then
the values; a check box's tap sends `set`, the answer shows and is not sent
back; a held slider sends one `set` a frame, the last, while a value the
model states meanwhile shows on the label and the bar but waits on the
slider until it is let go; a text field sends on Done, not before, keeps the
model's value off itself while focused, shows it when left, and sends when
left after an edit; a slider bound to an integer sends a whole number; a
value of another type than bound is not shown; no allocation in frames; a
text shown again allocates nothing when no longer than the last; a
pane's bound label shows the cached value at once and closing it unbinds
it; leaving the screen unbinds all; 130 bound labels: 128 bound, the rest
said once.

**What is kept (7.6):** a rotation's size and insets wait for the next
frame, are laid out once together, and build nothing (the same tables, the
list wider and shorter, placed past the inset); an event lays out what
waits first. A C++ screen with a bound field, a scroll pane and a second
field: pushed over and back, the scroll and the second field's edit are put
back, the edit held and not focused; the first field's edit is kept instead
when it has one, and the model's value waits until Done, which sends it; an
edit over 128 bytes keeps 127 (a two-byte character across the cut); a lost
context and an activity made again keep the scroll and the edit, a pane's
edit too; a list's place comes back, and a screen pushed after a back starts
at 0; `describe`'s lines; low memory frees the tables' working sizes, which
come back with the next layout, and reaches the model. The stack given to
Android, line for line after `squared app 1` (a screen's scroll and edit, a
list's offset, two panes, one with an edit); 7.7's `squared/screens` left
in the private files removed at start and not shown; the model's save in
its file (a uid key, text quoted), at once for an inline model; a new
process (a new App in the same storage) given those bytes shows the stack
again, panes and edits; the
list scrolls to its place once the count comes; the model has its values
back before `start`, its text included, and a `state` line or a line that
is not one in the file is passed over; a save the model cannot write
(a directory in the way) is said when the app comes back; a quit gives
Android nothing, the model's file stays; kept bytes naming a screen there
is not, another form, a pane first, a screen where a pane was, five
scrolls, odd hex, more on the line, a pane before a screen, a negative
scroll, a first line not the App's: each said, the stack `squared_main`'s;
`keep_screens( false )`: kept bytes are not shown, and Android is given the
first line alone; a layout not in `layouts::all()` on the stack: the first
line alone, and said. Quit, then a new activity: the old keyboard is never called
(the phone's crash, pkg57). A state-only key cannot be saved
(`static_assert`).

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

Planted bugs for 7.5a, 37 (`plant76.py`), each fail a test: in the Stage,
the type ignored, a held slider or a focused field not held, shown values
sent back, a stale value never shown, an integer sent as a number, a slider
sending every move, a text never sent, a pane keeping its bindings, an old
screen staying bound, a full table not said, a check box sending nothing, a
new binding not showing the cached value, the cache not kept; in the text
field, Enter not committing, leaving not committing, typing not an edit,
leaving always committing, its text copied into a new string; in messaging, the state hook not called or called
before caching, `write_value` writing for no value; in gui, a label showing
uids, a check box an integer, a slider text, the check skipping the type or
allowing a set unbound or on a label, the writer dropping the type, setting
on a label, never setting, taking a wrong type, a key's verbs ignored; a
file never setting or dropping the type; sqgen dropping `sends_set` or the
value. Five were missed at first (the type, an integer slider, leaving with
an edit, typing, a set with nothing bound); the tests above were added for
them.

Planted bugs for 7.5b, 42 (`plant77.py`), 41 caught: in the list, rows moved
to other tables every scroll, rows past the count shown, no slop, a drag
keeping the button pressed, the list's own drags seen twice, moves after a
drag not stopped, a scroll not kept inside, a layout not keeping it, rows
not told, a table short, the row's context not its table, a count that lays
out nothing; in the Stage, no reset, items not kept, rows leaving kept, the
count kept after, rows not rebound, a row's state not shown, a row's tap
without its uid, two widgets of one list, no blank, nothing laid out per
frame, the count ignored, new rows not kept; in gui, a borrowed `Lines`
never returned, `listed` not called, the list node bound as a value, row taps
to `tapped`, the check allowing a uid anywhere, two rows, nested lists or a
list of text, the writer taking a row anywhere, a list in a row or dropping
`row_uid`; in layout files, any uid in a row, a list of anything; sqgen
dropping `row_uid`; in messaging, the reset keeping what was sent or sending
no count, and the UI's reset sent as a view. Eight got through at first; the
tests above were added for seven. The one left: the window's cap at half
the cache made four times larger, which the test's list (a 20-row screen, a
row of three values: 240 entries in a window of three screens) never reaches.

Planted bugs for 7.6, 50 (`plant78.py`), 48 caught: in the Stage, a push
keeping nothing, nothing put back, `detach` or a lost context keeping
nothing, a pane not kept or not put back, a list or a scroll pane not
visited, any field kept, the last field kept, a cut inside a character, a
field's index off by one, no layout before an event, the safe area laid
out at once, the layout never settling, an edit not held, a save skipping
the names or dropping the panes or writing the edit raw, a restore taking
any kind, any header, more on the line, odd hex, a screen after a pane, no
screen, dropping the panes or a negative scroll, low memory keeping; in the
App, the keyboard kept past `dispose` (the phone's crash), a quit keeping
the screens, a background saving none, an inline model saving late, the
screens shown at every start, a restore when keeping is off, a save when it
is off or while quitting, a failed save not said; in the save, lines run
together, every verb given back, text not given back, the failure not
marked, nothing given back; in gui, a restored scroll forgotten at once or
ignored, a restored edit not an edit, the kept sizes kept; in files, no
rename, a temporary left. The event's layout was missed at first; the
rotation test now touches after a rotation. The two left are equivalent
pairs: a push clears the slot it fills and `back` clears the one it pops,
and either alone keeps a stale record from a new screen (nothing else
writes those slots), so planting one finds the other still there; both
kept, as hygiene.

Not covered here: the template's `entry.cpp` (compiled on the phone; its new
lines were compiled and run on the host against the real `App` and
`Console`), a real GL context's surface loss, and how a screen looks (the
gui `Table` places its grid at the top left: it has no table-wide alignment
yet, noted for 7.9).
