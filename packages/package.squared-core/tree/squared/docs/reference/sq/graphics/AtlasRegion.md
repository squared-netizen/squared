# class AtlasRegion

*Defined at [graphics/atlas_region.hpp:19](../../../../graphics/atlas_region.hpp#L19)*



**brief** One named region and its libGDX-compatible atlas metadata.

 The TextureRegion and its Texture remain owned by the containing TextureAtlas. AtlasRegion pointers become invalid when that atlas is destroyed or successfully reloaded.



## Members

public int name_

public int index_

public TextureRegion region_

public int packed_width_

public int packed_height_

public int original_width_

public int original_height_

public int offset_x_

public int offset_y_



## Functions

### name

*public const int & name()*



**brief** Read the logical region name.

**return** Reference to the region's name; stable while the atlas lives.

### index

*public int index()*



**brief** Read the duplicate-region index.

**return** Index for repeated names, or -1 when unindexed.

### packed_width

*public int packed_width()*



**brief** Read the trimmed logical width before atlas rotation.

**return** Width in source pixels.

### packed_height

*public int packed_height()*



**brief** Read the trimmed logical height before atlas rotation.

**return** Height in source pixels.

### region

*public const TextureRegion & region()*

*Defined at [graphics/atlas_region.hpp:38](../../../../graphics/atlas_region.hpp#L38)*



**brief** Access the drawable texture view.

**return** Reference to the region's texture view.

### rotated_clockwise

*public bool rotated_clockwise()*



**brief** Read the storage rotation flag.

**return** true when storage is rotated 90 degrees clockwise.

### original_width

*public int original_width()*



**brief** Read the width before whitespace trimming.

**return** Original width in source pixels.

### original_height

*public int original_height()*



**brief** Read the height before whitespace trimming.

**return** Original height in source pixels.

### offset_x

*public int offset_x()*



**brief** Read the trimmed region's horizontal placement.

**return** Horizontal offset in source pixels.

### offset_y

*public int offset_y()*



**brief** Read the trimmed region's vertical placement.

**return** Vertical offset in source pixels.



