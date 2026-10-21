# class NinePatchDrawable

*Defined at [gui/src/nine_patch_drawable.hpp:22](../../../../gui/src/nine_patch_drawable.hpp#L22)*



**brief** Scalable drawable that preserves corners and stretches edges and center.



Inherits from Drawable



## Functions

### NinePatchDrawable

*public void NinePatchDrawable(const graphics::TextureRegion & region, NinePatchSplits splits, std::optional<Insets> content_insets)*



**brief** Construct a nine-patch drawable.

**region** Region to sample; must outlive the drawable.

**splits** Corner scissor amounts in source pixels.

**content_insets** Content insets; defaults to a nine-patch inset derived from the splits.

### minimum_size

*public Size minimum_size()*



**brief** Report the smallest acceptable size.

**return** Minimum extent that preserves the corner pieces.

### content_insets

*public Insets content_insets()*



**brief** Report the content insets.

**return** Effective insets bounding the stretchable content region.

### draw

*public void draw(Painter & painter, const Rectangle & rectangle, graphics::Color tint)*



**brief** Draw the nine scalable pieces into the rectangle.

**painter** Destination painter; valid for the call.

**rectangle** Target rectangle in logical units.

**tint** Color multiplied with the sampled texture.



