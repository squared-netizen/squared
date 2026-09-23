# graphics2d &mdash; internals

Textures, atlases, regions, sprites, bitmap fonts and the 2D camera.

Programmer counterpart: [../programmer/graphics2d.md](../programmer/graphics2d.md)

- Public types: 27
- Translation units: 3

## Layout

```
graphics2d/
  include/squared/graphics2d/   one header per public type, plus graphics2d.hpp
  src/                        one translation unit per type with definitions
  Makefile
```

Types with no out-of-line definitions have no `.cpp`. Adding one means adding
its file to `SOURCES` in the module's `Makefile`.

## Texture

Split the way `graphics::Context` is: `src/common/texture_common.cpp` holds
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
`sq::gles`, because the header stores raw handles and `graphics2d` depending
on `gles` would invert the layering.

**80 bytes**, plus one vertex buffer allocated at `initialize()` and never
grown. A frame loop must not allocate, and a vector that reallocated mid-frame
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

### The null backend counts draw calls

Batching is invisible from outside: the same pixels appear whether they took
one draw call or a thousand, and only the frame time tells you which. The
headless backend counts them, so `make -C graphics2d test` asserts the
decisions directly - one call for ten sprites sharing a texture, two when the
texture switches once, six when two textures interleave six times, and a flush
exactly at the capacity boundary.

That last number is the argument for sorting by texture, made in a test rather
than in a comment.

## Notes

`graphics2d/src/detail/bitmap_font_detail.hpp` holds the
BMFont parsing helpers shared by `bitmap_font.cpp` and `glyph_layout.cpp`.

`BitmapFont` and `GlyphLayout` both name their options struct only by
reference but use it in a defaulted argument, so those two headers include the
options definition rather than forward declaring it.
