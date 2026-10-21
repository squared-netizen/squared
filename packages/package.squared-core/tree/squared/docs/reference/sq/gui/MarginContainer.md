# class MarginContainer

*Defined at [gui/margin_container.hpp:16](../../../../gui/margin_container.hpp#L16)*



**brief** Container adding fixed insets around one child. 



Inherits from Widget



## Functions

### MarginContainer

*public void MarginContainer(Insets margin)*



**brief** Construct a margin container.

**margin** Insets in logical units; non-negative.

### set_content

*public Widget & set_content(std::unique_ptr<Widget> content)*



**brief** Set the single wrapped child.

**content** Widget to adopt; ownership transfers to the container.

**return** Reference to the adopted content widget.

### preferred_size

*public Size preferred_size(Painter & painter, const Skin & skin)*



**brief** Report the content size plus margins.

**painter** Active painter used for measurement.

**skin** Skin forwarded to the content widget.

**return** Preferred extent in logical units.

### layout

*public void layout(Painter & , const Skin & )*



**brief** Position the content inside the margin insets.

**note** Parameters match Widget::layout: an active painter and the skin providing style values.



