# class FontResource

*Defined at [gui/src/font_resource.cppm:18](../../../../gui/src/font_resource.cppm#L18)*

## Functions

### FontResource

*public void FontResource(std::string descriptor_path)*

### FontResource

*public void FontResource(std::string descriptor_path, graphics::BitmapFont font, std::vector<graphics::TextureRegion> pages, float scale)*

### descriptor_path

*public const std::string & descriptor_path()*

*Defined at [gui/src/font_resource.cppm:43](../../../../gui/src/font_resource.cppm#L43)*

### resolved

*public bool resolved()*

*Defined at [gui/src/font_resource.cppm:49](../../../../gui/src/font_resource.cppm#L49)*

### bitmap_font

*public const graphics::BitmapFont * bitmap_font()*

*Defined at [gui/src/font_resource.cppm:52](../../../../gui/src/font_resource.cppm#L52)*

### glyph_region

*public graphics::TextureRegion glyph_region(const graphics::GlyphPlacement & glyph)*

### pages

*public std::span<const graphics::TextureRegion> pages()*

*Defined at [gui/src/font_resource.cppm:58](../../../../gui/src/font_resource.cppm#L58)*

### scale

*public float scale()*

*Defined at [gui/src/font_resource.cppm:73](../../../../gui/src/font_resource.cppm#L73)*



