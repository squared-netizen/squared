# struct GlyphPlacement

*Defined at [graphics/src/glyph_placement.hpp:15](../../../../graphics/src/glyph_placement.hpp#L15)*



**brief** One SpriteBatch-ready glyph quad in top-left logical coordinates.

 `page`, source coordinates, and destination geometry are values. A renderer resolves the page, creates a TextureRegion for the source rectangle, and submits it to SpriteBatch at `(x, y, width, height)`.



## Members

public char32_t codepoint

public int page

public int source_x

public int source_y

public int source_width

public int source_height

public float x

public float y

public float width

public float height

public float advance



