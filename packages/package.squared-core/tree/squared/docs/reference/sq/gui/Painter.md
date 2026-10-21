# class Painter

*Defined at [gui/src/painter.hpp:26](../../../../gui/src/painter.hpp#L26)*



**brief** Portable drawing boundary implemented over sq::graphics.

 All coordinate systems are logical pixels in the same space used for widget placement. The painter is owned by the UI framework and is only valid within the measure/layout/paint calls that pass it.



## Functions

### measure_text

*public Size measure_text(std::string_view text)*



**brief** Measure rendered text.

**text** UTF-8 text to measure.

**return** Extent the text occupies when drawn with the current font.

### measure_text

*public Size measure_text(std::string_view text, const FontResource * font)*



**brief** Measure text using a style-selected font when it is resolved.

**text** UTF-8 text to measure.

**font** Borrowed font resource, or null for the painter default.

**return** Extent in logical units. The base implementation uses GlyphLayout for resolved resources and otherwise calls measure_text(text).

### fill_rectangle

*public void fill_rectangle(const Rectangle & rectangle, graphics::Color color)*



**brief** Fill a rectangle with a solid color.

**rectangle** Area to fill in logical units.

**color** Normalized fill color.

### stroke_rectangle

*public void stroke_rectangle(const Rectangle & rectangle, graphics::Color color, float thickness)*



**brief** Stroke a rectangle border.

**rectangle** Centerline of the border in logical units.

**color** Normalized stroke color.

**thickness** Border width in logical units; non-negative.

### draw_region

*public void draw_region(const graphics::TextureRegion & region, const Rectangle & rectangle, graphics::Color color)*



**brief** Draw a texture region scaled to a rectangle.

**region** Region to sample; must outlive the call.

**rectangle** Target rectangle in logical units.

**color** Tint multiplied with the sampled texture.

### draw_text

*public void draw_text(std::string_view text, float x, float y, graphics::Color color)*



**brief** Draw single-line text.

**text** UTF-8 text to render.

**x** Left baseline position in logical units.

**y** Top baseline position in logical units.

**color** Normalized text color.

### draw_text

*public void draw_text(std::string_view text, float x, float y, const FontResource * font, graphics::Color color)*



**brief** Draw single-line text with a style-selected font.

**text** UTF-8 text to render.

**x** Left position in logical units.

**y** Top position in logical units.

**font** Borrowed font resource, or null for the painter default.

**color** Normalized text color.

**note** The base implementation delegates to the font-agnostic overload; renderers supporting bitmap pages override this method.

### push_clip

*public void push_clip(const Rectangle & rectangle)*



**brief** Push a clipping rectangle.

**rectangle** Clip region in logical units; nested clips intersect.

### pop_clip

*public void pop_clip()*



**brief** Pop the most recently pushed clipping rectangle. 

### ~Painter

*public void ~Painter()*

*Defined at [gui/src/painter.hpp:29](../../../../gui/src/painter.hpp#L29)*



