# gui &mdash; internals

The libGDX-flavoured widget set, skinning and the `Ui` host, and the scene
graph under them: `Actor`, `Group`, `Stage` and input routing. Until Sprint 1
the scene graph was its own module, `scene2d`. It merged here because `gui`
was its only user, `Widget` derives from its `Group`, and the seam had bred
duplicate key and modifier types (deduplicated in Sprint 3).

Programmer counterpart: [../programmer/gui.md](../programmer/gui.md)

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
