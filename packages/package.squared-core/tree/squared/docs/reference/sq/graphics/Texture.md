# class Texture

*Defined at [graphics/texture.hpp:30](../../../../graphics/texture.hpp#L30)*



**brief** Move-only texture owned by the selected graphics backend.

 A valid graphics context must be current whenever a texture is created, released, restored, or destroyed. Asset paths and RGBA source data are retained according to the selected portable recovery policy.



## Functions

### Texture

*public void Texture()*

*Defined at [graphics/texture.hpp:34](../../../../graphics/texture.hpp#L34)*



**brief** Construct an empty texture owning no backend object. 

### Texture

*public void Texture(const Texture & )*

*Defined at [graphics/texture.hpp:35](../../../../graphics/texture.hpp#L35)*

### Texture

*public void Texture(Texture && other)*



**brief** Move-construct from another texture.

**other** Texture to assume; left empty afterward.

### operator=

*public Texture & operator=(Texture && other)*



**brief** Move-assign from another texture.

**other** Texture to assume; left empty afterward.

**return** Reference to this texture.

**note** Releases this texture's backend object first, if any.

### ~Texture

*public void ~Texture()*



**brief** Destroy the backend object, if any. 

### load

*public bool load(const files::FileHandle & source)*



**brief** Load an image asset through the selected backend.

**source** Handle naming the encoded image. The handle is retained, so the file system it came from must outlive this texture.

**return** true when the texture loaded successfully.

**note** A handle rather than a path because a path alone cannot be read back: the handle carries its file system with it, which is what lets restore() refill this texture after context loss without reaching into the asset manager that created it.

### load

*public bool load(const files::FileHandle & source, TextureRecoveryOptions recovery)*



**brief** Load an asset with an explicit recovery policy.

**source** Handle naming the encoded image.

**recovery** Policy controlling post-context-loss recovery.

**return** true when the texture loaded successfully.

### create_rgba

*public bool create_rgba(int width, int height, const int * pixels)*



**brief** Create an RGBA8888 texture from tightly packed pixels.

**width** Image width in pixels; must be greater than zero.

**height** Image height in pixels; must be greater than zero.

**pixels** Tightly packed RGBA8888 source, width * height * 4 bytes.

**return** true when the texture storage was created.

### create_rgba

*public bool create_rgba(int width, int height, const int * pixels, TextureRecoveryOptions recovery)*



**brief** Create RGBA8888 storage with an explicit recovery policy.

**width** Image width in pixels; must be greater than zero.

**height** Image height in pixels; must be greater than zero.

**pixels** Tightly packed RGBA8888 source, width * height * 4 bytes.

**recovery** Policy controlling post-context-loss recovery.

**return** true when the texture storage was created.

### create_solid

*public bool create_solid(sq::graphics::Color color)*



**brief** Create a one-pixel texture containing a solid color.

**color** Normalized color for the single texel.

**return** true when the texture storage was created.

### create_solid

*public bool create_solid(sq::graphics::Color color, TextureRecoveryOptions recovery)*



**brief** Create a solid texture with an explicit recovery policy.

**color** Normalized color for the single texel.

**recovery** Policy controlling post-context-loss recovery.

**return** true when the texture storage was created.

### destroy

*public void destroy()*



**brief** Destroy the backend texture. 

### release

*public void release()*



**brief** Release only the GPU object while retaining its restoration recipe.

### invalidate

*public void invalidate()*



**brief** Mark the GPU object stale without issuing graphics calls.

### operator=

*public Texture & operator=(const Texture & )*

*Defined at [graphics/texture.hpp:36](../../../../graphics/texture.hpp#L36)*

### restore

*public bool restore(const graphics::Context & graphics)*



**brief** Recreate a released GPU object in the current graphics context.

**context_preserved** Whether the GPU context survived loss; true skips recomputing fast-path state where possible.

**return** true when the backend object was restored.

### restore

*public bool restore(bool context_preserved)*



**brief** Rebuild, stating explicitly whether the context survived.

**context_preserved** true when the GPU objects are still valid.



**note** No default. The previous one was `false`, which made `restore()` read as cheap and behave destructively: it reloaded everything, moved every texture's generation, and invalidated every TextureRegion copied out of an atlas - on the first frame, before anything had been lost.



**note** Prefer the overload taking a Context. It reads the answer from the object that knows it, so the polarity cannot be got backwards.

### restorable

*public bool restorable()*



**brief** Check whether restoration source data is available.

**return** true when this texture has enough source data to restore.

### recovery_policy

*public TextureRecoveryPolicy recovery_policy()*



**brief** Read the configured recovery policy.

**return** Policy selected at creation time.

### retained_recovery_bytes

*public int retained_recovery_bytes()*



**brief** Measure retained recovery data.

**return** Bytes retained solely as an RGBA recovery recipe.

### set_filter

*public void set_filter(TextureFilter minification, TextureFilter magnification)*



**brief** Set minification and magnification filters.

**minification** Texture-wide minification filter.

**magnification** Texture-wide magnification filter.

### set_wrap

*public void set_wrap(TextureWrap horizontal, TextureWrap vertical)*



**brief** Set horizontal and vertical texture wrapping.

**horizontal** Texture-wide horizontal wrap mode.

**vertical** Texture-wide vertical wrap mode.

### bind

*public void bind(unsigned int unit)*



**brief** Bind the texture to a zero-based texture unit.

**unit** Zero-based texture unit to bind into.

### valid

*public bool valid()*



**brief** Check whether this texture owns a backend object.

**return** true while the texture owns a valid backend object.

### width

*public int width()*



**brief** Read the texture width.

**return** Width in pixels; zero when no backend object exists.

### height

*public int height()*



**brief** Read the texture height.

**return** Height in pixels; zero when no backend object exists.

### content_generation

*public int content_generation()*



**brief** Read the counter that changes whenever the content does.

**return** A value that increments on every upload, restore, release and invalidate.

**note** This is what makes a stale TextureRegion detectable. A region records the generation it was built against; when the two differ, the region's coordinates no longer describe the pixels that are there, and after a Discard there are no pixels at all. Without it, a region outliving its texture's content is a crash after a phone call.

### has_content

*public bool has_content()*



**brief** Check whether the texture currently holds usable content.

**return** true when a backend object exists and has not been released, invalidated or discarded.



