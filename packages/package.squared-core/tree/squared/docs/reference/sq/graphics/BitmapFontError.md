# struct BitmapFontError

*Defined at [graphics/src/bitmap_font_error.hpp:19](../../../../graphics/src/bitmap_font_error.hpp#L19)*



**brief** What went wrong in `BitmapFont::load` or `GlyphLayout::set_text`.

 Returned by value, and true when something failed: `if( const auto error = font.load( text ) ) { ... }`. It owns nothing: the texts are fixed strings in the library, so building, copying or returning one never allocates, even on the per-frame text path.



## Members

public const char * message

public const char * field

public int line

public int byte

public BitmapFontErrorCode code



## Functions

### operator bool

*public bool operator bool()*

*Defined at [graphics/src/bitmap_font_error.hpp:33](../../../../graphics/src/bitmap_font_error.hpp#L33)*



**brief** True when something failed. 



