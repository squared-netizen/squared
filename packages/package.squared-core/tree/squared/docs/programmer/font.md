# font

Font faces read and rendered at run time: a TrueType or OpenType file (or
one face of a .ttc collection), at a pixel size, its cell metrics, and any
character it has as 8-bit coverage, ready to put in a texture. For sqterm's
terminal (Sprint 12.3), and any app that draws text from a font file rather
than the skin's pre-baked bitmap fonts. FreeType 2.15 does the work
(`third_party/freetype/`).

`import sq.font;` (it is not part of `import squared;`).

Developer counterpart: [../developer/font.md](../developer/font.md)

| Type | File | Purpose |
|---|---|---|
| `Face` | `squared/font/face.cppm` | One face at one size: open it, size it, its metrics, whether it has a character, the character rendered |
| `FontMetrics` | `squared/font/font_metrics.cppm` | A cell's width and height, the baseline, how far below it glyphs reach, the underline |
| `GlyphBitmap` | `squared/font/glyph_bitmap.cppm` | A rendered glyph: coverage rows, size, bearings, advance |
| `GlyphStyle` | `squared/font/glyph_style.cppm` | Bold and italic, made from the face when it lacks them |
| `FaceError`, `FaceErrorCode` | `squared/font/face_error.cppm` | Why opening or sizing failed |
| `FontSet` | `squared/font/font_set.cppm` | The faces a terminal draws with: the primary, its bold, fallbacks for what it lacks |
| `GlyphAtlas` | `squared/font/glyph_atlas.cppm` | Glyphs rendered once into one coverage image, for one texture |
| `AtlasGlyph` | `squared/font/atlas_glyph.cppm` | Where a glyph is in the atlas, and how it sits on the baseline |

## The shortest use

```cpp
sq::font::Face face;
if( const auto error = face.open_file( "fonts/JetBrainsMono-Regular.ttf" ) ) {
    report( error.what() );
}
(void)face.set_pixel_size( 32 );
const sq::font::FontMetrics cell = face.metrics();     // 19 x 43 for JetBrains Mono at 32
if( const auto glyph = face.render( U'A' ) ) {
    for( std::uint16_t y = 0; y < glyph->height; ++y ) {
        copy_row( glyph->row( y ), glyph->left, cell.ascender - glyph->top + y );
    }
}
```

## Face

| Call | Does |
|---|---|
| `open_file(path, index)` | opens face `index` (0 for a .ttf or .otf; a .ttc holds several) of the file at `path` |
| `open_memory(file, index)` | the same from the file's bytes, which are not copied: they must outlive the face (an asset mapped from the APK) |
| `set_pixel_size(pixels)` | glyphs `pixels` high (the em: about a line's height); the metrics follow |
| `metrics()` | `FontMetrics` at that size (all zero before a size is set) |
| `family()`, `bold()`, `italic()` | the family name; whether this face is its family's bold or italic |
| `has_glyph(code)` | it has a glyph for `code`; if not, a fallback face draws it |
| `render(code, style)` | `code` rendered as `GlyphBitmap`; `style.bold` emboldens a face that is not bold, `style.italic` slants one that is not italic. Nothing when the face lacks the character, no size is set, or the glyph is not 8-bit coverage (a colour emoji). Valid until the next `render` on the face |

Opening again closes what was open. Moved, not copied. One thread at a time
uses a face; faces on different threads are independent.

Failure modes (`FaceError::code`, with FreeType's own error in
`freetype_error`): `library` (FreeType could not start: out of memory),
`open` (missing, or not a font FreeType reads), `no_such_face` (no face of
that index), `size` (0, or a size a bitmap-only font lacks), `not_open`,
`set_full` (a `FontSet` already holds `max_faces`).

## FontSet

```cpp
sq::font::FontSet fonts;
(void)fonts.add( std::move( regular ) );          // the primary: its cell is the grid's
(void)fonts.set_bold( std::move( bold ) );
fonts.add_system_fallbacks();                     // Noto CJK and symbols, where they are
(void)fonts.set_pixel_size( 32 );
```

| Call | Does |
|---|---|
| `add(face)` | a regular face (opened), after those before; the first is the primary. Sized to the set's size if one is set |
| `set_bold(face)` | the primary's bold face |
| `add_system_fallbacks()` | opens and adds each of `system_fallback_paths()` that exists here, in order, until the set is full; returns how many |
| `set_pixel_size(pixels)` | every face at `pixels`; `metrics()` is the primary's |
| `choose(code, bold)` | which face draws `code`: for bold the bold face when it has it, else the first face that does; nothing when none has it |
| `render(code, style, chosen)` | `code` rendered by that face, `style` made where the face lacks it; `chosen` (optional) says which |
| `size()`, `has_bold()`, `max_faces` (8) | faces held |

`system_fallback_paths()`, in order: Android's Noto Sans CJK and Noto Sans
SC, the Droid Sans Fallback fonts of Android 6 and before (the 5.1
tablet's), Noto Sans Symbols; then a Debian desktop's Noto Sans CJK and
DejaVu. A wide character drawn by a fallback is about an em wide, less than
its two cells: centre it in them.

## GlyphAtlas

```cpp
sq::font::GlyphAtlas atlas;                       // 1024 x 1024, 2,048 glyphs
const auto g = atlas.glyph( fonts, U'A', { .bold = cell.attributes.bold } );
if( g ) { quad( g->x, g->y, g->width, g->height, pen_x + g->left, baseline - g->top ); }
if( !atlas.dirty().empty() ) {
    texture.update( atlas.pixels(), atlas.dirty() );   // the changed rectangle only
    atlas.clear_dirty();
}
```

| Call | Does |
|---|---|
| `GlyphAtlas(width, height, capacity)` | an empty image (1024 x 1024 bytes) and room for `capacity` glyphs (2,048) |
| `glyph(fonts, code, style)` | where `code` in `style` is: rendered by `fonts` the first time, found after that; nothing when no face has it (remembered, so asking again costs a lookup) |
| `pixels()`, `width()`, `height()` | the coverage image, row by row |
| `dirty()`, `clear_dirty()` | the rectangle changed since the last `clear_dirty` (all of it at first): upload it |
| `generation()` | moves on whenever the atlas starts again (full, at capacity, or `clear`): every place taken before is gone |
| `clear()` | everything forgotten: after `set_pixel_size` |
| `size()` | glyphs held |

Never allocates after construction. One thread at a time.

## FontMetrics

| Field | Meaning |
|---|---|
| `cell_width` | the face's advance: every cell of a monospace grid |
| `cell_height` | ascender to descender, or the face's line height if larger |
| `ascender` | the baseline, down from a cell's top |
| `descender` | how far below the baseline glyphs reach (negative) |
| `underline_position`, `underline_thickness` | the underline's top below the baseline, and its thickness |

## GlyphBitmap

`coverage` is `height` rows of `pitch` bytes (`row(y)` gives row `y`,
`width` bytes, from the top), 0 for none to 255 for full. The bitmap's top
left goes `left` right of the pen and `top` above the baseline; the pen then
moves `advance`. A space has an advance and no pixels.

## Licence

FreeType's licence (the FTL) asks that a program using it says so in its
documentation: "Portions of this software are copyright © The FreeType
Project (https://freetype.org). All rights reserved." An app that links
`sq.font` carries that line in its own documentation or about screen.
