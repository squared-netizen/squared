# class GlyphLayout

*Defined at [graphics/glyph_layout.hpp:18](../../../../graphics/glyph_layout.hpp#L18)*



**brief** Reusable value-semantic layout of UTF-8 text into glyph quads. 



## Functions

### set_text

*public BitmapFontError set_text(const BitmapFont & font, std::string_view text, const GlyphLayoutOptions & options)*



**brief** Decode and lay out explicit newline-delimited UTF-8 text.

**font** Valid parsed font; borrowed for this call only.

**text** UTF-8 bytes; not retained.

**options** Scale, line-box alignment, replacement, and tab policy.

**return** The failure, true when something failed; an empty error after replacing the layout. A failure leaves the prior layout unchanged. Building and returning the error never allocates.

 The layout's storage is reused: a call allocates only when this text needs more glyphs or lines than any earlier one did. Keep one layout and call this again rather than making a new one per call.

### clear

*public void clear()*



**brief** Empty the layout, keeping its storage; assign GlyphLayout{} to release it. 

### width

*public float width()*

*Defined at [graphics/glyph_layout.hpp:39](../../../../graphics/glyph_layout.hpp#L39)*

### height

*public float height()*

*Defined at [graphics/glyph_layout.hpp:40](../../../../graphics/glyph_layout.hpp#L40)*

### line_height

*public float line_height()*

*Defined at [graphics/glyph_layout.hpp:41](../../../../graphics/glyph_layout.hpp#L41)*

### glyphs

*public std::span<const GlyphPlacement> glyphs()*

*Defined at [graphics/glyph_layout.hpp:42](../../../../graphics/glyph_layout.hpp#L42)*

### lines

*public std::span<const GlyphLine> lines()*

*Defined at [graphics/glyph_layout.hpp:43](../../../../graphics/glyph_layout.hpp#L43)*



