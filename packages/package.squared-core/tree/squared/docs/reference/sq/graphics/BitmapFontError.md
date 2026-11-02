# struct BitmapFontError

*Defined at [graphics/src/bitmap_font_error.cppm:17](../../../../graphics/src/bitmap_font_error.cppm#L17)*



**brief** What went wrong in `BitmapFont::load` or `GlyphLayout::set_text`.

 Returned by value, and true when something failed: `if( const auto error = font.load( text ) ) { ... }`. It owns nothing: the texts are fixed strings in the library, so building, copying or returning one never allocates, even on the per-frame text path.



## Members

public const char * message

public const char * field

public std::uint32_t line

public std::uint32_t byte

public BitmapFontErrorCode code



## Functions

### operator bool

*public bool operator bool()*

*Defined at [graphics/src/bitmap_font_error.cppm:31](../../../../graphics/src/bitmap_font_error.cppm#L31)*



**brief** True when something failed. 

