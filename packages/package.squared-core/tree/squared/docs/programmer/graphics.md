# graphics

The rendering context and colour, then everything you draw with it: textures, atlases, regions, sprites, the sprite batch, the 2D camera, bitmap fonts and glyph layout.

`import sq.graphics;` gives every type here. Each type is written in a file of its own, a partition of the module: the ones at the module root (`squared/graphics/*.cppm`) are the everyday ones, those under `squared/graphics/src/` the supporting types they are made of. See [include-layout.md](include-layout.md).

Developer counterpart: [../developer/graphics.md](../developer/graphics.md)

## Context and colour

| Type | Header | Purpose |
|---|---|---|
| `Context` | `squared/graphics/context.cppm` | Own the native window and rendering context selected at link time |
| `ContextConfig` | `squared/graphics/src/context_config.cppm` | What `Context::create` needs: the native window, a title and the logical size |
| `Color` | `squared/graphics/color.cppm` | Normalized four-component color used by Squared graphics APIs |

## Textures and atlases

| Type | Header | Purpose |
|---|---|---|
| `Texture` | `squared/graphics/texture.cppm` | Move-only texture owned by the selected graphics backend |
| `TextureRegion` | `squared/graphics/texture_region.cppm` | Non-owning rectangular view into a Texture |
| `TextureAtlas` | `squared/graphics/texture_atlas.cppm` | Owning, transactionally loaded libGDX text texture atlas |
| `AtlasRegion` | `squared/graphics/atlas_region.cppm` | One named region and its libGDX-compatible atlas metadata |
| `TextureFilter` | `squared/graphics/src/texture_filter.cppm` | Nearest or linear sampling; also what `gles::Texture2D` takes |
| `TextureWrap` | `squared/graphics/src/texture_wrap.cppm` | Clamp, repeat or mirrored outside zero to one; also what `gles::Texture2D` takes |
| `TextureFormat` | `squared/graphics/src/texture_format.cppm` | What a texel holds: `Rgba8`, or `Alpha8` (one byte, drawn tinted by the draw colour) |

### Alpha textures and partial updates (12.3d)

A glyph atlas, a mask: one byte a texel, a quarter of RGBA's memory. It draws
as the draw colour, each texel its alpha, so the same `SpriteBatch` and
`Painter::draw_region` tint it; no shader of your own. Filled a piece at a
time with `update`, which uploads only the rectangle that changed:

```cpp
sq::graphics::Texture glyphs;
glyphs.create_alpha( 1024, 1024, atlas.pixels(),
                     TextureRecoveryOptions::regenerate( refill, &atlas ) );
// later, after new glyphs were drawn into the atlas's image:
glyphs.update( dirty.x, dirty.y, dirty.width, dirty.height,
               atlas.pixels() + dirty.y * 1024 + dirty.x, 1024 );
```

| Call | Does |
|---|---|
| `create_alpha( w, h, pixels, recovery )` | an `Alpha8` texture from `w * h` bytes. `ReloadFromAsset` is refused (no asset behind memory); `Regenerate` for pixels you keep (its callback calls `TextureRecoveryTarget::upload_alpha`), `RetainPixels` for a copy kept by the texture |
| `update( x, y, w, h, pixels, stride )` | replace that rectangle, in the texture's own format (1 or 4 bytes a texel), from rows `stride` texels apart. Regions stay valid: the generation does not move. A retained copy is updated too. False, changing nothing, outside the texture, for a stride under `w`, null pixels, or a texture with no content |
| `format()` | `Rgba8` (loaded, `create_rgba`, `create_solid`) or `Alpha8` |
| `texel_bytes( format )` | 4 or 1 |

## Recovering textures after context loss

| Type | Header | Purpose |
|---|---|---|
| `TextureRecoveryPolicy` | `squared/graphics/src/texture_recovery_policy.cppm` | CPU-side recipe used to recover a texture after context loss |
| `TextureRecoveryOptions` | `squared/graphics/src/texture_recovery_options.cppm` | Recovery selection supplied when a texture is created |
| `TextureRecoveryCallback` | `squared/graphics/src/texture_recovery_callback.cppm` | Callback that regenerates a texture's pixels under the `Regenerate` policy |
| `TextureRecoveryTarget` | `squared/graphics/src/texture_recovery_target.cppm` | Synchronous destination supplied to a regeneration callback |

## Sprites, batching and the camera

| Type | Header | Purpose |
|---|---|---|
| `Sprite` | `squared/graphics/sprite.cppm` | Lightweight mutable state for drawing one TextureRegion |
| `SpriteBatch` | `squared/graphics/sprite_batch.cppm` | Efficiently draw ordered textured quads with the selected backend |
| `OrthographicCamera` | `squared/graphics/orthographic_camera.cppm` | Two-dimensional orthographic camera with pan and zoom |
| `CoordinateOrigin` | `squared/graphics/src/coordinate_origin.cppm` | Coordinate orientation used by an orthographic camera |

## Bitmap fonts and text layout

| Type | Header | Purpose |
|---|---|---|
| `BitmapFont` | `squared/graphics/bitmap_font.cppm` | Transactionally parsed text BMFont resource |
| `BitmapFontInfo` | `squared/graphics/src/bitmap_font_info.cppm` | Portable BMFont face metadata |
| `BitmapFontPage` | `squared/graphics/src/bitmap_font_page.cppm` | One texture-page dependency declared by a BMFont |
| `BitmapFontParseLimits` | `squared/graphics/src/bitmap_font_parse_limits.cppm` | Explicit resource limits for one text BMFont descriptor |
| `BitmapFontError` | `squared/graphics/src/bitmap_font_error.cppm` | What `BitmapFont::load` or `GlyphLayout::set_text` returns: true when something failed |
| `BitmapFontErrorCode` | `squared/graphics/src/bitmap_font_error_code.cppm` | Stable failure categories for BMFont parsing and glyph layout |
| `BitmapGlyph` | `squared/graphics/src/bitmap_glyph.cppm` | One Unicode scalar's source rectangle and layout metrics |
| `BitmapKerning` | `squared/graphics/src/bitmap_kerning.cppm` | Pair-specific horizontal advance adjustment in source pixels |
| `GlyphLayout` | `squared/graphics/glyph_layout.cppm` | Reusable value-semantic layout of UTF-8 text into glyph quads |
| `GlyphLayoutOptions` | `squared/graphics/src/glyph_layout_options.cppm` | Controls for one UTF-8 glyph layout operation |
| `GlyphAlignment` | `squared/graphics/src/glyph_alignment.cppm` | Horizontal placement within an optional target width |
| `GlyphLine` | `squared/graphics/src/glyph_line.cppm` | One explicit line in a GlyphLayout |
| `GlyphPlacement` | `squared/graphics/src/glyph_placement.cppm` | One SpriteBatch-ready glyph quad in top-left logical coordinates |

**Keep one `GlyphLayout` and lay text out again into it.** `set_text()` reuses
the layout's storage, and allocates only when the text needs more glyphs or
lines than any earlier text did. A layout made fresh for every call allocates
twice, every time:

```cpp
// once, alongside your font
sq::graphics::GlyphLayout layout;

// every frame
if (!layout.set_text(font, score_text)) {
    for (const auto& glyph : layout.glyphs()) { /* draw it */ }
}
```

`clear()` empties the layout and keeps that storage. Assign
`sq::graphics::GlyphLayout{}` to give it back. `reserve( glyphs, lines )`
makes the room up front, where allocating is expected (at load, not in a
frame), so a text that fits lays out with no allocation. A failed `set_text()` leaves the
previous layout as it was.

### Errors

`BitmapFont::load()` and `GlyphLayout::set_text()` return a `BitmapFontError`.
It is empty on success and true when something failed, so the failure path
reads as one line:

```cpp
sq::graphics::BitmapFont font;
if (const auto error = font.load(descriptor_text)) {
    std::printf("font: line %u: %s %s\n", error.line, error.message, error.field);
}
```

A failed `load()` leaves the font as it was; a failed `set_text()` leaves the
layout as it was. Nothing is thrown.

| Field | Meaning |
|---|---|
| `code` | The category, a `BitmapFontErrorCode`; `none` on success |
| `message` | What failed, in words, such as `"missing required field"`. Never null; empty on success |
| `field` | The descriptor field involved, such as `"lineHeight"`; empty when none is |
| `line` | The descriptor line, counted from 1; 0 when the failure is not about one line |
| `byte` | For `invalid_utf8`: the offset of the first bad byte in the text; 0 otherwise |

| Code | Returned by | When |
|---|---|---|
| `invalid_argument` | both | The parse limits include a zero; or the font is not valid, or the options are out of range |
| `limit_exceeded` | both | The descriptor, a line, a name or a count is over its parse limit; the text expands past 2^24 glyphs; out of memory |
| `malformed` | `load` | A record or field cannot be read, or the records disagree |
| `unsupported` | `load` | A record BMFont text does not define; `line` says which |
| `duplicate` | `load` | A record, page, glyph or kerning pair appears twice |
| `missing_required` | `load` | A required record or field is absent; `field` names a missing field |
| `invalid_utf8` | `set_text` | The text is not valid UTF-8 and `reject_invalid_utf8` is set, which is the default; `byte` says where |

The error owns nothing, so making, copying and returning one never allocates.
The texts in `message` and `field` live in the library for the life of the
program; keep the pointers or print them, but don't free them.

## Creating a context

The platform layer owns the `Context`. Application code receives higher-level objects and never presents the window itself.

```cpp
sq::graphics::Context context;
if (!context.create(sq::graphics::ContextConfig{
        .native_window = platform_window,   // ANativeWindow* on Android
        .title = "My Game",
        .logical_width = 1280,
        .logical_height = 720})) {
    return;
}
```

Leave `native_window` null on a backend that creates its own window.

## Per frame

```cpp
context.clear(background);
// ... draw ...
if (!context.present()) {
    // The surface is gone. Stop drawing and wait for the platform layer to
    // resume. generation() tells you whether GPU objects must be rebuilt.
}
```

**`present()` is `[[nodiscard]]` on purpose.** Ignoring it is how an application ends up frozen on its last frame after a phone call.

## Surface loss

Android takes the rendering surface away when your application goes to the background, and may or may not keep GPU objects alive when it comes back.

```cpp
const std::uint64_t generation = context.generation();
if (!context.resume()) { return; }
if (context.generation() != generation && !context.resources_preserved()) {
    rebuild_gpu_objects();
}
```

Atlases and sprite batches each have an `invalidate()` and `restore()` pair for this; see below.

## Choosing a backend

```sh
make SQUARED_GRAPHICS_BACKEND=null   # headless; the default
make SQUARED_GRAPHICS_BACKEND=gles   # EGL and GLES 3.0
```

The null backend draws nothing and succeeds at everything. A project builds and runs before a real backend exists, and `graphics` and `gui` can be tested without a GPU.

## Loading an atlas

```cpp
sq::graphics::TextureAtlas atlas;
if (!atlas.load(runtime.files.internal("skins/default/skin/uiskin.atlas"))) {
    return false;
}

const sq::graphics::AtlasRegion* button = atlas.find_region("default-round");
if (button != nullptr && button->region().valid()) {
    // draw with button->region()
}
```

Pages are resolved next to the atlas file, so the `.png` the atlas names is found wherever the atlas came from: inside the APK or on disk.

`find_region(name)` returns the first region with that name. `find_region(name, index)` selects among regions packed under the same name. Both return `nullptr` when there is none.

Filtering is taken from the atlas file. A pixel-art skin that asks for `Nearest` gets it, and nothing you pass can override that.

After context loss:

```cpp
void App::surface_created(sq::graphics::Context& graphics) {
    if (!graphics.resources_preserved()) {
        atlas_.invalidate();          // not release(): the old context is gone
        if (!atlas_.restore()) { /* the atlas could not be rebuilt */ }
    }
}
```

`restore()` reloads every page and re-validates every region for you. Any `AtlasRegion` you're holding is usable again afterwards, without being re-fetched.

## Drawing

```cpp
sq::graphics::SpriteBatch batch;
if (!batch.initialize()) { return false; }          // 2048 sprites by default

// each frame
if (batch.begin(camera)) {
    batch.draw(*atlas.find_region("default-round"), 10, 10, 64, 64);
    batch.draw(sprite);
    batch.end();
}
```

`begin()` returns false if the batch has no GPU objects: after context loss, before `restore()`. `end()` draws whatever is left queued.

**Which way is up comes from the camera.** `draw(region, x, y, w, h)` places the image's top edge at the top of the quad under either orientation:

| `camera.origin()` | `(x, y)` is the quad's | y grows |
|---|---|---|
| `CoordinateOrigin::TopLeft` (default) | top-left corner | downwards |
| `CoordinateOrigin::BottomLeft` | bottom-left corner | upwards |

```cpp
sq::graphics::OrthographicCamera ui_camera{720, 1280};    // TopLeft
sq::graphics::OrthographicCamera world_camera{
    720, 1280, sq::graphics::CoordinateOrigin::BottomLeft};
```

The GUI, glyph layout and `GlyphPlacement` all measure from the top, so use a `TopLeft` camera for them. A `Sprite`'s rotation is counter-clockwise under a `BottomLeft` camera, so it appears clockwise under a `TopLeft` one.

**Draw sprites that share a texture together.** The batch issues one draw call per run of sprites from the same texture. Ten sprites from one atlas cost one call; alternating between two textures six times costs six. Regions from the same atlas page share a texture, which is most of why atlases exist.

A region whose texture was discarded or reloaded is skipped rather than drawn, so a stale region leaves a gap instead of showing the wrong pixels.

After context loss:

```cpp
if (!graphics.resources_preserved()) {
    batch_.invalidate();
    if (!batch_.restore()) { /* the batch could not be rebuilt */ }
}
```

`invalidate()` abandons anything queued, so you don't need to call `end()` first.
