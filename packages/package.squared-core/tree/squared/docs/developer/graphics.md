# graphics &mdash; internals

The rendering context and everything drawn through it: `Context` and `Color`,
textures, atlases, regions, the sprite batch, the 2D camera, bitmap fonts and
glyph layout. Until Sprint 1 the second half was its own module,
`graphics2d`; it merged here because it had one user and the dependency ran
one way.

Programmer counterpart: [../programmer/graphics.md](../programmer/graphics.md)

- Public types: 30
- One backend linked, chosen at build time

## Layout

```
graphics/
  *.hpp          the 11 workhorse headers, and graphics.hpp, which includes them all
  src/           19 supporting headers, and the module's sources
    common/      backend-independent halves of Texture and SpriteBatch
    detail/      private: bitmap_font_detail.hpp, image_decode.hpp
    null/        the headless backend
    gles/        the EGL and GLES 3.0 backend
  test/
  Makefile
```

Supporting headers are included as `<squared/graphics/src/...>`; `detail/` is
reached only by the module's own sources. See [file-layout.md](file-layout.md).

## Link-time substitution

`Context` is a concrete `final` class with a fixed layout &mdash; a window
pointer, a native context pointer, pixel dimensions, a generation counter and a
preserved flag. It declares its members and defines none of them. A backend
supplies every definition.

That was the existing design, and it is the right one here: no vtable, no
indirection, one name at the seam, and the class was already shaped for it.
The cost is that a missing backend is a link error naming a private member
function, which is a terrible message. The null backend is the answer to that.

Choosing one is a make variable:

```sh
make SQUARED_GRAPHICS_BACKEND=null   # default
make SQUARED_GRAPHICS_BACKEND=gles
```

A new backend is a new directory under `graphics/src/` holding three files:
`context.cpp`, `sprite_batch.cpp` and `texture.cpp`. The first is all of
`Context`; the other two are only the members that issue graphics calls, since
the policy halves of `SpriteBatch` and `Texture` live once in `src/common/`.
No header changes, no other module changes. `graphics/Makefile` treats a
directory under `src/` as a backend only if it holds `context.cpp`, and lists
the available ones when given a name that does not exist.

## The null backend

`graphics/src/null/context.cpp`. Every symbol resolves, nothing draws.

It is not a stub for its own sake. It buys three things:

- A project with no backend selected shows a black screen and runs, instead of
  failing to link. Debuggable rather than cryptic.
- Textures, the sprite batch and fonts, and the whole of `gui`, become linkable
  and testable on a machine with no GPU and no EGL. Before it, they could be
  compiled but never linked.
- It answers the same contract a real backend does, so a test written against
  it is a test of the contract: `create` succeeds, `valid()` turns true,
  `generation()` advances on every activation, `present()` returns true.

Nothing in it allocates.

## Two contract changes

**`present()` returns `[[nodiscard]] bool`.** It was `void`, which discarded
the only notice some devices give that the surface is gone. The failure mode is
an application frozen on its last frame after a phone call. `Context` already
carries `generation()` and `resources_preserved()`, so `present()` only has to
say *stop*; those two say *what changed*.

**`create()` takes a `ContextConfig`.** It was `create(title, width, height)`,
which cannot express an `ANativeWindow*` the platform hands you and did not
create. The config carries a `void*` native window alongside the title and
size, so no squared header includes a platform header, and each backend reads
the fields that mean something to it.

## The GLES backend

`graphics/src/gles/context.cpp`. EGL 1.4 and GLES 3.0, RGBA8 with a 16-bit
depth buffer and 8-bit stencil.

The display, config and surface are file-scope, and that is correct rather than
lazy: `EGL_DEFAULT_DISPLAY` is one display per process, and NativeActivity
gives a process one native window at a time. A second `Context` would be a
second window, which the platform does not offer. `Context`'s own members carry
what is per-instance: `window_` holds the `ANativeWindow*`, `native_context_`
holds the `EGLContext`.

**Buffer geometry is set from `EGL_NATIVE_VISUAL_ID` before creating the
surface.** Skipping that is the classic Android EGL bug: the surface is created
against a buffer format the config did not ask for, and it shows up as a failed
`eglMakeCurrent` or as colours that are wrong on some devices only.

**`resume()` tries the surviving context first.** If `eglMakeCurrent` binds the
old `EGLContext` to the new surface, every GPU object it held is still there
and `resources_preserved()` reports true. Only when that fails is a new context
created and the application told to rebuild. This is what makes backgrounding
cheap in the common case.

**`present()` reads `eglGetError()` on failure.** `EGL_BAD_SURFACE` and
`EGL_CONTEXT_LOST` both destroy the surface and return false, so the platform
layer's next `resume()` starts from a clean state.

`set_native_window()` exists because Android does not hand back the same
window: it destroys it on backgrounding and supplies a different one. `suspend`
and `resume` alone cannot express that, so the platform layer calls this
between them.

One deliberate deviation: the backend declares
`ANativeWindow_setBuffersGeometry` itself rather than including
`<android/native_window.h>`, which would pull in `hardware_buffer.h`,
`data_space.h` and `rect.h` for declarations nothing here uses. The function
has been public and stable since API 1, and a future change would be a compile
error at that line rather than a silent mismatch.

## Texture

Split the way `Context` is: `src/common/texture_common.cpp` holds
everything that is policy and bookkeeping, and `src/<backend>/texture.cpp`
holds the six members that issue a graphics call. The recovery logic - the
part with the interesting failure modes - is written once and tested once.

`sizeof(Texture)` is 128 bytes. The largest part is the 48-byte
`files::FileHandle`, which is what lets `restore()` refill the texture without
reaching back into the `AssetManager` that created it. The alternative was
routing recovery through `AssetManager::reload`, which creates a *new* texture
and leaves every existing handle pointing at the dead one - correct for
hot-reloading a changed file, exactly wrong for context loss.

**`content_generation()` is the stale-region signal.** It increments on every
upload, restore, release, invalidate and destroy. A `TextureRegion` records
the generation it was built against; when the two differ, its coordinates no
longer describe the pixels that are there, and after a `Discard` there are no
pixels at all. Without it, a region outliving its texture's content is a crash
after a phone call.

`Discard` clears `width_` and `height_` on restore, so a caller cannot mistake
retained dimensions for live content.

**Create-from-memory refuses `ReloadFromAsset`.** The default options ask for
it, and a texture built from a caller's pixels has no asset behind it.
Accepting the combination would produce a texture that looks correct and
silently fails to come back. `create_rgba(w, h, pixels)` with no options picks
`RetainPixels` instead, which is the only policy that can honour memory the
caller supplied and did not name. `Regenerate` with a null callback is refused
for the same reason.

A recovery callback that returns true without uploading is treated as a
failure rather than leaving an empty texture that would draw as garbage.

`load()` releases the encoded bytes before uploading. On a large atlas the
encoded file, the decoded pixels and the GPU copy would otherwise all be
resident at the same moment.

## TextureRegion and the stale check

`TextureRegion` was already fully inline in its header; the work here was the
validity signal.

A region records `texture_->content_generation()` at construction.
`valid()` then requires the texture to still hold content *and* to be at the
same generation. A texture that was discarded, reloaded or regenerated has
moved on, and the region's coordinates describe pixels that are no longer
there - or are not there at all. Drawing it would sample whatever now occupies
that texture unit.

**It cost zero bytes.** `sizeof(TextureRegion)` is 40 before and after: the
`std::uint32_t` fits in padding the struct already had between its pointer and
its ints. A nine-patch holds nine regions and pays nothing for the check.

Three states, not two:

| | `valid()` | `stale()` |
|---|---|---|
| never had a texture | false | false |
| has current content | true | false |
| had content, lost or replaced it | false | true |

`stale()` exists because `valid()` alone cannot tell an empty slot from a
region whose atlas needs rebuilding, and those want different responses.

`refresh()` re-binds to the current generation. It is for the common case
after context loss, where a texture was restored with the same pixels in the
same layout - the caller asserting that the layout still matches. A region
into an atlas that was repacked is not made correct by it, because its
coordinates never described the new layout. It returns `valid()` so a caller
that guessed wrong finds out.

`AtlasRegion::rotated_clockwise()` delegates to its `TextureRegion` rather
than storing a second copy. The region already carries it because its UV
arithmetic depends on it, and a duplicate could disagree with the coordinates
it is meant to describe.

## TextureAtlas

One pass over the libGDX `.atlas` text. The whole grammar is indentation: an
unindented line names a page or a region, an indented one is a property of the
region above it, and a blank line means the next unindented line names a page.

`load()` takes a `files::FileHandle`, and pages are resolved with
`sibling()` - the handle carries its file system, so an atlas works against
the APK and a directory alike with no path juggling.

**The parse is staged.** A region's properties arrive after its name, and its
`TextureRegion` cannot exist until its page is loaded, so the parse collects
plain numbers into a local vector and builds the real regions once at the end.

`textures_` is `vector<unique_ptr<Texture>>` and the class is non-movable, both
load-bearing: every region holds a `const Texture*`, and a plain vector would
dangle all of them on reallocation.

**The filter comes from the file and nothing can override it.** `filter:
Nearest,Nearest` is what the packer intended; commodore64 is unreadable at
Linear. Every MipMap variant maps to Linear, because squared uploads no
mipmaps and a mipmapped minification filter without them samples black.
`repeat:` maps to wrap the same way.

`size:` is passed to `TextureRegion` unchanged even when `rotate: true`. The
region swaps its own storage extents; swapping here as well would undo it.

**Recovery.** The one-argument `load()` uses `ReloadFromAsset`: a page always
has an asset behind it and it costs nothing resident. `Regenerate` is refused,
because it needs a callback and the parameter is a bare policy - accepting it
would produce an atlas that silently fails to come back after a phone call.

`restore()` refreshes every region after restoring the pages. The pixels come
back from the same file in the same layout, so the coordinates still describe
them, and only the recorded generation is stale. This is the case
`TextureRegion::refresh()` exists for: the atlas asserting the layout matches
on its regions' behalf. A page that fails leaves its regions invalid, which is
correct.

Under `Discard`, regions stay listed and report themselves invalid rather than
being dropped, so `region_count()` does not change under the caller.

`k_max_pages` and `k_max_regions` bound what a corrupt file can make this
allocate before anything notices.

## SpriteBatch

Split like `Texture`: `src/common/sprite_batch_common.cpp` holds the vertex
building, the flush decisions and the begin/draw/end state; the GPU half -
buffers, the shader, the draw call - is per backend. It does not build on
`sq::gles`, because the header stores raw handles and `graphics` depending
on `gles` would invert the layering.

**88 bytes** (the size the test prints; it was 80 before `skipped_draws_`
and `active_texture_` arrived), plus one vertex buffer allocated at
`initialize()` and never grown. A frame loop must not allocate, and a vector that reallocated mid-frame
would be exactly that: capacity is the budget and `flush()` enforces it.

**Two reasons to flush, and they are the same reason** - what is queued can no
longer go in one draw call. A different texture cannot share the call; a full
buffer has nowhere to put the next quad.

Texture identity is compared **by address**, not by GL name. That is what a
flush decision actually asks, the name is private to `Texture` anyway, and the
backend binds through the public `bind()`.

**A region that fails `valid()` is skipped**, not drawn. Its texture may have
been discarded or reloaded underneath it, and drawing would sample whatever
now occupies that unit. This is the payoff for the generation counter.

`invalidate()` and `release()` **abandon whatever was queued**. Leaving
`drawing_` set would make the next `begin()` look like a nested one - a
programmer error - when the real cause was the context going away. `begin()`
asserts on a genuine nested call, because that means two pieces of code
believe they own the batch.

Capacity is capped at 16384 sprites: six indices per sprite must stay
addressable by the 16-bit index buffer.

Indices are uploaded once as `GL_STATIC_DRAW` - quad N is always the same six
values - so a flush uploads vertices only, as `GL_STREAM_DRAW`.

### Orientation follows the camera

`draw()` builds corners at `(x, y)`, `(x, y + h)`, `(x + w, y + h)`,
`(x + w, y)`. Which of those is the quad's top depends on which way y points,
and only the camera knows that. `begin()` reads `camera.origin()` once into
`y_down_`; `append_quad()` then gives the corners at `y` the image's top edge
(`v1`) under a `TopLeft` camera and its bottom edge (`v2`) under `BottomLeft`.

This used to be fixed at y-up while the camera defaulted to `TopLeft` and the
GUI and glyph layout measure from the top, so every textured quad on a device
was drawn upside down in place. Text showed it immediately; a symmetric
nine-patch or a flat fill cannot, and the null backend draws nothing, so no
test noticed. The test now draws a region whose top and bottom rows differ
under both cameras and checks the texture coordinates of each corner.

Cost: `y_down_` is one `bool` in the padding after `drawing_` and
`invalidated_`, so `sizeof(SpriteBatch)` is unchanged; per quad it is two
selects. No allocation.

Known limitation: `draw(const Sprite&)` names its corners `bottom`/`top` in
y-up terms. The texture follows the camera correctly, but positive rotation is
counter-clockwise only under `BottomLeft`; under `TopLeft` it appears
clockwise. Documented in the programmer page rather than changed, since
nothing rotates sprites yet.

### The null backend counts draw calls

Batching is invisible from outside: the same pixels appear whether they took
one draw call or a thousand, and only the frame time tells you which. The
headless backend counts them, so `make -C graphics test` asserts the
decisions directly - one call for ten sprites sharing a texture, two when the
texture switches once, six when two textures interleave six times, and a flush
exactly at the capacity boundary.

That last number is the argument for sorting by texture, made in a test rather
than in a comment.

It also records the texture coordinates of the first quad in each flush
(`detail::g_null_first_quad_uv`), which is what lets the orientation test
above run without a driver.

## Four changes that came out of one bug

An application drew nothing, and finding out why took several rounds. The
cause was one stale `TextureRegion`, but four separate decisions had conspired
to make it both easy to cause and hard to see. Each is worth naming.

**`resources_preserved()` now answers the caller's question.** It meant "did a
resume preserve anything", so a first `create()` reported `false` - nothing had
been lost, because nothing had existed. Every application dutifully reloaded
its assets before drawing its first frame, which moved every texture's
generation and invalidated every region copied out of an atlas. It now means
"are the GPU objects an application owns still valid", which is what a caller
is actually asking, and a first context reports `true`.

**`restore()` takes the Context.** `restore(bool context_preserved = false)`
read as cheap and behaved destructively, and a bare `restore(false)` at a call
site says nothing about which way round the flag goes. The primary overload
now takes the `Context` and reads the answer from the object that knows it:

```cpp
atlas.restore(graphics);          // cannot be got backwards
```

The bool overload remains for tests and has no default: a caller who wants to
state it must state it.

**`SpriteBatch::skipped_draws()`.** A draw whose region is stale is skipped
silently - correct, since drawing it would sample whatever now occupies that
texture unit, and invisible, since an interface that draws nothing looks
exactly like one that was never asked to. The count makes the difference one
assertion in a test instead of an afternoon of bisecting.

**`BatchPainter` keeps a pointer to its fill region.** A copy goes stale the
moment the atlas reloads a page, and the caller would have to know to set it
again after every restore. `Skin`'s drawables never had this problem because
they hold pointers into the atlas. The rule is worth stating plainly: **a
`TextureRegion` copied out of an atlas does not survive a restore.**

Three of the four are about the same failure of API design - a call that reads
as harmless and is not. The fourth is about making a silent correct behaviour
audible.

## Solid fills sample one texel

A skin's white patch is a few pixels square with linear filtering. Stretching
all of it across a widget lets the edge texels blend with whatever the packer
placed beside them, so a solid fill comes out blurred at its borders.
`fill_rectangle` samples the centre texel alone, which cannot reach a
neighbour. For a 1x1 fallback it is the same texel either way.

It is computed per call rather than stored, so it stays a view of the current
region rather than a copy that would go stale.

## Notes

`graphics/src/detail/bitmap_font_detail.hpp` holds the
BMFont parsing helpers shared by `bitmap_font.cpp` and `glyph_layout.cpp`.

`BitmapFont` and `GlyphLayout` both name their options struct only by
reference but use it in a defaulted argument, so those two headers include the
options definition rather than forward declaring it.
