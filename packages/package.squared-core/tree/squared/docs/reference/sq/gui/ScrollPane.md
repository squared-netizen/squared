# class ScrollPane

*Defined at [gui/scroll_pane.hpp:18](../../../../gui/scroll_pane.hpp#L18)*



**brief** Container clipping and vertically scrolling a larger child. 



Inherits from Widget



## Functions

### scroll_y

*public float scroll_y()*

*Defined at [gui/scroll_pane.hpp:38](../../../../gui/scroll_pane.hpp#L38)*



**brief** Read the vertical scroll offset.

**return** Current offset in logical units.

### set_content

*public Widget & set_content(int content)*



**brief** Set the single scrollable child.

**content** Widget to adopt; ownership transfers to the pane.

**return** Reference to the adopted content widget.

### set_scroll_y

*public void set_scroll_y(float scroll_y)*



**brief** Set the vertical scroll offset.

**scroll_y** Offset in logical units; clamped to valid range.

### preferred_size

*public Size preferred_size(Painter & painter, const Skin & skin)*



**brief** Report the content size.

**painter** Active painter used for measurement.

**skin** Skin forwarded to the content widget.

**return** Preferred extent in logical units.

### layout

*public void layout(Painter & , const Skin & )*



**brief** Position the content and clamp the scroll offset.

**note** Parameters match Widget::layout: an active painter and the skin providing style values.

### pointer_event

*public bool pointer_event(const PointerEvent & )*



**brief** Handle drag-to-scroll pointer events.

**note** The parameter matches Widget::pointer_event: a pointer payload in widget-local coordinates.

**return** true when the event was handled.



