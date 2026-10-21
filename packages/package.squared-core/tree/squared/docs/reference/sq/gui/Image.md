# class Image

*Defined at [gui/image.hpp:14](../../../../gui/image.hpp#L14)*



**brief** Widget rendering one shared drawable. 



Inherits from Widget



## Functions

### Image

*public void Image(DrawablePtr drawable)*



**brief** Construct an image widget.

**drawable** Drawable to display, or empty for a blank image.

### set_drawable

*public void set_drawable(DrawablePtr drawable)*



**brief** Replace the displayed drawable.

**drawable** Drawable to display; shared ownership.

### preferred_size

*public Size preferred_size(Painter & painter, const Skin & skin)*



**brief** Report the drawable size.

**painter** Active painter used for measurement.

**skin** Active skin; unused by image measurement.

**return** Preferred extent in logical units.

### paint

*public void paint(Painter & , const Skin & , float , float )*



**brief** Paint the drawable.

**note** Parameters match the Widget::paint signature: the active painter, the skin providing style resources, and the stage-space origin.



