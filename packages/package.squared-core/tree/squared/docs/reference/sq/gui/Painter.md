# class Painter

*Defined at [gui/src/painter.cppm:20](../../../../gui/src/painter.cppm#L20)*

## Functions

### measure_text

*public Size measure_text(std::string_view text)*

### measure_text

*public Size measure_text(std::string_view text, const FontResource * font)*

### fill_rectangle

*public void fill_rectangle(const Rectangle & rectangle, graphics::Color color)*

### stroke_rectangle

*public void stroke_rectangle(const Rectangle & rectangle, graphics::Color color, float thickness)*

### draw_region

*public void draw_region(const graphics::TextureRegion & region, const Rectangle & rectangle, graphics::Color color)*

### draw_text

*public void draw_text(std::string_view text, float x, float y, graphics::Color color)*

### draw_text

*public void draw_text(std::string_view text, float x, float y, const FontResource * font, graphics::Color color)*

### push_clip

*public void push_clip(const Rectangle & rectangle)*

### pop_clip

*public void pop_clip()*

### ~Painter

*public void ~Painter()*

*Defined at [gui/src/painter.cppm:23](../../../../gui/src/painter.cppm#L23)*



