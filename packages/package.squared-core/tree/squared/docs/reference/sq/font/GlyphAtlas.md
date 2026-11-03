# class GlyphAtlas

*Defined at [font/glyph_atlas.cppm:32](../../../../font/glyph_atlas.cppm#L32)*



**brief** The glyphs a terminal has drawn, rendered once into one 8-bit coverage image, for one texture: a glyph is rendered the first time it is asked for and found after that.

 Packed in shelves (rows of glyphs as tall as the tallest of the row), a pixel apart so a texture's filtering never bleeds. When it is full, or holds `capacity` glyphs, it starts again empty and `generation()` moves on: an owner keeping atlas places drops them. A glyph no face has is remembered too, so it is not looked for each frame.

 Memory, at construction only: width x height bytes (1 MB for the default 1024 x 1024: about 1,200 cells of 19 x 43, JetBrains Mono at 32 px), and 24 bytes a table slot (two slots a glyph: 96 KB for 2,048 glyphs). `glyph` never allocates.



 sq::font::GlyphAtlas atlas;                         // 1024 x 1024

 if( const auto g = atlas.glyph( fonts, U'A', {} ) ) { quad( g->x, g->y, g->width, g->height ); }

 if( !atlas.dirty().empty() ) { upload( atlas.pixels(), atlas.dirty() ); atlas.clear_dirty(); }





## Records

Rect



## Functions

### GlyphAtlas

*public void GlyphAtlas(std::uint16_t width, std::uint16_t height, std::uint16_t capacity)*

### clear

*public void clear()*

 Every glyph forgotten (after the fonts' size changed); `generation()` moves on. 

### clear_dirty

*public void clear_dirty()*

*Defined at [font/glyph_atlas.cppm:64](../../../../font/glyph_atlas.cppm#L64)*

### dirty

*public Rect dirty()*

*Defined at [font/glyph_atlas.cppm:63](../../../../font/glyph_atlas.cppm#L63)*

### generation

*public std::uint32_t generation()*

*Defined at [font/glyph_atlas.cppm:65](../../../../font/glyph_atlas.cppm#L65)*

### glyph

*public std::optional<AtlasGlyph> glyph(FontSet & fonts, char32_t code, GlyphStyle style)*

 Where `code` in `style` is, rendered by `fonts` now if it was not yet; nothing when no face has it. 

### height

*public std::uint16_t height()*

*Defined at [font/glyph_atlas.cppm:62](../../../../font/glyph_atlas.cppm#L62)*

### pixels

*public std::span<const std::uint8_t> pixels()*

*Defined at [font/glyph_atlas.cppm:57](../../../../font/glyph_atlas.cppm#L57)*

### size

*public std::size_t size()*

*Defined at [font/glyph_atlas.cppm:67](../../../../font/glyph_atlas.cppm#L67)*

 Glyphs held, those no face has among them. 

### width

*public std::uint16_t width()*

*Defined at [font/glyph_atlas.cppm:61](../../../../font/glyph_atlas.cppm#L61)*

