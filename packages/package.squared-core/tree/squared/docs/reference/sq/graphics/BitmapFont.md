# class BitmapFont

*Defined at [graphics/bitmap_font.cppm:21](../../../../graphics/bitmap_font.cppm#L21)*

## Functions

### load

*public BitmapFontError load(std::string_view descriptor, const BitmapFontParseLimits & limits)*



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

*Defined at [graphics/bitmap_font.cppm:41](../../../../graphics/bitmap_font.cppm#L41)*

### line_height

*public int line_height()*

*Defined at [graphics/bitmap_font.cppm:42](../../../../graphics/bitmap_font.cppm#L42)*

### baseline

*public int baseline()*

*Defined at [graphics/bitmap_font.cppm:43](../../../../graphics/bitmap_font.cppm#L43)*

### page_width

*public int page_width()*

*Defined at [graphics/bitmap_font.cppm:44](../../../../graphics/bitmap_font.cppm#L44)*

### page_height

*public int page_height()*

*Defined at [graphics/bitmap_font.cppm:45](../../../../graphics/bitmap_font.cppm#L45)*

### pages

*public std::span<const BitmapFontPage> pages()*

*Defined at [graphics/bitmap_font.cppm:46](../../../../graphics/bitmap_font.cppm#L46)*

### glyphs

*public std::span<const BitmapGlyph> glyphs()*

*Defined at [graphics/bitmap_font.cppm:47](../../../../graphics/bitmap_font.cppm#L47)*

### kernings

*public std::span<const BitmapKerning> kernings()*

*Defined at [graphics/bitmap_font.cppm:48](../../../../graphics/bitmap_font.cppm#L48)*

### valid

*public bool valid()*



**brief** Return true when required metadata, pages, and glyphs exist. 

### glyph

*public const BitmapGlyph * glyph(char32_t codepoint)*



**brief** Find a glyph by Unicode scalar; returns null on a miss. 

### kerning

*public int kerning(char32_t first, char32_t second)*



**brief** Return the pair adjustment, or zero when no pair is declared. 



