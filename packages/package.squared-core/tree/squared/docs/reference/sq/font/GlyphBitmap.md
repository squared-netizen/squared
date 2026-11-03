# struct GlyphBitmap

*Defined at [font/glyph_bitmap.cppm:15](../../../../font/glyph_bitmap.cppm#L15)*



**brief** One rendered glyph: 8-bit coverage, 0 (none) to 255 (full), row by row from the top, and where it goes from the pen.

 It points into the face's own buffer: valid until the face renders another glyph. A space has no pixels and only an advance.



## Members

public span coverage

public std::uint16_t width

public std::uint16_t height

public std::uint16_t pitch

public std::int16_t left

public std::int16_t top

public std::int16_t advance



## Functions

### row

*public std::span<const std::uint8_t> row(std::uint16_t y)*

*Defined at [font/glyph_bitmap.cppm:30](../../../../font/glyph_bitmap.cppm#L30)*

 Row `y`, from the top; `y` is below `height`. 

