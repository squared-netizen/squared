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
that index), `size` (0, or a size a bitmap-only font lacks), `not_open`.

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
