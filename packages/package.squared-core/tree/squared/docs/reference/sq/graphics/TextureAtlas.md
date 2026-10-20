# class TextureAtlas

*Defined at [graphics/texture_atlas.hpp:29](../../../../graphics/texture_atlas.hpp#L29)*



**brief** Owning, transactionally loaded libGDX text texture atlas.

 Page image paths are resolved relative to the atlas asset. Multiple pages, duplicate indexed names, rotation, trimming, filtering, repeat modes, splits, and padding are supported. Packing source images is a separate future asset-tools concern.



## Functions

### TextureAtlas

*public void TextureAtlas()*

*Defined at [graphics/texture_atlas.hpp:33](../../../../graphics/texture_atlas.hpp#L33)*



**brief** Construct an empty atlas owning no pages or regions. 

### TextureAtlas

*public void TextureAtlas(const TextureAtlas & )*

*Defined at [graphics/texture_atlas.hpp:34](../../../../graphics/texture_atlas.hpp#L34)*

### operator=

*public TextureAtlas & operator=(const TextureAtlas & )*

*Defined at [graphics/texture_atlas.hpp:35](../../../../graphics/texture_atlas.hpp#L35)*

### TextureAtlas

*public void TextureAtlas(TextureAtlas && )*

*Defined at [graphics/texture_atlas.hpp:36](../../../../graphics/texture_atlas.hpp#L36)*

### operator=

*public TextureAtlas & operator=(TextureAtlas && )*

*Defined at [graphics/texture_atlas.hpp:37](../../../../graphics/texture_atlas.hpp#L37)*

### ~TextureAtlas

*public void ~TextureAtlas()*



**brief** Destroy every owned page texture and region. 

### load

*public bool load(const files::FileHandle & atlas)*



**brief** Load an atlas asset without disturbing a valid prior load on failure.

**atlas_path** Null-terminated path of the libGDX text atlas asset. Page image paths are resolved relative to it.

**return** true when every page and region loaded successfully.

### load

*public bool load(const files::FileHandle & atlas, TextureRecoveryPolicy page_recovery)*



**brief** Load an atlas using one recovery policy for every page.

 ReloadFromAsset, RetainPixels, and Discard are supported. Regenerate is rejected because atlas pages require distinct application recipes.

**atlas_path** Null-terminated path of the libGDX text atlas asset.

**page_recovery** Policy applied to every loaded page.

**return** true when every page and region loaded successfully.

### destroy

*public void destroy()*



**brief** Destroy every atlas page texture and region. 

### release

*public void release()*



**brief** Release page GPU objects while preserving regions and load recipes.

### invalidate

*public void invalidate()*



**brief** Mark page GPU objects stale without issuing graphics calls.

### restore

*public bool restore(const graphics::Context & graphics)*



**brief** Restore every released page texture without invalidating regions.

**context_preserved** Whether the GPU context survived loss; true skips recomputing fast-path state where possible.

**return** true when every page texture was restored.

### restore

*public bool restore(bool context_preserved)*



**brief** Rebuild, stating explicitly whether the context survived.

**context_preserved** true when the GPU objects are still valid.



**note** No default. The previous one was `false`, which made `restore()` read as cheap and behave destructively: it reloaded everything, moved every texture's generation, and invalidated every TextureRegion copied out of an atlas - on the first frame, before anything had been lost.



**note** Prefer the overload taking a Context. It reads the answer from the object that knows it, so the polarity cannot be got backwards.

### valid

*public bool valid()*



**brief** Check whether any page and region is loaded.

**return** true when the atlas owns at least one page and region.

### page_count

*public int page_count()*



**brief** Count loaded page textures.

**return** Number of loaded page textures.

### region_count

*public int region_count()*



**brief** Count loaded regions.

**return** Number of loaded regions.

### find_region

*public const AtlasRegion * find_region(const int & name, int index)*



**brief** Find an exact name/index pair, or return null.

**name** Region name to look up.

**index** Duplicate-region index, or -1 for the default entry.

**return** Pointer to the matching region, or null when absent.



