# class TextureRegion

*Defined at [graphics/texture_region.cppm:15](../../../../graphics/texture_region.cppm#L15)*



**brief** Non-owning rectangular view into a Texture.

 The referenced Texture must outlive the region and every draw operation that uses it.



## Functions

### TextureRegion

*public void TextureRegion()*

*Defined at [graphics/texture_region.cppm:19](../../../../graphics/texture_region.cppm#L19)*



**brief** Construct an empty, invalid region. 

### TextureRegion

*public void TextureRegion(const Texture & texture)*

*Defined at [graphics/texture_region.cppm:41](../../../../graphics/texture_region.cppm#L41)*



**brief** Reference the complete texture.

**texture** Texture shared non-owningly; must outlive the region.

### TextureRegion

*public void TextureRegion(const Texture & texture, int x, int y, int width, int height, bool rotated_clockwise)*

*Defined at [graphics/texture_region.cppm:60](../../../../graphics/texture_region.cppm#L60)*



**brief** Reference a pixel rectangle using top-left image coordinates.

 For clockwise-packed atlas entries, width and height describe the logical unrotated region; their storage extents are swapped.

**texture** Texture shared non-owningly; must outlive the region.

**x** Left rect coordinate in source pixels.

**y** Top rect coordinate in source pixels.

**width** Logical width in source pixels.

**height** Logical height in source pixels.

**rotated_clockwise** Whether atlas storage is rotated.

### TextureRegion

*public void TextureRegion(int width, int height, bool rotated_clockwise)*

*Defined at [graphics/texture_region.cppm:31](../../../../graphics/texture_region.cppm#L31)*



**brief** Create an unbacked logical region for recording and validation.

 The region has dimensions and normalized coordinates but is not valid for GPU rendering because it references no Texture.

**width** Logical width in pixels; clamped to non-negative.

**height** Logical height in pixels; clamped to non-negative.

**rotated_clockwise** Whether atlas storage is rotated 90 degrees clockwise.

### height

*public int height()*

*Defined at [graphics/texture_region.cppm:146](../../../../graphics/texture_region.cppm#L146)*



**brief** Read the region height.

**return** Height in source pixels.

### refresh

*public bool refresh()*

*Defined at [graphics/texture_region.cppm:121](../../../../graphics/texture_region.cppm#L121)*



**brief** Re-bind this region to its texture's current content.

**return** true when the region is valid afterwards.

**note** For a region whose texture was restored with the same dimensions and layout - the common case after context loss, where the pixels come back exactly as they were. A region into an atlas that was repacked is not made correct by this, because its coordinates never described the new layout.

### rotated_clockwise

*public bool rotated_clockwise()*

*Defined at [graphics/texture_region.cppm:176](../../../../graphics/texture_region.cppm#L176)*



**brief** Report whether atlas storage is rotated.

**return** true when storage is rotated 90 degrees clockwise.

### stale

*public bool stale()*

*Defined at [graphics/texture_region.cppm:106](../../../../graphics/texture_region.cppm#L106)*



**brief** Report whether the texture's content changed under this region.

**return** true when the region still references a texture but that texture's content has since been replaced or discarded.

**note** Distinguishes "never had a texture" from "had one and lost it", which valid() alone cannot. A caller rebuilding an atlas wants the second; a caller checking an empty slot wants neither.

### subregion

*public TextureRegion subregion(int x, int y, int width, int height)*

*Defined at [graphics/texture_region.cppm:190](../../../../graphics/texture_region.cppm#L190)*



**brief** Return a logical subregion using top-left coordinates.

 The returned view references the same texture and preserves atlas rotation. Invalid bounds return an empty region.

**x** Left coordinate in logical pixels.

**y** Top coordinate in logical pixels.

**width** Subregion width in logical pixels.

**height** Subregion height in logical pixels.

**return** A non-owning subregion view, or an empty region when the requested bounds fall outside this region.

### texture

*public const Texture & texture()*

*Defined at [graphics/texture_region.cppm:134](../../../../graphics/texture_region.cppm#L134)*



**brief** Access the referenced texture.

**return** Reference to the texture; undefined when valid() is false.

### u1

*public float u1()*

*Defined at [graphics/texture_region.cppm:152](../../../../graphics/texture_region.cppm#L152)*



**brief** Read the left normalized texture coordinate.

**return** U coordinate in the inclusive range [0, 1].

### u2

*public float u2()*

*Defined at [graphics/texture_region.cppm:164](../../../../graphics/texture_region.cppm#L164)*



**brief** Read the right normalized texture coordinate.

**return** U coordinate in the inclusive range [0, 1].

### v1

*public float v1()*

*Defined at [graphics/texture_region.cppm:158](../../../../graphics/texture_region.cppm#L158)*



**brief** Read the top normalized texture coordinate.

**return** V coordinate in the inclusive range [0, 1].

### v2

*public float v2()*

*Defined at [graphics/texture_region.cppm:170](../../../../graphics/texture_region.cppm#L170)*



**brief** Read the bottom normalized texture coordinate.

**return** V coordinate in the inclusive range [0, 1].

### valid

*public bool valid()*

*Defined at [graphics/texture_region.cppm:91](../../../../graphics/texture_region.cppm#L91)*



**brief** Check whether this region can be rendered.

**return** true when the referenced texture still holds the content this region was built against, and the region has positive size.



**note** The generation check is what stops a crash after a phone call. A texture whose content was discarded, reloaded or regenerated has moved its content_generation(); this region's coordinates then describe pixels that are no longer there, or are not there at all. Drawing it would sample whatever now occupies that texture unit.

### width

*public int width()*

*Defined at [graphics/texture_region.cppm:140](../../../../graphics/texture_region.cppm#L140)*



**brief** Read the region width.

**return** Width in source pixels.

