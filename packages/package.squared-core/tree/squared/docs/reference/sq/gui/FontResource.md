# class FontResource

*Defined at [gui/src/font_resource.hpp:28](../../../../gui/src/font_resource.hpp#L28)*



**brief** Immutable portable bitmap-font resource selected by GUI styles.

 A descriptor-only resource records the safe asset path imported from a skin. A resolved resource additionally owns parsed BMFont metrics and non-owning texture-page regions. The textures referenced by those regions must outlive the resource. FontResource performs no I/O and has no HoloDisk or rendering-backend dependency.



## Functions

### FontResource

*public void FontResource(int descriptor_path)*



**brief** Construct a descriptor-only resource.

**descriptor_path** Contained relative BMFont descriptor path.

**throws**when the path is empty, absolute, contains a backslash, or contains an empty, `.` or `..` component.

### FontResource

*public void FontResource(int descriptor_path, graphics::BitmapFont font, int pages, float scale)*



**brief** Construct a resolved bitmap-font resource.

**descriptor_path** Contained relative descriptor identity.

**font** Valid parsed BMFont value transferred into the resource.

**pages** Page regions in BMFont page-id order. Each region must be at least as large as the descriptor page dimensions and its texture must outlive this resource.

**scale** Uniform logical-unit scale; finite and greater than zero.

**throws**when any invariant is not satisfied.

### descriptor_path

*public const int & descriptor_path()*

*Defined at [gui/src/font_resource.hpp:53](../../../../gui/src/font_resource.hpp#L53)*



**brief** Return the descriptor asset path owned by this resource. 

### resolved

*public bool resolved()*

*Defined at [gui/src/font_resource.hpp:59](../../../../gui/src/font_resource.hpp#L59)*



**brief** Return true when parsed metrics and matching pages are present. 

### bitmap_font

*public const graphics::BitmapFont * bitmap_font()*

*Defined at [gui/src/font_resource.hpp:62](../../../../gui/src/font_resource.hpp#L62)*



**brief** Return parsed metrics, or null for a descriptor-only resource. 

### glyph_region

*public graphics::TextureRegion glyph_region(const graphics::GlyphPlacement & glyph)*



**brief** Derive the texture view for one placement from this font.

**glyph** Placement produced from this resource's BitmapFont.

**return** Non-owning page subregion, or an empty region when unresolved or when the placement does not fit its declared page.

### pages

*public int pages()*

*Defined at [gui/src/font_resource.hpp:68](../../../../gui/src/font_resource.hpp#L68)*



**brief** Return page regions in BMFont page-id order. 

### scale

*public float scale()*

*Defined at [gui/src/font_resource.hpp:83](../../../../gui/src/font_resource.hpp#L83)*



**brief** Return the uniform conversion from source pixels to GUI units. 



