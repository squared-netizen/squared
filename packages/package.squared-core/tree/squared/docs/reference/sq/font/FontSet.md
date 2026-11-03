# class FontSet

*Defined at [font/font_set.cppm:26](../../../../font/font_set.cppm#L26)*



**brief** The faces a terminal draws with: a primary face (its cell size is the grid's), its bold face if there is one, and fallback faces for what the primary lacks (CJK, symbols), tried in order.

 Up to `max_faces` regular faces, one bold; all at one pixel size. Holds the faces themselves (32 bytes each), so 320 bytes and FreeType's memory for each face opened.



 sq::font::FontSet fonts;

 (void)fonts.add( std::move( jetbrains_mono ) );        // the primary

 (void)fonts.set_bold( std::move( jetbrains_mono_bold ) );

 fonts.add_system_fallbacks();                          // Noto CJK and the rest

 (void)fonts.set_pixel_size( 32 );





## Members

public static const std::size_t max_faces



## Records

Choice



## Functions

### add

*public FaceError add(Face face)*

 A regular face, after those added before; the first is the primary. 

### add_system_fallbacks

*public std::size_t add_system_fallbacks()*

 The system's fallback fonts that exist here, in order (Android's Noto CJK, or the 5.1 tablet's Droid Sans Fallback, then symbols; on Linux, Noto CJK and DejaVu): how many were added.

### choose

*public std::optional<Choice> choose(char32_t code, bool bold)*

 The face for `code`: for bold, the bold face when it has it; else the first that does. 

### has_bold

*public bool has_bold()*

*Defined at [font/font_set.cppm:56](../../../../font/font_set.cppm#L56)*

### metrics

*public FontMetrics metrics()*

*Defined at [font/font_set.cppm:53](../../../../font/font_set.cppm#L53)*

### render

*public std::optional<GlyphBitmap> render(char32_t code, GlyphStyle style, Choice * chosen)*

 `code` rendered by the face `choose` picks, `style` made where that face lacks it. Nothing when no face has it. Valid until the next render by the same face.

### set_bold

*public FaceError set_bold(Face face)*

 The primary's bold face. 

### set_pixel_size

*public FaceError set_pixel_size(std::uint16_t pixels)*

 Every face at `pixels`; the metrics are the primary's. 

### size

*public std::size_t size()*

*Defined at [font/font_set.cppm:55](../../../../font/font_set.cppm#L55)*

 Regular faces held, the primary among them. 

### system_fallback_paths

*public static std::span<const std::string_view> system_fallback_paths()*

 Paths `add_system_fallbacks` looks for, in order. 

