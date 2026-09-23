# gui &mdash; internals

The libGDX-flavoured widget set, skinning, and the Ui host.

Programmer counterpart: [../programmer/gui.md](../programmer/gui.md)

- Public types: 57
- Translation units: 30

## Layout

```
gui/
  include/squared/gui/   one header per public type, plus gui.hpp
  src/                        one translation unit per type with definitions
  Makefile
```

Types with no out-of-line definitions have no `.cpp`. Adding one means adding
its file to `SOURCES` in the module's `Makefile`.

## Notes

`Skin` is the module's largest resident cost: ten
`std::unordered_map<std::string, T>` members, 688 bytes of object plus a heap
node per entry. See [standard-library-deviations.md](
standard-library-deviations.md).

`Ui` owns the `scene2d::Stage`, the content widget, and the overlay stack. It
is the only type that knows about `sq::app`, which keeps the
widget set free of any platform dependency.

`gui/src/detail/gui_detail.hpp` holds the helpers that used to sit in
`gui.cpp`'s anonymous namespace: colour multiplication, dimension clamping,
UTF-8 cursor stepping, font-path validation, and the `style_or_default`
template. They are `inline` in `sq::gui::detail` because several
translation units now need them. The header is not installed.

`Widget` publishes `actor_interface_id` so `scene2d::actor_cast<Widget>()`
recovers it from an `Actor&` during tree traversal. `Ui`, `Widget` and `Stack`
all walk Scene2D trees this way; none of them uses RTTI.

`Widget::wants_text_input()` decides whether focusing a widget raises the
platform soft keyboard. `TextField` returns true; everything else returns
false. `Ui` reads it in three places and no longer names `TextField` at all,
which is why `ui.cpp` does not include `text_field.hpp`. Any custom text-entry
widget can opt in by overriding it.

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
`graphics2d/src/detail/bitmap_font_detail.hpp` without `inline`, so
`bitmap_font.o` and `glyph_layout.o` both emitted them. Archiving never
noticed; the first executable to link both objects failed.

That had been latent since the font code was written, and would have surfaced
in an application rather than here. The whole tree is now checked with

```sh
g++ main.cpp -Wl,--whole-archive build/release/lib/*.a -Wl,--no-whole-archive
```

which forces every object of every archive into one binary. It is worth
re-running after adding anything to a `detail/` header.

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

All four `graphics2d` tests, the `gui` painter test and the skin spike now
list their archives. Verified by touching a `files` source and a `gui` source
and confirming both binaries relink.
