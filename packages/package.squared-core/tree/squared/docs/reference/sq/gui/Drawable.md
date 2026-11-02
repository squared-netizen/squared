# class Drawable

*Defined at [gui/src/drawable.cppm:19](../../../../gui/src/drawable.cppm#L19)*



**brief** Skin image abstraction; it never exposes SDL or another backend type.

 Drawables are immutable and shared through DrawablePtr. Any referenced Texture or TextureAtlas must outlive every use of the drawable.



## Functions

### content_insets

*public Insets content_insets()*



**brief** Report the region reserved for content.

**return** Insets between the outer box and drawable content.

### draw

*public void draw(Painter & painter, const Rectangle & rectangle, graphics::Color tint)*



**brief** Draw the image into a rectangle.

**painter** Destination painter; valid for the call.

**rectangle** Target rectangle in logical units.

**tint** Color multiplied with the drawable's own coloring.

### minimum_size

*public Size minimum_size()*



**brief** Report the smallest acceptable size.

**return** Minimum extent in logical units.

### ~Drawable

*public void ~Drawable()*

*Defined at [gui/src/drawable.cppm:22](../../../../gui/src/drawable.cppm#L22)*

