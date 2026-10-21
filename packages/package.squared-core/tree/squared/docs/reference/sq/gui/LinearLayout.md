# class LinearLayout

*Defined at [gui/linear_layout.hpp:19](../../../../gui/linear_layout.hpp#L19)*



**brief** Container stacking children along one axis with optional growth.



Inherits from Widget



## Functions

### LinearLayout

*public void LinearLayout(Direction direction)*



**brief** Construct a linear layout.

**direction** Stacking axis.

### add

*public Widget & add(std::unique_ptr<Widget> child, float grow)*



**brief** Add a child widget.

**child** Widget to adopt; ownership transfers to the layout.

**grow** Extra stretch share for the child, in logical units.

**return** Reference to the added child.

### set_padding

*public void set_padding(float padding)*



**brief** Set layout padding.

**padding** Padding in logical units; non-negative.

### set_spacing

*public void set_spacing(float spacing)*



**brief** Set spacing between children.

**spacing** Gap in logical units; non-negative.

### minimum_size

*public Size minimum_size(Painter & , const Skin & )*



**brief** Report the smallest size fitting all children.

**note** Parameters match Widget::minimum_size: an active painter and the skin providing style values.

**return** Minimum extent in logical units.

### preferred_size

*public Size preferred_size(Painter & painter, const Skin & skin)*



**brief** Report the preferred size from stacked content.

**painter** Active painter used for measurement.

**skin** Skin forwarded to every child widget.

**return** Preferred extent in logical units.

### layout

*public void layout(Painter & , const Skin & )*



**brief** Position children along the layout axis.

**note** Parameters match Widget::layout: an active painter and the skin providing style values.



