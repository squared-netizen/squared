# class SpriteBatch

*Defined at [graphics/sprite_batch.cppm:22](../../../../graphics/sprite_batch.cppm#L22)*



**brief** Efficiently draw ordered textured quads with the selected backend.



## Functions

### SpriteBatch

*public void SpriteBatch()*

*Defined at [graphics/sprite_batch.cppm:26](../../../../graphics/sprite_batch.cppm#L26)*



**brief** Construct a batch owning no backend objects. 

### SpriteBatch

*public void SpriteBatch(SpriteBatch && )*

*Defined at [graphics/sprite_batch.cppm:29](../../../../graphics/sprite_batch.cppm#L29)*

### SpriteBatch

*public void SpriteBatch(const SpriteBatch & )*

*Defined at [graphics/sprite_batch.cppm:27](../../../../graphics/sprite_batch.cppm#L27)*

### begin

*public bool begin(const OrthographicCamera & camera)*



**brief** Begin an ordered batch using the camera projection.

**camera** Camera whose projection matrix drives the batch.

**return** true when the batch started; false if not initialized.

### clear_clip

*public void clear_clip()*



**brief** Stop restricting drawing. Flushes first, for the same reason. 

### destroy

*public void destroy()*



**brief** Release all owned backend objects. 

### draw

*public void draw(const Sprite & sprite)*



**brief** Draw one transformed Sprite.

**sprite** Sprite whose region and transform are drawn.

### draw

*public void draw(const TextureRegion & region, float x, float y, float width, float height, sq::graphics::Color color)*



**brief** Draw one region without rotation.

**region** Region to sample; must remain valid for the batch.

**x** Horizontal position in logical pixels.

**y** Vertical position in logical pixels.

**width** Drawn width in logical pixels.

**height** Drawn height in logical pixels.

**color** Tint multiplied with the sampled texture; defaults to opaque white.

### drawing

*public bool drawing()*



**brief** Report whether begin() has been called without a matching end(). 

### end

*public void end()*



**brief** Flush queued sprites and finish the batch. 

### flush

*public void flush()*



**brief** Flush queued sprites without ending the batch. 

### initialize

*public bool initialize(std::size_t maximum_sprites)*



**brief** Allocate GPU buffers and compile the built-in sprite shader.



**maximum_sprites** Maximum sprites buffered before an automatic flush.

**return** true when backend initialization succeeded.

### invalidate

*public void invalidate()*



**brief** Mark shaders and buffers stale without issuing graphics calls.

### operator=

*public SpriteBatch & operator=(SpriteBatch && )*

*Defined at [graphics/sprite_batch.cppm:30](../../../../graphics/sprite_batch.cppm#L30)*

### operator=

*public SpriteBatch & operator=(const SpriteBatch & )*

*Defined at [graphics/sprite_batch.cppm:28](../../../../graphics/sprite_batch.cppm#L28)*

### queued_sprites

*public std::size_t queued_sprites()*



**brief** Read how many sprites are queued and not yet drawn.

**return** Zero outside begin()/end(), or straight after a flush.

### release

*public void release()*



**brief** Release GPU objects while preserving batch capacity for restoration.

### restore

*public bool restore(bool context_preserved)*



**brief** Rebuild, stating explicitly whether the context survived.

**context_preserved** true when the GPU objects are still valid.



**note** No default. The previous one was `false`, which made `restore()` read as cheap and behave destructively: it reloaded everything, moved every texture's generation, and invalidated every TextureRegion copied out of an atlas - on the first frame, before anything had been lost.



**note** Prefer the overload taking a Context. It reads the answer from the object that knows it, so the polarity cannot be got backwards.

### restore

*public bool restore(const graphics::Context & graphics)*



**brief** Rebuild released shaders and buffers in the current context.

**context_preserved** Whether the GPU context survived loss; true skips recomputing fast-path state where possible.

**return** true when backend objects were restored.

### set_clip

*public void set_clip(int x, int y, int width, int height)*



**brief** Restrict drawing to a rectangle of the framebuffer.

**x** Left edge in framebuffer pixels.

**y** Bottom edge in framebuffer pixels, measured from the bottom.

**width** Width in pixels; zero or negative clips everything away.

**height** Height in pixels.



**note** Pixels, not logical units, and bottom-left origin - this is the scissor rectangle as the graphics API means it. Converting from whatever coordinate system the caller uses is the caller's job, because only the caller knows its own camera and drawable size.



**note** Flushes first. The scissor cannot change part-way through a draw call, so everything already queued must be drawn under the old one. That is why this lives on the batch: the flush and the change are inseparable, and anywhere else they could be desynchronised.

### skipped_draws

*public std::size_t skipped_draws()*



**brief** Read how many draws were refused since the last begin().

 A draw is refused when its region is no longer valid - its texture was discarded, reloaded, or restored under it. Drawing it would sample whatever now occupies that texture unit, so it is skipped.

 Skipping silently is correct and invisible, which is the problem: an interface that draws nothing looks exactly like one that was never asked to draw. This is the difference, and it is one assertion in a test rather than an afternoon of bisecting.

### sprite_capacity

*public std::size_t sprite_capacity()*



**brief** Read how many sprites one batch may hold before it must flush.

**return** The capacity given to initialize().

### valid

*public bool valid()*



**brief** Check initialization state.

**return** true when initialization succeeded.

### ~SpriteBatch

*public void ~SpriteBatch()*



**brief** Destroy all owned backend objects. 

