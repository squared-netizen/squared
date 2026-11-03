# class Face

*Defined at [font/face.cppm:36](../../../../font/face.cppm#L36)*



**brief** One font face (a .ttf, .otf, or one face of a .ttc), at one pixel size: its cell metrics, whether it has a character, and the character rendered as 8-bit coverage.

 FreeType 2.15 underneath (third_party/freetype/), with its auto-hinter in light mode (vertical hinting only: glyphs keep their shapes, stems land on whole pixels) and anti-aliasing. Each face has its own FreeType library, so faces on different threads never share state. A face is used by one thread at a time.

 Memory: FreeType's own allocations, from the C heap: about 100 KB a face opened from memory, more from a file (it keeps its own read buffer), and the rendered glyph's buffer, reused. Rendering allocates only when a glyph is larger than any before it.



 sq::font::Face face;

 if( const auto error = face.open_file( "fonts/JetBrainsMono-Regular.ttf" ) ) { report( error.what() ); }

 face.set_pixel_size( 32 );

 const auto cells = face.metrics();          // cell_width, cell_height, ascender ...

 if( const auto glyph = face.render( U'A' ) ) { upload( *glyph ); }





## Functions

### Face

*public void Face()*

*Defined at [font/face.cppm:39](../../../../font/face.cppm#L39)*

### Face

*public void Face(Face && other)*

### Face

*public void Face(const Face & )*

*Defined at [font/face.cppm:40](../../../../font/face.cppm#L40)*

### bold

*public bool bold()*

 The face is a bold one of its family. 

### family

*public std::string_view family()*

 The face's family ("JetBrains Mono"), empty when none is open. 

### has_glyph

*public bool has_glyph(char32_t code)*

 It has a glyph for `code` (else a fallback face draws it). 

### is_open

*public bool is_open()*

*Defined at [font/face.cppm:58](../../../../font/face.cppm#L58)*

### italic

*public bool italic()*

 The face is an italic one of its family. 

### metrics

*public FontMetrics metrics()*

*Defined at [font/face.cppm:59](../../../../font/face.cppm#L59)*

### open_file

*public FaceError open_file(std::string_view path, int index)*

 The face `index` of the font file at `path` (0 for a .ttf; a .ttc holds several). 

### open_memory

*public FaceError open_memory(std::span<const std::byte> file, int index)*

 The same from the file's bytes, which are not copied: they must outlive the face (an asset mapped from the APK, say).

### operator=

*public Face & operator=(Face && other)*

### operator=

*public Face & operator=(const Face & )*

*Defined at [font/face.cppm:41](../../../../font/face.cppm#L41)*

### render

*public std::optional<GlyphBitmap> render(char32_t code, GlyphStyle style)*

 `code` rendered, `style` applied where the face lacks it (bold on a regular face is emboldened; italic is slanted). Nothing when the face has no glyph for it or no size is set. Valid until the next `render`.

### set_pixel_size

*public FaceError set_pixel_size(std::uint16_t pixels)*

 Glyphs `pixels` high (the em size: about the height of a line); the metrics follow. 

### ~Face

*public void ~Face()*

