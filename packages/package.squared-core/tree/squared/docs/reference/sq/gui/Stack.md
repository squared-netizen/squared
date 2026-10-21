# class Stack

*Defined at [gui/stack.hpp:15](../../../../gui/stack.hpp#L15)*



**brief** Container overlaying children into the same box, later on top. 



Inherits from Widget



## Functions

### add

*public Widget & add(std::unique_ptr<Widget> child)*



**brief** Add a child widget to the stack.

**child** Widget to adopt; ownership transfers to the stack.

**return** Reference to the added child.

### preferred_size

*public Size preferred_size(Painter & painter, const Skin & skin)*



**brief** Report the largest child as preferred.

**painter** Active painter used for measurement.

**skin** Skin forwarded to every child widget.

**return** Preferred extent in logical units.

### layout

*public void layout(Painter & , const Skin & )*



**brief** Expand every child to the stack box.

**note** Parameters match Widget::layout: an active painter and the skin providing style values.



