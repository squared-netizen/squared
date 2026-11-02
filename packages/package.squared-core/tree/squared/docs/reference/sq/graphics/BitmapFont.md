# class BitmapFont

*Defined at [graphics/bitmap_font.cppm:21](../../../../graphics/bitmap_font.cppm#L21)*



**brief** Transactionally parsed text BMFont resource.

 The resource owns only portable metadata and relative texture-page paths. Applications resolve those pages through their selected asset and graphics systems. The class is value-semantic and not thread-safe during mutation.



## Functions

### baseline

*public int baseline()*

*Defined at [graphics/bitmap_font.cppm:43](../../../../graphics/bitmap_font.cppm#L43)*

### clear

*public void clear()*



**brief** Remove all parsed metadata and return to the invalid state. 

### glyph

*public const BitmapGlyph * glyph(char32_t codepoint)*



**brief** Find a glyph by Unicode scalar; returns null on a miss. 

### glyphs

*public std::span<const BitmapGlyph> glyphs()*

*Defined at [graphics/bitmap_font.cppm:47](../../../../graphics/bitmap_font.cppm#L47)*

### info

*public const BitmapFontInfo & info()*

*Defined at [graphics/bitmap_font.cppm:41](../../../../graphics/bitmap_font.cppm#L41)*

### kerning

*public int kerning(char32_t first, char32_t second)*



**brief** Return the pair adjustment, or zero when no pair is declared. 

### kernings

*public std::span<const BitmapKerning> kernings()*

*Defined at [graphics/bitmap_font.cppm:48](../../../../graphics/bitmap_font.cppm#L48)*

### line_height

*public int line_height()*

*Defined at [graphics/bitmap_font.cppm:42](../../../../graphics/bitmap_font.cppm#L42)*

### load

*public BitmapFontError load(std::string_view descriptor, const BitmapFontParseLimits & limits)*



**brief** Parse a text BMFont descriptor transactionally.

**descriptor** Complete UTF-8/ASCII BMFont text; not retained.

**limits** Positive resource bounds applied before allocation.

**return** The first failure, true when something failed; an empty error after committing a complete font. A failure leaves this font unchanged.

 `if( const auto error = font.load( text ) ) { ... }`

### page_height

*public int page_height()*

*Defined at [graphics/bitmap_font.cppm:45](../../../../graphics/bitmap_font.cppm#L45)*

### page_width

*public int page_width()*

*Defined at [graphics/bitmap_font.cppm:44](../../../../graphics/bitmap_font.cppm#L44)*

### pages

*public std::span<const BitmapFontPage> pages()*

*Defined at [graphics/bitmap_font.cppm:46](../../../../graphics/bitmap_font.cppm#L46)*

### valid

*public bool valid()*



**brief** Return true when required metadata, pages, and glyphs exist. 

