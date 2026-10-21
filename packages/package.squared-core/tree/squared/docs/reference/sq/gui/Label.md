# class Label

*Defined at [gui/label.hpp:15](../../../../gui/label.hpp#L15)*



**brief** Read-only text widget. 



Inherits from Widget



## Functions

### Label

*public void Label(std::string text)*



**brief** Construct a label.

**text** Initial text content.

### set_text

*public void set_text(std::string text)*



**brief** Replace the label text.

**text** New text content.

### set_style

*public void set_style(std::string style)*



**brief** Select a named LabelStyle from the active Skin.

**style** Style name; an unknown name falls back to `default`.

### preferred_size

*public Size preferred_size(Painter & painter, const Skin & skin)*



**brief** Report the measured text size.

**painter** Active painter used for measurement.

**skin** Skin providing the selected label style and font.

**return** Preferred extent in logical units.

### paint

*public void paint(Painter & , const Skin & , float , float )*



**brief** Paint the label text.

**note** Parameters match the Widget::paint signature: the active painter, the skin providing text colors, and the stage-space origin.

### text

*public const std::string & text()*

*Defined at [gui/label.hpp:34](../../../../gui/label.hpp#L34)*



**brief** Read the current text.

**return** Reference to the stored text.

### style

*public const std::string & style()*

*Defined at [gui/label.hpp:43](../../../../gui/label.hpp#L43)*



**brief** Return the selected label-style name. 

### set_muted

*public void set_muted(bool muted)*

*Defined at [gui/label.hpp:49](../../../../gui/label.hpp#L49)*



**brief** Control the muted visual style.

**muted** true renders with the muted text color.



