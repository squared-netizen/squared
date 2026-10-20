# class RegionDrawable

*Defined at [gui/src/region_drawable.hpp:24](../../../../gui/src/region_drawable.hpp#L24)*



**brief** Drawable that samples one texture region.

 The referenced region must outlive the drawable.



Inherits from Drawable



## Functions

### RegionDrawable

*public void RegionDrawable(const graphics::TextureRegion & region, Insets insets)*



**brief** Construct a texture-region drawable.

**region** Region to sample; must outlive the drawable.

**insets** Content insets in logical units.

### minimum_size

*public Size minimum_size()*



**brief** Report the smallest acceptable size.

**return** Minimum extent set from the region's pixel size.

### content_insets

*public Insets content_insets()*



**brief** Report the content insets.

**return** Insets between the outer box and the region content.

### draw

*public void draw(Painter & painter, const Rectangle & rectangle, graphics::Color tint)*



**brief** Draw the region stretched to the rectangle.

**painter** Destination painter; valid for the call.

**rectangle** Target rectangle in logical units.

**tint** Color multiplied with the sampled texture.



