# class FontResource

*Defined at [gui/src/font_resource.cppm:18](../../../../gui/src/font_resource.cppm#L18)*



**brief** Immutable portable bitmap-font resource selected by GUI styles.

 A descriptor-only resource records the safe asset path imported from a skin. A resolved resource additionally owns parsed BMFont metrics and non-owning texture-page regions. The textures referenced by those regions must outlive the resource. FontResource performs no I/O and has no HoloDisk or rendering-backend dependency.



## Functions

### FontResource

*public void FontResource(std::string descriptor_path)*



**brief** Construct a descriptor-only resource.

**descriptor_path** Contained relative BMFont descriptor path.

**throws**when the path is empty, absolute, contains a backslash, or contains an empty, `.` or `..` component.

### FontResource

*public void FontResource(std::string descriptor_path, graphics::BitmapFont font, std::vector<graphics::TextureRegion> pages, float scale)*



**brief** Construct a resolved bitmap-font resource.

**descriptor_path** Contained relative descriptor identity.

**font** Valid parsed BMFont value transferred into the resource.

**pages** Page regions in BMFont page-id order. Each region must be at least as large as the descriptor page dimensions and its texture must outlive this resource.

**scale** Uniform logical-unit scale; finite and greater than zero.

**throws**when any invariant is not satisfied.

### bitmap_font

*public const graphics::BitmapFont * bitmap_font()*

*Defined at [gui/src/font_resource.cppm:52](../../../../gui/src/font_resource.cppm#L52)*



**brief** Return parsed metrics, or null for a descriptor-only resource. 

### descriptor_path

*public const std::string & descriptor_path()*

*Defined at [gui/src/font_resource.cppm:43](../../../../gui/src/font_resource.cppm#L43)*



**brief** Return the descriptor asset path owned by this resource. 

### glyph_region

*public graphics::TextureRegion glyph_region(const graphics::GlyphPlacement & glyph)*



**brief** Derive the texture view for one placement from this font.

**glyph** Placement produced from this resource's BitmapFont.

**return** Non-owning page subregion, or an empty region when unresolved or when the placement does not fit its declared page.

### pages

*public std::span<const graphics::TextureRegion> pages()*

*Defined at [gui/src/font_resource.cppm:58](../../../../gui/src/font_resource.cppm#L58)*



**brief** Return page regions in BMFont page-id order. 

### resolved

*public bool resolved()*

*Defined at [gui/src/font_resource.cppm:49](../../../../gui/src/font_resource.cppm#L49)*



**brief** Return true when parsed metrics and matching pages are present. 

### scale

*public float scale()*

*Defined at [gui/src/font_resource.cppm:73](../../../../gui/src/font_resource.cppm#L73)*



**brief** Return the uniform conversion from source pixels to GUI units. 

