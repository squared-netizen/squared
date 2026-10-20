# class TextureRegion

*Defined at [graphics/texture_region.hpp:16](../../../../graphics/texture_region.hpp#L16)*



**brief** Non-owning rectangular view into a Texture.

 The referenced Texture must outlive the region and every draw operation that uses it.



## Functions

### TextureRegion

*public void TextureRegion()*

*Defined at [graphics/texture_region.hpp:20](../../../../graphics/texture_region.hpp#L20)*



**brief** Construct an empty, invalid region. 

### TextureRegion

*public void TextureRegion(int width, int height, bool rotated_clockwise)*

*Defined at [graphics/texture_region.hpp:32](../../../../graphics/texture_region.hpp#L32)*



**brief** Create an unbacked logical region for recording and validation.

 The region has dimensions and normalized coordinates but is not valid for GPU rendering because it references no Texture.

**width** Logical width in pixels; clamped to non-negative.

**height** Logical height in pixels; clamped to non-negative.

**rotated_clockwise** Whether atlas storage is rotated 90 degrees clockwise.

### TextureRegion

*public void TextureRegion(const Texture & texture)*

*Defined at [graphics/texture_region.hpp:42](../../../../graphics/texture_region.hpp#L42)*



**brief** Reference the complete texture.

**texture** Texture shared non-owningly; must outlive the region.

### TextureRegion

*public void TextureRegion(const Texture & texture, int x, int y, int width, int height, bool rotated_clockwise)*

*Defined at [graphics/texture_region.hpp:61](../../../../graphics/texture_region.hpp#L61)*



**brief** Reference a pixel rectangle using top-left image coordinates.

 For clockwise-packed atlas entries, width and height describe the logical unrotated region; their storage extents are swapped.

**texture** Texture shared non-owningly; must outlive the region.

**x** Left rect coordinate in source pixels.

**y** Top rect coordinate in source pixels.

**width** Logical width in source pixels.

**height** Logical height in source pixels.

**rotated_clockwise** Whether atlas storage is rotated.

### valid

*public bool valid()*

*Defined at [graphics/texture_region.hpp:92](../../../../graphics/texture_region.hpp#L92)*



**brief** Check whether this region can be rendered.

**return** true when the referenced texture still holds the content this region was built against, and the region has positive size.



**note** The generation check is what stops a crash after a phone call. A texture whose content was discarded, reloaded or regenerated has moved its content_generation(); this region's coordinates then describe pixels that are no longer there, or are not there at all. Drawing it would sample whatever now occupies that texture unit.

### stale

*public bool stale()*

*Defined at [graphics/texture_region.hpp:107](../../../../graphics/texture_region.hpp#L107)*



**brief** Report whether the texture's content changed under this region.

**return** true when the region still references a texture but that texture's content has since been replaced or discarded.

**note** Distinguishes "never had a texture" from "had one and lost it", which valid() alone cannot. A caller rebuilding an atlas wants the second; a caller checking an empty slot wants neither.

### refresh

*public bool refresh()*

*Defined at [graphics/texture_region.hpp:122](../../../../graphics/texture_region.hpp#L122)*



**brief** Re-bind this region to its texture's current content.

**return** true when the region is valid afterwards.

**note** For a region whose texture was restored with the same dimensions and layout - the common case after context loss, where the pixels come back exactly as they were. A region into an atlas that was repacked is not made correct by this, because its coordinates never described the new layout.

### texture

*public const Texture & texture()*

*Defined at [graphics/texture_region.hpp:135](../../../../graphics/texture_region.hpp#L135)*



**brief** Access the referenced texture.

**return** Reference to the texture; undefined when valid() is false.

### width

*public int width()*

*Defined at [graphics/texture_region.hpp:141](../../../../graphics/texture_region.hpp#L141)*



**brief** Read the region width.

**return** Width in source pixels.

### height

*public int height()*

*Defined at [graphics/texture_region.hpp:147](../../../../graphics/texture_region.hpp#L147)*



**brief** Read the region height.

**return** Height in source pixels.

### u1

*public float u1()*

*Defined at [graphics/texture_region.hpp:153](../../../../graphics/texture_region.hpp#L153)*



**brief** Read the left normalized texture coordinate.

**return** U coordinate in the inclusive range [0, 1].

### v1

*public float v1()*

*Defined at [graphics/texture_region.hpp:159](../../../../graphics/texture_region.hpp#L159)*



**brief** Read the top normalized texture coordinate.

**return** V coordinate in the inclusive range [0, 1].

### u2

*public float u2()*

*Defined at [graphics/texture_region.hpp:165](../../../../graphics/texture_region.hpp#L165)*



**brief** Read the right normalized texture coordinate.

**return** U coordinate in the inclusive range [0, 1].

### v2

*public float v2()*

*Defined at [graphics/texture_region.hpp:171](../../../../graphics/texture_region.hpp#L171)*



**brief** Read the bottom normalized texture coordinate.

**return** V coordinate in the inclusive range [0, 1].

### rotated_clockwise

*public bool rotated_clockwise()*

*Defined at [graphics/texture_region.hpp:177](../../../../graphics/texture_region.hpp#L177)*



**brief** Report whether atlas storage is rotated.

**return** true when storage is rotated 90 degrees clockwise.

### subregion

*public TextureRegion subregion(int x, int y, int width, int height)*

*Defined at [graphics/texture_region.hpp:191](../../../../graphics/texture_region.hpp#L191)*



**brief** Return a logical subregion using top-left coordinates.

 The returned view references the same texture and preserves atlas rotation. Invalid bounds return an empty region.

**x** Left coordinate in logical pixels.

**y** Top coordinate in logical pixels.

**width** Subregion width in logical pixels.

**height** Subregion height in logical pixels.

**return** A non-owning subregion view, or an empty region when the requested bounds fall outside this region.



