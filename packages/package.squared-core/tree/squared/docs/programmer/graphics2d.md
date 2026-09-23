# graphics2d

Textures, atlases, regions, sprites, bitmap fonts and the 2D camera.

Every type below lives in its own header. Include exactly the ones a
translation unit names; `squared/graphics2d/graphics2d.hpp` pulls in all of them
and exists for convenience, not for use inside headers of your own.

Developer counterpart: [../developer/graphics2d.md](../developer/graphics2d.md)

| Type | Header | Purpose |
|---|---|---|
| `AtlasRegion` | `squared/graphics2d/atlas_region.hpp` | One named region and its libGDX-compatible atlas metadata |
| `BitmapFont` | `squared/graphics2d/bitmap_font.hpp` | Transactionally parsed text BMFont resource |
| `BitmapFontError` | `squared/graphics2d/bitmap_font_error.hpp` | One owned BMFont parsing or glyph-layout failure |
| `BitmapFontErrorCode` | `squared/graphics2d/bitmap_font_error_code.hpp` | Stable failure categories for BMFont parsing and glyph layout |
| `BitmapFontInfo` | `squared/graphics2d/bitmap_font_info.hpp` | Portable BMFont face metadata |
| `BitmapFontPage` | `squared/graphics2d/bitmap_font_page.hpp` | One texture-page dependency declared by a BMFont |
| `BitmapFontParseLimits` | `squared/graphics2d/bitmap_font_parse_limits.hpp` | Explicit resource limits for one text BMFont descriptor |
| `BitmapGlyph` | `squared/graphics2d/bitmap_glyph.hpp` | One Unicode scalar's source rectangle and layout metrics |
| `BitmapKerning` | `squared/graphics2d/bitmap_kerning.hpp` | Pair-specific horizontal advance adjustment in source pixels |
| `CoordinateOrigin` | `squared/graphics2d/coordinate_origin.hpp` | Coordinate orientation used by an orthographic camera |
| `GlyphAlignment` | `squared/graphics2d/glyph_alignment.hpp` | Horizontal placement within an optional target width |
| `GlyphLayout` | `squared/graphics2d/glyph_layout.hpp` | Reusable value-semantic layout of UTF-8 text into glyph quads |
| `GlyphLayoutOptions` | `squared/graphics2d/glyph_layout_options.hpp` | Controls for one UTF-8 glyph layout operation |
| `GlyphLine` | `squared/graphics2d/glyph_line.hpp` | One explicit line in a GlyphLayout |
| `GlyphPlacement` | `squared/graphics2d/glyph_placement.hpp` | One SpriteBatch-ready glyph quad in top-left logical coordinates |
| `OrthographicCamera` | `squared/graphics2d/orthographic_camera.hpp` | Two-dimensional orthographic camera with pan and zoom |
| `Sprite` | `squared/graphics2d/sprite.hpp` | Lightweight mutable state for drawing one TextureRegion |
| `SpriteBatch` | `squared/graphics2d/sprite_batch.hpp` | Efficiently draw ordered textured quads with the selected backend |
| `Texture` | `squared/graphics2d/texture.hpp` | Move-only texture owned by the selected graphics backend |
| `TextureAtlas` | `squared/graphics2d/texture_atlas.hpp` | Owning, transactionally loaded libGDX text texture atlas |
| `TextureFilter` | `squared/graphics2d/texture_filter.hpp` | Texture filtering mode |
| `TextureRecoveryCallback` | `squared/graphics2d/texture_recovery_callback.hpp` |  |
| `TextureRecoveryOptions` | `squared/graphics2d/texture_recovery_options.hpp` | Recovery selection supplied when a texture is created |
| `TextureRecoveryPolicy` | `squared/graphics2d/texture_recovery_policy.hpp` | CPU-side recipe used to recover a texture after context loss |
| `TextureRecoveryTarget` | `squared/graphics2d/texture_recovery_target.hpp` | Synchronous destination supplied to a regeneration callback |
| `TextureRegion` | `squared/graphics2d/texture_region.hpp` | Non-owning rectangular view into a Texture |
| `TextureWrap` | `squared/graphics2d/texture_wrap.hpp` | Texture coordinate wrapping mode |

## Loading an atlas

```cpp
sq::graphics2d::TextureAtlas atlas;
if (!atlas.load(runtime.files.internal("skins/default/skin/uiskin.atlas"))) {
    return false;
}

const sq::graphics2d::AtlasRegion* button = atlas.find_region("default-round");
if (button != nullptr && button->region().valid()) {
    // draw with button->region()
}
```

Pages are resolved next to the atlas file, so the `.png` the atlas names is
found wherever the atlas came from - inside the APK or on disk.

`find_region(name)` returns the first region with that name;
`find_region(name, index)` selects among regions packed under the same name.
It returns `nullptr` when there is none.

Filtering is taken from the atlas file. A pixel-art skin that asks for
`Nearest` gets it, and nothing you pass can override that.

After context loss:

```cpp
void App::surface_created(sq::graphics::Context& graphics) {
    if (!graphics.resources_preserved()) {
        atlas_.invalidate();          // not release(): the old context is gone
        if (!atlas_.restore()) { /* the atlas could not be rebuilt */ }
    }
}
```

`restore()` reloads every page and re-validates every region for you. Any
`AtlasRegion` you are holding is usable again afterwards, without being
re-fetched.
