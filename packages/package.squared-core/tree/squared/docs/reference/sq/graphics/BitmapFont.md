# class BitmapFont

*Defined at [graphics/bitmap_font.hpp:27](../../../../graphics/bitmap_font.hpp#L27)*



**brief** Transactionally parsed text BMFont resource.

 The resource owns only portable metadata and relative texture-page paths. Applications resolve those pages through their selected asset and graphics systems. The class is value-semantic and not thread-safe during mutation.



## Functions

### load

*public BitmapFontError load(int descriptor, const BitmapFontParseLimits & limits)*



**brief** Parse a text BMFont descriptor transactionally.

**descriptor** Complete UTF-8/ASCII BMFont text; not retained.

**limits** Positive resource bounds applied before allocation.

**return** The first failure, true when something failed; an empty error after committing a complete font. A failure leaves this font unchanged.

 `if( const auto error = font.load( text ) ) { ... }`

### clear

*public void clear()*



**brief** Remove all parsed metadata and return to the invalid state. 

### info

*public const BitmapFontInfo & info()*

*Defined at [graphics/bitmap_font.hpp:47](../../../../graphics/bitmap_font.hpp#L47)*

### line_height

*public int line_height()*

*Defined at [graphics/bitmap_font.hpp:48](../../../../graphics/bitmap_font.hpp#L48)*

### baseline

*public int baseline()*

*Defined at [graphics/bitmap_font.hpp:49](../../../../graphics/bitmap_font.hpp#L49)*

### page_width

*public int page_width()*

*Defined at [graphics/bitmap_font.hpp:50](../../../../graphics/bitmap_font.hpp#L50)*

### page_height

*public int page_height()*

*Defined at [graphics/bitmap_font.hpp:51](../../../../graphics/bitmap_font.hpp#L51)*

### pages

*public int pages()*

*Defined at [graphics/bitmap_font.hpp:52](../../../../graphics/bitmap_font.hpp#L52)*

### glyphs

*public int glyphs()*

*Defined at [graphics/bitmap_font.hpp:53](../../../../graphics/bitmap_font.hpp#L53)*

### kernings

*public int kernings()*

*Defined at [graphics/bitmap_font.hpp:54](../../../../graphics/bitmap_font.hpp#L54)*

### valid

*public bool valid()*



**brief** Return true when required metadata, pages, and glyphs exist. 

### glyph

*public const BitmapGlyph * glyph(char32_t codepoint)*



**brief** Find a glyph by Unicode scalar; returns null on a miss. 

### kerning

*public int kerning(char32_t first, char32_t second)*



**brief** Return the pair adjustment, or zero when no pair is declared. 



