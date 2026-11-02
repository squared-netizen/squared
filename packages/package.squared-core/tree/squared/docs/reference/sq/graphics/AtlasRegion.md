# class AtlasRegion

*Defined at [graphics/atlas_region.cppm:16](../../../../graphics/atlas_region.cppm#L16)*



**brief** One named region and its libGDX-compatible atlas metadata.

 The TextureRegion and its Texture remain owned by the containing TextureAtlas. AtlasRegion pointers become invalid when that atlas is destroyed or successfully reloaded.



## Functions

### index

*public int index()*



**brief** Read the duplicate-region index.

**return** Index for repeated names, or -1 when unindexed.

### name

*public const std::string & name()*



**brief** Read the logical region name.

**return** Reference to the region's name; stable while the atlas lives.

### offset_x

*public int offset_x()*



**brief** Read the trimmed region's horizontal placement.

**return** Horizontal offset in source pixels.

### offset_y

*public int offset_y()*



**brief** Read the trimmed region's vertical placement.

**return** Vertical offset in source pixels.

### original_height

*public int original_height()*



**brief** Read the height before whitespace trimming.

**return** Original height in source pixels.

### original_width

*public int original_width()*



**brief** Read the width before whitespace trimming.

**return** Original width in source pixels.

### packed_height

*public int packed_height()*



**brief** Read the trimmed logical height before atlas rotation.

**return** Height in source pixels.

### packed_width

*public int packed_width()*



**brief** Read the trimmed logical width before atlas rotation.

**return** Width in source pixels.

### pads

*public const std::optional<std::array<int, 4>> & pads()*

*Defined at [graphics/atlas_region.cppm:92](../../../../graphics/atlas_region.cppm#L92)*



**brief** Read optional nine-patch padding.

**return** Left, right, top, bottom padding when present, else empty.

### region

*public const TextureRegion & region()*

*Defined at [graphics/atlas_region.cppm:35](../../../../graphics/atlas_region.cppm#L35)*



**brief** Access the drawable texture view.

**return** Reference to the region's texture view.

### rotated_clockwise

*public bool rotated_clockwise()*



**brief** Read the storage rotation flag.

**return** true when storage is rotated 90 degrees clockwise.

### splits

*public const std::optional<std::array<int, 4>> & splits()*

*Defined at [graphics/atlas_region.cppm:83](../../../../graphics/atlas_region.cppm#L83)*



**brief** Read optional nine-patch splits.

**return** Left, right, top, bottom splits when present, else empty.

