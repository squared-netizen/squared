# class GlyphLayout

*Defined at [graphics/glyph_layout.cppm:15](../../../../graphics/glyph_layout.cppm#L15)*



**brief** Reusable value-semantic layout of UTF-8 text into glyph quads. 



## Functions

### clear

*public void clear()*



**brief** Empty the layout, keeping its storage; assign GlyphLayout{} to release it. 

### glyphs

*public std::span<const GlyphPlacement> glyphs()*

*Defined at [graphics/glyph_layout.cppm:46](../../../../graphics/glyph_layout.cppm#L46)*

### height

*public float height()*

*Defined at [graphics/glyph_layout.cppm:44](../../../../graphics/glyph_layout.cppm#L44)*

### line_height

*public float line_height()*

*Defined at [graphics/glyph_layout.cppm:45](../../../../graphics/glyph_layout.cppm#L45)*

### lines

*public std::span<const GlyphLine> lines()*

*Defined at [graphics/glyph_layout.cppm:47](../../../../graphics/glyph_layout.cppm#L47)*

### reserve

*public void reserve(std::size_t glyphs, std::size_t lines)*



**brief** Make room for `glyphs` glyphs and `lines` lines now, so a text that fits lays out with no allocation: for a layout kept to be reused, made where allocating is expected.

### set_text

*public BitmapFontError set_text(const BitmapFont & font, std::string_view text, const GlyphLayoutOptions & options)*



**brief** Decode and lay out explicit newline-delimited UTF-8 text.

**font** Valid parsed font; borrowed for this call only.

**text** UTF-8 bytes; not retained.

**options** Scale, line-box alignment, replacement, and tab policy.

**return** The failure, true when something failed; an empty error after replacing the layout. A failure leaves the prior layout unchanged. Building and returning the error never allocates.

 The layout's storage is reused: a call allocates only when this text needs more glyphs or lines than any earlier one did. Keep one layout and call this again rather than making a new one per call.

### width

*public float width()*

*Defined at [graphics/glyph_layout.cppm:43](../../../../graphics/glyph_layout.cppm#L43)*

