# class ColorDrawable

*Defined at [gui/src/color_drawable.hpp:17](../../../../gui/src/color_drawable.hpp#L17)*



**brief** Drawable that paints a flat, optionally inset color.



Inherits from Drawable



## Functions

### ColorDrawable

*public void ColorDrawable(graphics::Color color, Size minimum, Insets insets)*



**brief** Construct a flat color drawable.

**color** Normalized fill color.

**minimum** Smallest acceptable size in logical units.

**insets** Content insets in logical units.

### minimum_size

*public Size minimum_size()*



**brief** Report the smallest acceptable size.

**return** Minimum extent in logical units.

### content_insets

*public Insets content_insets()*



**brief** Report the content insets.

**return** Insets between the outer box and the flat color.

### draw

*public void draw(Painter & painter, const Rectangle & rectangle, graphics::Color tint)*



**brief** Paint the flat color into the rectangle.

**painter** Destination painter; valid for the call.

**rectangle** Target rectangle in logical units.

**tint** Color multiplied with the stored fill color.



