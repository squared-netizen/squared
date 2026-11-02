# class Separator

*Defined at [gui/separator.cppm:14](../../../../gui/separator.cppm#L14)*



**brief** Thin vertical or horizontal divider line. 



Inherits from Widget



## Functions

### Separator

*public void Separator(Direction direction)*



**brief** Construct a separator.

**direction** Orientation of the divider.

### paint

*public void paint(Painter & , const Skin & , float , float )*



**brief** Paint the divider line.

**note** Parameters match the Widget::paint signature: the active painter, the skin providing the border color, and the stage-space origin.

### preferred_size

*public Size preferred_size(Painter & painter, const Skin & skin)*



**brief** Report the line-driven preferred size.

**painter** Active painter used for measurement.

**skin** Active skin; unused by separator measurement.

**return** Preferred extent in logical units.

