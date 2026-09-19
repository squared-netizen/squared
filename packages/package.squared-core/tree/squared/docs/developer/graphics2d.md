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

## Notes

`graphics2d/src/detail/bitmap_font_detail.hpp` holds the
BMFont parsing helpers shared by `bitmap_font.cpp` and `glyph_layout.cpp`.

`BitmapFont` and `GlyphLayout` both name their options struct only by
reference but use it in a defaulted argument, so those two headers include the
options definition rather than forward declaring it.
