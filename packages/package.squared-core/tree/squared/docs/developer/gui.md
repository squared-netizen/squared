# gui &mdash; internals

The libGDX-flavoured widget set, skinning and the `Ui` host, and the scene
graph under them: `Actor`, `Group`, `Stage` and input routing. Until Sprint 1
the scene graph was its own module, `scene2d`. It merged here because `gui`
was its only user, `Widget` derives from its `Group`, and the seam had bred
duplicate key and modifier types (deduplicated in Sprint 3).

Programmer counterpart: [../programmer/gui.md](../programmer/gui.md)

Layouts (the node table, `check`, `LayoutWriter`, `instantiate`): [layouts.md](layouts.md).

- Public types: 70

## Layout

```
gui/
  *.hpp          29 headers: the widgets, Ui, Skin, the scene graph, and
                 input.hpp (the input-routing types together); gui.hpp
                 includes the whole module
  src/           43 supporting headers, and the module's sources
    detail/      private: gui_detail.hpp
  test/
  Makefile
```

Types with no out-of-line definitions have no `.cpp`. Adding one means adding
its file to `SOURCES` in the module's `Makefile`. See
[file-layout.md](file-layout.md).

## Notes

`Skin` is the module's largest resident cost: ten
`std::unordered_map<std::string, T>` members, 688 bytes of object plus a heap
node per entry. See [standard-library-deviations.md](
standard-library-deviations.md).

`Ui` owns the `Stage`, the content widget, and the overlay stack. It
is the only type that knows about `sq::app`, which keeps the
widget set free of any platform dependency.

`gui/src/detail/gui_detail.hpp` holds the helpers that used to sit in
`gui.cpp`'s anonymous namespace: colour multiplication, dimension clamping,
UTF-8 cursor stepping, font-path validation, and the `style_or_default`
template. They are `inline` in `sq::gui::detail` because several
translation units now need them. The header is not installed.

`Widget` publishes `actor_interface_id` so `actor_cast<Widget>()` recovers it
from an `Actor&` during tree traversal. `Ui`, `Widget` and `Stack` all walk
actor trees this way; none of them uses RTTI. See
[Downcasting without RTTI](#downcasting-without-rtti).

`Widget::wants_text_input()` decides whether focusing a widget raises the
platform soft keyboard. `TextField` returns true; everything else returns
false. `Ui` reads it in three places and no longer names `TextField` at all,
which is why `ui.cpp` does not include `text_field.hpp`. Any custom text-entry
widget can opt in by overriding it.

## A text field's commit, and a slider held (7.5a)

The Stage needs to know when a text edit is finished, and whether the user
holds a widget. `TextField` gained a commit callback (a function pointer and
a `void *`, 16 bytes, over `std::function`'s 32: priority 1, and the Stage
needs no captures), an `edited_` flag set by every change the user makes
(typing, Backspace, Delete; not `set_text`), and `focused()`. Enter commits;
losing the focus commits only when `edited_`, so tapping in and out sends
nothing. `TextField` grew from 248 to 264 bytes; `edited_` fits in padding.
`Slider::dragging()` reads the existing `drag_pointer_`: no new state.

The check box and the slider keep their `std::function` change callbacks
(debt from before); the Stage's lambdas capture two pointers, which both
libraries store in place, so binding allocates nothing for them.

## What a screen keeps (7.6)

For the Stage's kept screens (stage's developer page): `TextField::edited()`
reads the existing `edited_`, and `restore_edit( text )` is `set_text` with
`edited_` set, so the restored edit commits as a typed one does. `ScrollPane`
and `TextField` publish interface ids (`SCRL`, `TXTF`), so the Stage finds
them with `actor_cast`; no storage, two vtable slots each, as every
interface. `List::restore_scroll( offset )` keeps the offset in `wanted_`
(4 bytes, in padding: the List is 384 still) and `layout` scrolls to it as
far as the count allows, until it reaches it; `set_scroll_y`, a drag's or
the app's, clears it. A plain clamp at restore time would lose the place,
since a list shown again after the process ended has no count until the
model sends it. `Table::release_kept()` swaps the `Lines` pool with an empty
vector when none is borrowed (not from inside a layout), so low memory can
free it; `kept_count()` is for the Console and the test.

## `List` (7.5b)

`list.hpp`, `src/list.cpp`; built by `instantiate` for a `list` node, bound by
the Stage. 384 bytes (libstdc++): the row's copy (a vector of nodes, the
layout's words and numbers), the hooks, the row tables and the row each
shows (two vectors), and a few floats.

- **One height.** The first row table is built when the list is first
  measured, and its preferred height is every row's. Measuring builds it, in
  `preferred_size`, which is `const`: a `const_cast`, since a `List` is never
  made `const` (one widget tree, owned by the Ui). Chosen over measuring in
  `layout`: by then the parent has laid it out at a size it did not know.
- **Tables reused.** `layout` builds tables up to the height's rows plus one
  (however it is scrolled, no more rows show at once), at most
  `max_rows_built` (64). Row `r` is in table `r` modulo the tables: scrolling
  one row moves one row to another table, the one that left the screen;
  the others are not touched, so their bindings stay. `place` tells the
  owner (`set_on_row`) when a table's row changes, positions it at
  `r * height - scroll`, and hides the tables past the count or the foot.
  Variable heights were not taken: they need every row measured, or a
  guess the scroll corrects, for a case no app has yet.
- **Drag to scroll.** Its own `pointer_event` scrolls when no row's widget
  took the finger (a label, a row's padding). A finger on a row's button is
  the button's (Ui captures the pointer for it); the list watches with a
  capture-phase listener (one `std::function` holding `this`, made once),
  and once the finger has moved `drag_slop` (8 units) it turns that move
  into a cancel (the button lets go: no tap), scrolls, and stops the moves
  and the release after it. Events for the list itself are left to
  `pointer_event`, so nothing scrolls twice.
- **Copying the row**, not pointing at it: a C++ screen's layout lives in the
  Stage's scratch, which a C++ pane writes over, and rows are built later
  (a rotation adds tables). The words and numbers are copied whole (offsets
  stay valid); a screen's are a few hundred bytes.
- **Built without the check** (`detail::build_row`): the row's uids need the
  list around them, which the copy does not have, and the layout it came
  from passed `check` already.

## A `Table`'s layout allocates nothing (7.5b)

`measure` and `layout` worked in six `std::vector<float>`s made each time;
once the Stage lays out every frame that something changed (a bound value,
a list scrolled), that was an allocation a frame. They now borrow `Lines`
(columns and rows) from a pool kept in `table.cpp`: a table measured inside
its parent's measure or layout borrows the next one, so each depth of
nesting has its own, and they keep their capacity. Once each depth has been
laid out at its largest, a layout allocates nothing. The pool is a
`std::vector<std::unique_ptr<Lines>>` (pointers, so growing it never moves a
`Lines` in use), grown to the deepest nesting seen, and is the UI thread's
alone, as the widgets are. Chosen over vectors in each `Table`: 6 &times; 24
bytes in every table, a list's rows included (priority 1).

## Downcasting without RTTI

`Actor::actor_interface(ActorInterfaceId)` is the framework's replacement for
`dynamic_cast`. Each Actor-derived interface publishes a
`static constexpr ActorInterfaceId actor_interface_id` and overrides the query
to answer its own identifier, delegating to its direct base otherwise:

```cpp
[[nodiscard]] void* actor_interface(ActorInterfaceId id) noexcept override
{
    return id == actor_interface_id ? this : Actor::actor_interface(id);
}
```

Delegating up the chain is what keeps an interface published anywhere in the
chain reachable: `actor_cast<Group>` on a `Widget` still resolves, because
`Widget` hands the unknown identifier to `Group`.

`actor_cast<Interface>(actor)` supplies the identifier and restores
the pointer type. It accepts null and has a `const` overload.

Cost: one virtual call, no storage. `Actor` already had a vtable, so the two
extra slots are free, and the call is cheaper than the `dynamic_cast` it
replaced. Identifiers are four-character constants owned by the layer that
publishes the interface; the framework reserves values whose high byte is
`0x00`. `Group` uses `0x47'52'50'01`, `Widget` uses `0x57'44'47'54`.

**Adding a layer:** derive from `Actor` or `Group`, publish an identifier,
override both overloads, and delegate to your direct base. Nothing in the scene
graph needs to know your type exists.

## Scene graph notes

`Actor` and `Group` are mutually recursive: `Actor` forward
declares `Group` for its parent pointer, `Group` includes `Actor` because it
derives from it. `InputEvent` forward declares `Actor` for the same reason,
which is what lets `input_listener.hpp` stay a two-line header.

## Safe area

`Ui` keeps one `Insets` (16 bytes, no allocation) and applies it in a single
private function, `place_content()`, which `set_content()`, `resize()` and
`set_safe_area()` all call. Before it existed each of the first two set the
content's bounds to the full viewport on its own; routing them through one
place is what makes the insets survive both. The content rectangle is the
viewport minus the insets, clamped at zero so oversized insets give an empty
rectangle rather than a negative size. Pointer coordinates are untouched: a
point is still in viewport space, and widgets outside the content rectangle
simply are not hit.

Overlays are deliberately not inset. A dialog centred on the viewport is
centred on the screen, not on the part of it between the bars.

Cost: one call to `place_content()` per resize or content change; nothing per
frame.

## BatchPainter

The concrete end of the widget set: every `Painter` call becomes a quad in a
`SpriteBatch`. It owns neither the batch nor the font.

**Fills come from the skin's `white` region, not a texture of the painter's
own.** That is the whole reason `set_fill_source()` exists. A painter with its
own 1x1 white texture forces a texture switch at every fill, and therefore a
draw call; taking white from the atlas means fills share a page with every
other widget graphic and a whole interface can be one call. The test measures
both: four sprites, one draw call with the skin's white, four draw calls
without.

Passing null is supported and creates an owned 1x1. It is the fallback, not
the intent.

**Text goes through one scratch layout.** `measure_text()` and `draw_text()`
lay text out into a `GlyphLayout` member, `text_layout_`, rather than a local.
Its storage grows to the longest text the painter has handled and then stays,
so painting text allocates nothing once that has happened. A local layout per
call allocated twice per text, and widgets measure and draw every frame: that
was all 8 to 10 allocations per frame the demo made (Sprint 3, task 3.0). Cost:
`sizeof(BatchPainter)` 472 → 536 bytes, plus the kept storage (56 bytes per
glyph of the longest text). The base `Painter::measure_text()` still uses a
local layout; it has no state of its own, and `BatchPainter` overrides it.

### Clipping

`push_clip`/`pop_clip` map onto `SpriteBatch::set_clip`, which takes a scissor
rectangle in framebuffer pixels.

The conversion lives here rather than in the batch because only the caller
knows its own camera and drawable size; `set_viewport()` supplies both. The
GUI measures y downwards from the top and a scissor measures upwards from the
bottom, so `bottom = pixel_height - top - height`. Getting that wrong clips
the mirror image of what was asked for, which is why the test asserts the
exact scissor values rather than that clipping "happened".

Nested clips intersect: a child may narrow what its parent allowed, never
widen it.

The stack is a fixed `std::array` of 16, not a vector: push and pop run per
frame, and a container that could allocate in the frame loop is what the
memory rules exist to prevent. Past 16 the parent's clip stays in force -
overdrawing is wrong in a way you can see, clipping to the wrong rectangle is
wrong in a way you cannot.

**Why the batch owns the scissor.** Changing it requires flushing, because it
cannot change part-way through a draw call. Anywhere else the flush and the
change could be desynchronised, and forgetting the flush does not fail loudly:
it tears, intermittently, depending on what was queued. `end()` clears any
clip, so one frame cannot restrict the next.

## An ODR violation the first link exposed

`detail::parse_record` and its siblings were defined in
`graphics/src/detail/bitmap_font_detail.hpp` without `inline`, so
`bitmap_font.o` and `glyph_layout.o` both emitted them. Archiving never
noticed; the first executable to link both objects failed.

That had been latent since the font code was written, and would have surfaced
in an application rather than here. The whole tree is now checked with

```sh
g++ main.cpp -Wl,--whole-archive build/release/lib/*.a -Wl,--no-whole-archive
```

which forces every object of every archive into one binary. It is worth
re-running after adding anything to a `detail/` header.

## Style classes are matched on their last name

A skin file keys each style table by a Java class name:
`com.badlogic.gdx.scenes.scene2d.ui.Label$LabelStyle`. The loader compares only
what follows the last dot (`Label$LabelStyle`), through `short_class_name()`
and `section()`, and no package name appears in its code. Two things follow.
A skin written for squared may use the short form; and a rename of squared's
own modules can no longer reach a libGDX name, which is how it went wrong once:
the package part used to be spelled out in the loader, a blanket rename
(`scene2d` &rarr; `gui`) rewrote it, no key matched, every style in a real
`uiskin.json` was reported as unused and a device showed backgrounds with no
text. The tests of the time built their skins in code and passed throughout.

The same class under two names in one file (`Color` and
`com.badlogic.gdx.graphics.Color`) is an error rather than a silent pick of one.
Two different libGDX packages that happen to end in the same name would be
confused; none of the supported classes has such a twin.

Cost: a scan of the file's class keys per supported class, at load time only
(about a dozen keys, eleven lookups). Nothing is retained.

`gui/test/skin_loader_test.cpp` loads a skin in both forms through the real
parser and fails unless the label, button and check-box styles come out with
their font and colour.

## libGDX skin JSON is not JSON

`load_libgdx_skin` normalizes before parsing, in two passes, because the
format the stock skin ships in is rejected by any strict parser.

**Bare tokens.** Keys and values are unquoted:
`com.badlogic.gdx.graphics.g2d.BitmapFont: { default-font: { file: default.fnt } }`.
The normalizer quotes anything that is not a number, `true`, `false` or
`null`. It also strips line and block comments.

**Trailing commas.** Nearly every block in the stock skin ends with one. That
pass is string-aware: a comma inside a string literal is content.

Together they turn the reference skin into 18 top-level sections of valid
JSON. The first was written from the format; the second was found by loading
the real file, which had not been done before.

## A test binary must depend on what it links

Three rounds of debugging a skin-loading failure went to stale binaries, not
to the bug. The fix was correct the first time; the test kept running a
previous build.

The rules listed only the sources they compiled, and named the archives in the
recipe alone. Make has no way to know a recipe mentions a file: an archive that
is not a prerequisite is not a reason to relink, so rebuilding the framework
left every test binary untouched and apparently unchanged behaviour.

This is the same shape as `make apk` linking whatever archives happened to
exist. It is worth stating as a rule, because it fails silently in exactly the
situation where a test matters most - after a change:

**Anything a recipe links belongs in the prerequisites.**

All four `graphics` tests (then `graphics2d`'s), the `gui` painter test and the skin spike now
list their archives. Verified by touching a `files` source and a `gui` source
and confirming both binaries relink.
