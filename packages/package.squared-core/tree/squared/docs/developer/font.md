# font &mdash; internals

Sprint 12.3a (2026-11-03): FreeType vendored, and `Face`, a face at a size
rendering coverage bitmaps. 12.3b (the same day): `FontSet` (the primary,
its bold, fallbacks from the system) and `GlyphAtlas` (glyphs rendered once
into one coverage image). Uploading the image to a texture is the drawing's
(12.3d): this module has no graphics dependency, so it is tested with no GL.

Programmer counterpart: [../programmer/font.md](../programmer/font.md)

## Decided (2026-11-03, the user)

- **FreeType** over stb_truetype: hinting and sharper small text, for a
  larger library to vendor (its latest stable release: 2.15.0).
- A font **bundled in sqterm's APK: JetBrains Mono, regular and bold**,
  italic made by slanting; CJK and other scripts it lacks from Android's
  own fonts (12.3b). The regular face is also this module's test font
  (`font/test/data/`, with its OFL licence).

## Layout

```
font/
  face.cppm face_error.cppm font_metrics.cppm glyph_bitmap.cppm glyph_style.cppm
  font_set.cppm glyph_atlas.cppm atlas_glyph.cppm module.cppm
  src/   face.cpp font_set.cpp glyph_atlas.cpp
         freetype/freetype/config/ftmodule.h ftoption.h   (squared's FreeType config)
  test/  font_test.cpp, data/JetBrainsMono-Regular.ttf, data/OFL.txt
  Makefile
third_party/freetype/   (pinned: tools/pin_freetype.sh; README.md, PINNED)
```

## FreeType, as squared builds it

- **Pinned, never edited.** `tools/pin_freetype.sh <source>` copies
  `include/` and the `.c` and `.h` files of nine modules (base, truetype,
  sfnt, smooth, autofit, cff, psaux, pshinter, psnames: 311 files, 5.8 MB)
  and writes `PINNED` with the tag, commit and a sha256 each; `make headers`
  checks them (tools/check_modules.sh, 1b). As the libc++ pin.
- **Configured from outside.** `font/src/freetype` comes first on the
  include path, so `<freetype/config/ftmodule.h>` and `ftoption.h` are
  squared's: the module list (autofit, truetype, cff, psaux, psnames,
  pshinter, sfnt, smooth) and FreeType's own options (its pinned `ftoption.h`,
  included by its path in the tree) with these turned off: zlib and LZW
  (compressed fonts, WOFF), Mac fonts, OpenType-SVG, BDF in TrueType,
  incremental loading, and `FREETYPE_PROPERTIES` from the environment.
- **Compiled into `libsquared_font.a`**, as data compiles yyjson: each
  module's one-file build (`truetype/truetype.c` and so on) and the base
  layer's files, 16 objects, as C11 with squared's warnings off (`-w`:
  FreeType's code is FreeType's to keep clean), through the build cache.
- **Not exported from an app.** FreeType marks its API
  `visibility("default")` itself (`config/public-macros.h`), which
  `-fvisibility=hidden` does not override, and Android's clang compiles
  position-independent executable code by default; so linked into an app's
  `.so`, its calls to its own API were refused by lld (`relocation
  R_AARCH64_ADR_PREL_PG_HI21 cannot be used against symbol TT_RunIns;
  recompile with -fPIC`, sqterm's first link, 2026-11-04). The package mk
  links with `-Wl,--exclude-libs,libsquared_font.a`: the archive's symbols
  are hidden in the `.so`, the calls bind inside it, and nothing of
  FreeType is exported. Chosen over `-fPIC` for FreeType (its 400-odd API
  functions exported and called through the PLT) and over a copy of
  `public-macros.h` of squared's (an edit of a pinned file in all but name).
- **Cost.** A program that renders one glyph, stripped, x86-64: 531 KB,
  nearly all FreeType (an empty program is 14 KB). Nothing for an app that
  never makes a `Face`: the archive's objects are only linked when called.
  Its heap: FreeType's own, from `malloc` (its default `FT_Memory`): about
  100 KB a face; a glyph's bitmap buffer, reused, grown only for a larger
  glyph.

## Face

32 bytes (`static_assert`ed): an `FT_Library` and an `FT_Face` as `void *`
(FreeType's types stay out of the interface: a forward declaration inside a
module would attach them to the module, and FreeType's own headers in the
implementation's global module fragment would then name other types),
`FontMetrics`, a flag. One library per face: FreeType's library is not
thread-safe across faces that share it, and a terminal's few faces (regular,
bold, a fallback) make the cost a few KB each.

- **Sizing.** `FT_Set_Pixel_Sizes(0, pixels)`; ascender rounded up,
  descender down, the cell's height the larger of the two's span and the
  face's line height; the cell's width the advance of `M` (a monospace face
  has one advance), else the face's widest; the underline from the face's
  values scaled by `y_scale`.
- **Rendering.** `FT_Load_Glyph` with `FT_LOAD_TARGET_LIGHT` (the
  auto-hinter, vertically only: shapes kept, stems on whole pixels: the
  usual choice for a terminal), then `FT_GlyphSlot_Oblique` and
  `FT_GlyphSlot_Embolden` (FreeType's `ftsynth`) where the style asks for
  what the face is not, then `FT_Render_Glyph` (normal: 8-bit
  anti-aliased). The `GlyphBitmap` points into the slot's buffer. Colour
  bitmaps (emoji) and 1-bit ones are refused for now.
- **No exceptions, no error strings from FreeType:** FreeType's integer
  error goes back in `FaceError::freetype_error`; the text is squared's
  fixed one.

## FontSet

Up to 8 regular faces and one bold, held by value (`std::array<Face, 8>`:
fixed, no allocation; 320 bytes with the bold one and the size).
`choose` asks each face `FT_Get_Char_Index` in order: for a character every
face lacks, that is 8 lookups, which the atlas spares by remembering a
miss. A face added after `set_pixel_size` is sized on the way in. The
fallback list is fixed paths probed with `std::filesystem::is_regular_file`
(over parsing Android's `/system/etc/fonts.xml`: the list a terminal needs
is short and stable; fonts.xml's format changed across Android releases).
`open_file` reads a font through FreeType's stream, on demand: a 20 MB
Noto CJK collection is not read whole.

## GlyphAtlas

- **Storage**, at construction only: the image (`unique_ptr<uint8_t[]>`,
  zeroed: the gaps must be empty) and an open-addressed table of
  `bit_ceil(2 x capacity)` slots (24 bytes each: the code, the style bits,
  a state, the `AtlasGlyph`), linear probing, a multiplicative hash.
  Over `std::unordered_map` (a node per glyph, priority 1); no erase is
  ever needed: the table empties all at once.
- **Packing**: shelves, left to right, a new shelf below when a row is
  full; each glyph a pixel from the next, so linear filtering never mixes
  two. A monospace font's glyphs are about one height, so shelves waste
  little.
- **Full** (no room, or `capacity` glyphs held): everything goes, the image
  is zeroed, `generation` moves on, and the glyph asked for is placed first.
  For a terminal's few hundred glyphs at once this is rare; an LRU would
  cost a list per glyph and repacking. A glyph larger than the image is
  remembered as missing.
- **Dirty**: the union of what was written since `clear_dirty`, one
  rectangle (a texture update of one sub-image).
- **Missing** characters are kept as such, so a program printing one each
  frame does not make every face look for it each frame.

## Tests

`font/test/font_test.cpp`, on JetBrains Mono Regular from `test/data/`
(tests run in the module's directory): opening from a file and from
memory; the metrics at 20 and 40 px (cell 12 and 24 wide, the height
doubling); 'A' (ink from its first row to its last, the advance a cell),
'g' below the baseline, a space with no ink, Cyrillic and box drawing
present, CJK absent; bold (over a tenth more ink, no narrower) and italic
(wider, about the same ink) from the regular face; a .ttc index past the
end, junk, a missing file; moves; reopening. `FontSet`: the primary, CJK
absent and then drawn by the system's Noto Sans CJK (checked where it is
installed: here Debian's, on the phone Android's; said when it is not),
added after sizing and so sized, bold by emboldening and by a bold face,
a full set. `GlyphAtlas`: a glyph's coverage in the image as the face
renders it, the dirty rectangle, found again unchanged, bold and italic as
other glyphs, a space, a missing character remembered, no two glyphs
overlapping, starting again when full and at capacity, a glyph larger than
the image, `clear`. Run here as built and under AddressSanitizer and UBSan;
clang-tidy 21 on the module: no findings.

## Known limits

- No colour glyphs (emoji): FreeType reads CBDT and COLR, but needs libpng
  for CBDT and a colour path in the renderer; a later step.
- No shaping (ligatures, combining marks placed): one code point, one
  glyph, as a terminal draws them.
