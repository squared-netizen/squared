# class Texture2D

*Defined at [gles/texture2d.cppm:23](../../../../gles/texture2d.cppm#L23)*



**brief** One 2D GL texture, owned.

 16 bytes: a name, dimensions and a format. **No pixel data is retained.** That is the priority-1 decision at the heart of this class: keeping a CPU copy would make recovery after context loss automatic and would double the resident cost of every texture in the application.

 Recovery is the caller's business, and squared::graphics::Texture is where the policy for it lives. This class only offers what recovery needs: upload() to refill, and invalidate() to forget a name whose context is gone.



## Functions

### Texture2D

*public void Texture2D()*

*Defined at [gles/texture2d.cppm:26](../../../../gles/texture2d.cppm#L26)*

### Texture2D

*public void Texture2D(Texture2D && other)*

### Texture2D

*public void Texture2D(const Texture2D & )*

*Defined at [gles/texture2d.cppm:29](../../../../gles/texture2d.cppm#L29)*

### bind

*public void bind(int unit)*



**brief** Bind to a texture unit.

**unit** Unit index; the value a sampler uniform is set to.

### bytes_per_pixel

*public static int bytes_per_pixel(TextureFormat format)*



**brief** Return the bytes one pixel of a format occupies. 

### create

*public GlesError create(int width, int height, TextureFormat format, std::span<const std::byte> pixels)*



**brief** Allocate storage and optionally fill it.

**width** Width in pixels; must be positive.

**height** Height in pixels; must be positive.

**format** Pixel format of the storage.

**pixels** Initial contents, tightly packed, or empty to allocate uninitialised storage.

**return** A no-error GlesError on success.

**note** Row alignment is set to 1 before upload. GL defaults to 4, which corrupts any R8 texture whose width is not a multiple of four — the classic skewed-font-atlas bug.

### destroy

*public void destroy()*



**brief** Release the GL object. Safe to call more than once. 

### format

*public TextureFormat format()*

*Defined at [gles/texture2d.cppm:105](../../../../gles/texture2d.cppm#L105)*



**brief** Return the storage format. 

### height

*public int height()*

*Defined at [gles/texture2d.cppm:102](../../../../gles/texture2d.cppm#L102)*



**brief** Return the height in pixels. 

### invalidate

*public void invalidate()*



**brief** Forget the GL name without deleting it, after context loss. 

### name

*public std::uint32_t name()*

*Defined at [gles/texture2d.cppm:96](../../../../gles/texture2d.cppm#L96)*



**brief** Return the raw GL name, or zero. 

### operator=

*public Texture2D & operator=(Texture2D && other)*

### operator=

*public Texture2D & operator=(const Texture2D & )*

*Defined at [gles/texture2d.cppm:30](../../../../gles/texture2d.cppm#L30)*

### set_sampling

*public GlesError set_sampling(graphics::TextureFilter minify, graphics::TextureFilter magnify, graphics::TextureWrap wrap)*



**brief** Set minification, magnification and wrapping.

**minify** Filter used when the texture is drawn smaller.

**magnify** Filter used when it is drawn larger.

**wrap** Behaviour outside the zero-to-one range.

**return** A no-error GlesError on success.

### upload

*public GlesError upload(std::span<const std::byte> pixels)*



**brief** Replace the whole contents, keeping the same storage.

**pixels** Tightly packed pixels matching the existing size and format.

**return** A no-error GlesError on success.

### upload_region

*public GlesError upload_region(int x, int y, int width, int height, std::span<const std::byte> pixels)*



**brief** Replace a rectangle of the contents.

**x** Left edge in pixels.

**y** Top edge in pixels.

**width** Width of the rectangle.

**height** Height of the rectangle.

**pixels** Tightly packed pixels for the rectangle.

**return** A no-error GlesError on success.

### valid

*public bool valid()*

*Defined at [gles/texture2d.cppm:93](../../../../gles/texture2d.cppm#L93)*



**brief** Report whether this holds a GL object. 

### width

*public int width()*

*Defined at [gles/texture2d.cppm:99](../../../../gles/texture2d.cppm#L99)*



**brief** Return the width in pixels. 

### ~Texture2D

*public void ~Texture2D()*

