# class BatchPainter

*Defined at [gui/batch_painter.cppm:24](../../../../gui/batch_painter.cppm#L24)*



**brief** A Painter that draws through a SpriteBatch.

 The concrete end of the widget set: everything a widget draws becomes a quad in a batch. Owns neither the batch nor the font; both must outlive it.

 Call set_viewport() before the first frame, and again on resize. Clipping needs both sizes: the GUI works in logical units with a top-left origin, and a scissor rectangle is framebuffer pixels measured from the bottom.



Inherits from Painter



## Functions

### BatchPainter

*public void BatchPainter(const BatchPainter & )*

*Defined at [gui/batch_painter.cppm:37](../../../../gui/batch_painter.cppm#L37)*

### BatchPainter

*public void BatchPainter(graphics::SpriteBatch & batch, const FontResource * default_font)*



**brief** Bind to a batch and a default font.

**batch** Batch to draw into; must outlive this painter.

**default_font** Font used by the text calls that take none; may be null, in which case those calls draw nothing and measure to zero.

### clip_depth

*public std::size_t clip_depth()*

*Defined at [gui/batch_painter.cppm:97](../../../../gui/batch_painter.cppm#L97)*



**brief** Report how deeply clips are currently nested. 

### draw_region

*public void draw_region(const graphics::TextureRegion & region, const Rectangle & rectangle, graphics::Color color)*

### draw_text

*public void draw_text(std::string_view text, float x, float y, const FontResource * font, graphics::Color color)*

### draw_text

*public void draw_text(std::string_view text, float x, float y, graphics::Color color)*

### fill_rectangle

*public void fill_rectangle(const Rectangle & rectangle, graphics::Color color)*

### measure_text

*public Size measure_text(std::string_view text)*

### measure_text

*public Size measure_text(std::string_view text, const FontResource * font)*

### operator=

*public BatchPainter & operator=(const BatchPainter & )*

*Defined at [gui/batch_painter.cppm:38](../../../../gui/batch_painter.cppm#L38)*

### pop_clip

*public void pop_clip()*

### push_clip

*public void push_clip(const Rectangle & rectangle)*

### set_fill_source

*public bool set_fill_source(const graphics::TextureRegion * white)*



**brief** Supply the texel that solid fills are drawn from.

**white** A fully white, fully opaque region, or null.

**return** true when a fill source is available.



**note** Pass the skin's `white` region. Fills then come from the same atlas page as every other widget graphic, so a whole interface can be one draw call. A painter with its own texture instead forces a texture switch - and therefore a draw call - at every fill.



**note** With null, a 1x1 white texture is created and owned here. That works, and costs a draw call per fill. It is the fallback, not the intent.



**note** The region is referenced, not copied, so it follows the atlas through a restore. A copy goes stale the moment the atlas reloads a page - every fill is then silently skipped - and the caller would have to know to set it again. Keeping the pointer removes that rule; the region must simply outlive the painter, which an atlas region does.

### set_viewport

*public void set_viewport(float logical_width, float logical_height, int pixel_width, int pixel_height)*



**brief** Tell the painter how logical units map to pixels.

**logical_width** Width the GUI lays out in.

**logical_height** Height the GUI lays out in.

**pixel_width** Drawable width, from the graphics context.

**pixel_height** Drawable height.



**note** Only clipping needs this. Everything else is in logical units and the camera handles it.

### stroke_rectangle

*public void stroke_rectangle(const Rectangle & rectangle, graphics::Color color, float thickness)*

### ~BatchPainter

*public void ~BatchPainter()*

