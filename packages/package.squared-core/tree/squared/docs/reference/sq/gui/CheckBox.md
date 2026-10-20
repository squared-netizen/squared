# class CheckBox

*Defined at [gui/check_box.hpp:15](../../../../gui/check_box.hpp#L15)*



**brief** Toggle button rendering a checkbox glyph and label. 



Inherits from ToggleButton



## Functions

### CheckBox

*public void CheckBox(int text, bool checked)*



**brief** Construct a checkbox.

**text** Optional label text.

**checked** Initial checked state.

### set_check_style

*public void set_check_style(int style)*



**brief** Change the applied check-box style.

**style** Named check-box style to apply.

### minimum_size

*public Size minimum_size(Painter & , const Skin & )*



**brief** Report the touch-target-driven minimum size.

**note** Parameters match Widget::minimum_size: an active painter and the skin providing the check-box style.

**return** Minimum extent in logical units.

### preferred_size

*public Size preferred_size(Painter & painter, const Skin & skin)*



**brief** Report the box-plus-label size.

**painter** Active painter used for measurement.

**skin** Skin providing the selected check-box style and font.

**return** Preferred extent in logical units.

### paint

*public void paint(Painter & , const Skin & , float , float )*



**brief** Paint the checkbox glyph and label.

**note** Parameters match the Widget::paint signature: the active painter, the skin providing the check-box style, and the stage-space origin.



